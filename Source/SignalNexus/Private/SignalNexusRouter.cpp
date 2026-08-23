// Copyright 2026 Silvan Teufel All Rights Reserved.

#include "SignalNexusRouter.h"
#include "SignalNexusInterceptor.h"
#include "SignalNexusLog.h"

FSignalNexusRouter::~FSignalNexusRouter()
{
	Reset();
}

FSignalNexusHandle FSignalNexusRouter::Subscribe(FGameplayTag Channel, FSignalCallback&& Callback, TWeakObjectPtr<UObject> Owner)
{
	if (!Channel.IsValid())
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("Subscribe rejected: invalid channel tag."));
		return FSignalNexusHandle();
	}
	if (!Callback)
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("Subscribe rejected: null callback for channel '%s'."), *Channel.ToString());
		return FSignalNexusHandle();
	}

	const FSignalNexusHandle Handle(NextHandleId++);

	FSubscription Sub;
	Sub.Handle = Handle;
	Sub.Callback = MoveTemp(Callback);
	Sub.Owner = Owner;
	Sub.bHasOwner = (Owner.Get() != nullptr);

	Subscribers.FindOrAdd(Channel).Add(MoveTemp(Sub));
	HandleToChannel.Add(Handle, Channel);

	UE_LOG(LogSignalNexus, Verbose, TEXT("Subscribed (handle %lld) to channel '%s'."), Handle.GetId(), *Channel.ToString());
	return Handle;
}

void FSignalNexusRouter::Unsubscribe(const FSignalNexusHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

	const FGameplayTag* Channel = HandleToChannel.Find(Handle);
	if (!Channel)
	{
		return;
	}

	if (TArray<FSubscription>* Bucket = Subscribers.Find(*Channel))
	{
		Bucket->RemoveAll([&Handle](const FSubscription& Sub) { return Sub.Handle == Handle; });
		if (Bucket->Num() == 0)
		{
			Subscribers.Remove(*Channel);
		}
	}

	HandleToChannel.Remove(Handle);
}

void FSignalNexusRouter::Broadcast(FGameplayTag Channel, FSignalNexusPayload&& Payload, ESignalNexusPriority Priority, float ThrottleRate)
{
	if (!Channel.IsValid())
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("Broadcast rejected: invalid channel tag."));
		return;
	}

	switch (Priority)
	{
	case ESignalNexusPriority::Immediate:
	{
		DispatchNow(Channel, Payload);
		break;
	}

	case ESignalNexusPriority::DeferredNextFrame:
	{
		FPendingSignal Pending;
		Pending.Channel = Channel;
		Pending.Payload = MoveTemp(Payload);
		DeferredQueue.Add(MoveTemp(Pending));
		break;
	}

	case ESignalNexusPriority::Throttled:
	{
		FThrottleState& State = ThrottleStates.FindOrAdd(Channel);
		if (ThrottleRate > 0.f)
		{
			State.Interval = 1.0 / static_cast<double>(ThrottleRate);
		}

		if (CurrentTimeSeconds >= State.NextAllowedTime)
		{
			DispatchNow(Channel, Payload);
			State.NextAllowedTime = CurrentTimeSeconds + State.Interval;
			State.bHasPending = false;
			State.Pending.Reset();
		}
		else
		{
			// Coalesce: keep only the most recent payload until the channel is allowed to fire again.
			State.Pending = MoveTemp(Payload);
			State.bHasPending = true;
		}
		break;
	}

	default:
		break;
	}
}

void FSignalNexusRouter::RegisterInterceptor(FGameplayTag Channel, TWeakObjectPtr<UObject> Interceptor)
{
	UObject* Obj = Interceptor.Get();
	if (!Channel.IsValid() || !Obj)
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("RegisterInterceptor rejected: invalid channel or null object."));
		return;
	}

	if (!Obj->GetClass()->ImplementsInterface(USignalNexusInterceptor::StaticClass()))
	{
		UE_LOG(LogSignalNexus, Error,
			TEXT("RegisterInterceptor rejected: '%s' does not implement ISignalNexusInterceptor."), *Obj->GetName());
		return;
	}

	TArray<TWeakObjectPtr<UObject>>& Bucket = Interceptors.FindOrAdd(Channel);
	Bucket.AddUnique(Interceptor);
	UE_LOG(LogSignalNexus, Verbose, TEXT("Registered interceptor '%s' on channel '%s'."), *Obj->GetName(), *Channel.ToString());
}

void FSignalNexusRouter::UnregisterInterceptor(FGameplayTag Channel, const UObject* Interceptor)
{
	if (TArray<TWeakObjectPtr<UObject>>* Bucket = Interceptors.Find(Channel))
	{
		Bucket->RemoveAll([Interceptor](const TWeakObjectPtr<UObject>& Weak)
		{
			return Weak.Get() == Interceptor || !Weak.IsValid();
		});
		if (Bucket->Num() == 0)
		{
			Interceptors.Remove(Channel);
		}
	}
}

void FSignalNexusRouter::Tick(double CurrentTime)
{
	CurrentTimeSeconds = CurrentTime;

	// Flush deferred signals. Move the queue aside first so signals broadcast *during* delivery
	// are handled on the following tick rather than mutating what we iterate.
	if (DeferredQueue.Num() > 0)
	{
		TArray<FPendingSignal> ToProcess = MoveTemp(DeferredQueue);
		DeferredQueue.Reset();
		for (FPendingSignal& Signal : ToProcess)
		{
			DispatchNow(Signal.Channel, Signal.Payload);
		}
	}

	// Dispatch any throttled channels whose interval has elapsed and that have a coalesced payload.
	for (TPair<FGameplayTag, FThrottleState>& Pair : ThrottleStates)
	{
		FThrottleState& State = Pair.Value;
		if (State.bHasPending && CurrentTimeSeconds >= State.NextAllowedTime)
		{
			DispatchNow(Pair.Key, State.Pending);
			State.Pending.Reset();
			State.bHasPending = false;
			State.NextAllowedTime = CurrentTimeSeconds + State.Interval;
		}
	}
}

void FSignalNexusRouter::Reset()
{
	Subscribers.Reset();
	HandleToChannel.Reset();
	Interceptors.Reset();
	DeferredQueue.Reset();
	ThrottleStates.Reset();
}

void FSignalNexusRouter::CollectReferences(FReferenceCollector& Collector)
{
	for (FPendingSignal& Signal : DeferredQueue)
	{
		Signal.Payload.AddStructReferencedObjects(Collector);
	}
	for (TPair<FGameplayTag, FThrottleState>& Pair : ThrottleStates)
	{
		if (Pair.Value.bHasPending)
		{
			Pair.Value.Pending.AddStructReferencedObjects(Collector);
		}
	}
}

int32 FSignalNexusRouter::NumSubscribersExact(FGameplayTag Channel) const
{
	const TArray<FSubscription>* Bucket = Subscribers.Find(Channel);
	return Bucket ? Bucket->Num() : 0;
}

void FSignalNexusRouter::DispatchNow(FGameplayTag Channel, FSignalNexusPayload& Payload)
{
	if (!RunInterceptors(Channel, Payload))
	{
		UE_LOG(LogSignalNexus, Verbose, TEXT("Signal on '%s' blocked by an interceptor."), *Channel.ToString());
		return;
	}
	DeliverToSubscribers(Channel, Payload);
}

bool FSignalNexusRouter::RunInterceptors(FGameplayTag Channel, FSignalNexusPayload& Payload)
{
	FGameplayTag Current = Channel;
	while (Current.IsValid())
	{
		if (TArray<TWeakObjectPtr<UObject>>* Bucket = Interceptors.Find(Current))
		{
			for (int32 Index = Bucket->Num() - 1; Index >= 0; --Index)
			{
				UObject* Obj = (*Bucket)[Index].Get();
				if (!Obj)
				{
					Bucket->RemoveAt(Index);
					continue;
				}

				if (Obj->GetClass()->ImplementsInterface(USignalNexusInterceptor::StaticClass()))
				{
					const EInterceptorResult Result = ISignalNexusInterceptor::Execute_Intercept(Obj, Channel, Payload);
					if (Result == EInterceptorResult::Block)
					{
						return false;
					}
				}
			}
		}
		Current = Current.RequestDirectParent();
	}
	return true;
}

void FSignalNexusRouter::DeliverToSubscribers(FGameplayTag Channel, const FSignalNexusPayload& Payload)
{
	// Snapshot the matching callbacks first so a subscriber that (un)subscribes during delivery
	// cannot invalidate the containers we are iterating.
	TArray<FSignalCallback, TInlineAllocator<16>> ToInvoke;

	FGameplayTag Current = Channel;
	while (Current.IsValid())
	{
		if (TArray<FSubscription>* Bucket = Subscribers.Find(Current))
		{
			for (int32 Index = Bucket->Num() - 1; Index >= 0; --Index)
			{
				FSubscription& Sub = (*Bucket)[Index];
				if (Sub.bHasOwner && !Sub.Owner.IsValid())
				{
					// Owner died — prune the stale subscription.
					HandleToChannel.Remove(Sub.Handle);
					Bucket->RemoveAt(Index);
					continue;
				}
				ToInvoke.Add(Sub.Callback);
			}
		}
		Current = Current.RequestDirectParent();
	}

	for (const FSignalCallback& Callback : ToInvoke)
	{
		Callback(Channel, Payload);
	}
}

// Copyright 2026 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SignalNexusTypes.h"
#include "SignalNexusPayload.h"

class UObject;
class FReferenceCollector;

/**
 * FSignalNexusRouter — the pure, UObject-free routing core.
 *
 * Holds all subscriptions, interceptors and scheduling queues, and performs hierarchical
 * Gameplay-Tag delivery: a broadcast to `Event.Player.Spawns` reaches subscribers on
 * `Event.Player.Spawns`, `Event.Player` and `Event`. Kept independent of the subsystem so it is
 * unit-testable in isolation (the UGameInstanceSubsystem wrapper just forwards to it and drives Tick).
 */
class SIGNALNEXUS_API FSignalNexusRouter
{
public:
	/** Callback receives the exact broadcast channel plus the (possibly interceptor-modified) payload. */
	using FSignalCallback = TFunction<void(FGameplayTag /*Channel*/, const FSignalNexusPayload& /*Payload*/)>;

	FSignalNexusRouter() = default;
	~FSignalNexusRouter();

	FSignalNexusRouter(const FSignalNexusRouter&) = delete;
	FSignalNexusRouter& operator=(const FSignalNexusRouter&) = delete;

	/**
	 * Subscribe to a channel. Optionally bind an Owner UObject: when it dies the subscription is
	 * pruned automatically on the next delivery. Returns an invalid handle on bad input.
	 */
	FSignalNexusHandle Subscribe(FGameplayTag Channel, FSignalCallback&& Callback, TWeakObjectPtr<UObject> Owner = nullptr);

	/** Remove a single subscription by handle. Safe to call with an invalid/stale handle. */
	void Unsubscribe(const FSignalNexusHandle& Handle);

	/** Broadcast a payload on a channel using the given scheduling mode. */
	void Broadcast(FGameplayTag Channel, FSignalNexusPayload&& Payload, ESignalNexusPriority Priority, float ThrottleRate);

	/** Register a UObject interceptor (must implement ISignalNexusInterceptor) on a channel subtree. */
	void RegisterInterceptor(FGameplayTag Channel, TWeakObjectPtr<UObject> Interceptor);

	/** Unregister a previously registered interceptor from a channel. */
	void UnregisterInterceptor(FGameplayTag Channel, const UObject* Interceptor);

	/** Advance scheduled work: flush the deferred queue and any due throttled signals. */
	void Tick(double CurrentTime);

	/** Drop everything (subscriptions, interceptors, queues). */
	void Reset();

	/** GC hook — keep queued payloads' UObject references alive. */
	void CollectReferences(FReferenceCollector& Collector);

	/** Number of subscriptions bound exactly to Channel (diagnostics / tests). */
	int32 NumSubscribersExact(FGameplayTag Channel) const;

	/** Pending deferred signals awaiting the next Tick (diagnostics / tests). */
	int32 NumPendingDeferred() const { return DeferredQueue.Num(); }

private:
	struct FSubscription
	{
		FSignalNexusHandle Handle;
		FSignalCallback Callback;
		TWeakObjectPtr<UObject> Owner;
		bool bHasOwner = false;
	};

	struct FPendingSignal
	{
		FGameplayTag Channel;
		FSignalNexusPayload Payload;
	};

	struct FThrottleState
	{
		double NextAllowedTime = 0.0;
		double Interval = 0.0;
		bool bHasPending = false;
		FSignalNexusPayload Pending;
	};

	/** Run interceptors then deliver to subscribers, both walking the channel's parent chain. */
	void DispatchNow(FGameplayTag Channel, FSignalNexusPayload& Payload);

	/** Returns false if an interceptor blocked the signal. */
	bool RunInterceptors(FGameplayTag Channel, FSignalNexusPayload& Payload);

	void DeliverToSubscribers(FGameplayTag Channel, const FSignalNexusPayload& Payload);

	TMap<FGameplayTag, TArray<FSubscription>> Subscribers;
	TMap<FSignalNexusHandle, FGameplayTag> HandleToChannel;
	TMap<FGameplayTag, TArray<TWeakObjectPtr<UObject>>> Interceptors;

	TArray<FPendingSignal> DeferredQueue;
	TMap<FGameplayTag, FThrottleState> ThrottleStates;

	int64 NextHandleId = 1;
	double CurrentTimeSeconds = 0.0;
};

// Copyright 2026 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"
#include "UObject/StrongObjectPtr.h"
#include "GameplayTagsManager.h"
#include "SignalNexusRouter.h"
#include "SignalNexusTypes.h"
#include "SignalNexusPayload.h"
#include "SignalNexusInterceptor.h"

namespace SignalNexusTest
{
	FGameplayTag Tag(const TCHAR* Name)
	{
		return UGameplayTagsManager::Get().AddNativeGameplayTag(FName(Name));
	}

	template <typename T>
	FSignalNexusPayload MakePayload(const T& Value)
	{
		FSignalNexusPayload P;
		P.Pack(Value);
		return P;
	}
}

// --------------------------------------------------------------------------------------------
// 1) Hierarchical routing: a subscription on a parent channel receives child broadcasts.
// --------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalNexusHierarchyTest, "SignalNexus.Routing.Hierarchical",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSignalNexusHierarchyTest::RunTest(const FString& /*Parameters*/)
{
	const FGameplayTag Parent = SignalNexusTest::Tag(TEXT("Event.Player"));
	const FGameplayTag Spawns = SignalNexusTest::Tag(TEXT("Event.Player.Spawns"));
	const FGameplayTag Deaths = SignalNexusTest::Tag(TEXT("Event.Player.Deaths"));
	const FGameplayTag Unrelated = SignalNexusTest::Tag(TEXT("Event.Enemy.Spawns"));

	if (!Parent.IsValid() || !Spawns.IsValid())
	{
		AddInfo(TEXT("Could not register gameplay tags in this context — skipping."));
		return true;
	}

	FSignalNexusRouter Router;
	int32 Received = 0;
	int32 LastValue = -1;

	Router.Subscribe(Parent, [&](FGameplayTag Ch, const FSignalNexusPayload& P)
	{
		FSignalIntPayload Out;
		if (P.Unpack(Out)) { LastValue = Out.Value; }
		++Received;
	});

	Router.Broadcast(Spawns, SignalNexusTest::MakePayload(FSignalIntPayload(11)), ESignalNexusPriority::Immediate, 0.f);
	Router.Broadcast(Deaths, SignalNexusTest::MakePayload(FSignalIntPayload(22)), ESignalNexusPriority::Immediate, 0.f);
	Router.Broadcast(Unrelated, SignalNexusTest::MakePayload(FSignalIntPayload(99)), ESignalNexusPriority::Immediate, 0.f);

	TestEqual(TEXT("parent subscriber received both child signals"), Received, 2);
	TestEqual(TEXT("last delivered value came from the child channel"), LastValue, 22);
	return true;
}

// --------------------------------------------------------------------------------------------
// 2) Type safety: unpacking a payload as the wrong struct fails cleanly (no crash).
// --------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalNexusTypeSafetyTest, "SignalNexus.Payload.TypeSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSignalNexusTypeSafetyTest::RunTest(const FString& /*Parameters*/)
{
	FSignalDamagePayload Damage;
	Damage.Amount = 42.f;

	FSignalNexusPayload P = SignalNexusTest::MakePayload(Damage);
	TestTrue(TEXT("payload is valid"), P.IsValid());
	TestTrue(TEXT("payload reports the correct type"), P.IsA<FSignalDamagePayload>());

	// Correct unpack round-trips.
	FSignalDamagePayload OutDamage;
	TestTrue(TEXT("correct-type unpack succeeds"), P.Unpack(OutDamage));
	TestEqual(TEXT("value round-trips"), OutDamage.Amount, 42.f);

	// Wrong-type unpack must fail without corrupting the destination or crashing.
	FSignalIntPayload WrongOut;
	WrongOut.Value = 7;
	AddExpectedError(TEXT("Unpack type mismatch"), EAutomationExpectedErrorFlags::Contains, 0);
	const bool bWrong = P.Unpack(WrongOut);
	TestFalse(TEXT("wrong-type unpack fails"), bWrong);
	TestEqual(TEXT("destination untouched on failed unpack"), WrongOut.Value, 7);
	return true;
}

// --------------------------------------------------------------------------------------------
// 3) Interceptors: one interceptor modifies the payload, another blocks it.
// --------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalNexusInterceptorTest, "SignalNexus.Interceptors.ModifyAndBlock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSignalNexusInterceptorTest::RunTest(const FString& /*Parameters*/)
{
	const FGameplayTag Damage = SignalNexusTest::Tag(TEXT("Gameplay.Damage"));
	const FGameplayTag Physical = SignalNexusTest::Tag(TEXT("Gameplay.Damage.Physical"));
	if (!Damage.IsValid() || !Physical.IsValid())
	{
		AddInfo(TEXT("Could not register gameplay tags — skipping."));
		return true;
	}

	FSignalNexusRouter Router;
	float DeliveredAmount = -1.f;
	int32 DeliveredCount = 0;

	Router.Subscribe(Physical, [&](FGameplayTag, const FSignalNexusPayload& P)
	{
		FSignalDamagePayload Out;
		if (P.Unpack(Out)) { DeliveredAmount = Out.Amount; }
		++DeliveredCount;
	});

	// Interceptor on the *parent* channel halves incoming damage (hierarchical match).
	TStrongObjectPtr<USignalNexusLambdaInterceptor> Halver(NewObject<USignalNexusLambdaInterceptor>());
	Halver->InterceptFn = [](FGameplayTag, FSignalNexusPayload& P) -> EInterceptorResult
	{
		FSignalDamagePayload D;
		if (P.Unpack(D))
		{
			D.Amount *= 0.5f;
			P.Pack(D);
		}
		return EInterceptorResult::Pass;
	};
	Router.RegisterInterceptor(Damage, Halver.Get());

	FSignalDamagePayload In;
	In.Amount = 100.f;
	Router.Broadcast(Physical, SignalNexusTest::MakePayload(In), ESignalNexusPriority::Immediate, 0.f);

	TestEqual(TEXT("modified signal delivered once"), DeliveredCount, 1);
	TestEqual(TEXT("interceptor halved the damage"), DeliveredAmount, 50.f);

	// Now add a blocking interceptor — nothing further should be delivered.
	TStrongObjectPtr<USignalNexusLambdaInterceptor> Blocker(NewObject<USignalNexusLambdaInterceptor>());
	Blocker->InterceptFn = [](FGameplayTag, FSignalNexusPayload&) { return EInterceptorResult::Block; };
	Router.RegisterInterceptor(Physical, Blocker.Get());

	DeliveredCount = 0;
	Router.Broadcast(Physical, SignalNexusTest::MakePayload(In), ESignalNexusPriority::Immediate, 0.f);
	TestEqual(TEXT("blocked signal never reaches subscribers"), DeliveredCount, 0);
	return true;
}

// --------------------------------------------------------------------------------------------
// 4) Scheduling: deferred signals wait for a Tick; throttled signals coalesce per interval.
// --------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalNexusSchedulingTest, "SignalNexus.Scheduling.DeferredAndThrottled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSignalNexusSchedulingTest::RunTest(const FString& /*Parameters*/)
{
	const FGameplayTag Channel = SignalNexusTest::Tag(TEXT("Event.UI.Update"));
	if (!Channel.IsValid())
	{
		AddInfo(TEXT("Could not register gameplay tag — skipping."));
		return true;
	}

	FSignalNexusRouter Router;
	int32 Count = 0;
	int32 LastValue = -1;
	Router.Subscribe(Channel, [&](FGameplayTag, const FSignalNexusPayload& P)
	{
		FSignalIntPayload Out;
		if (P.Unpack(Out)) { LastValue = Out.Value; }
		++Count;
	});

	// Deferred: nothing until Tick.
	Router.Broadcast(Channel, SignalNexusTest::MakePayload(FSignalIntPayload(1)), ESignalNexusPriority::DeferredNextFrame, 0.f);
	Router.Broadcast(Channel, SignalNexusTest::MakePayload(FSignalIntPayload(2)), ESignalNexusPriority::DeferredNextFrame, 0.f);
	TestEqual(TEXT("deferred signals not yet delivered"), Count, 0);
	TestEqual(TEXT("two deferred signals queued"), Router.NumPendingDeferred(), 2);

	Router.Tick(0.016);
	TestEqual(TEXT("both deferred signals delivered on tick"), Count, 2);

	// Throttled @ 10 Hz (0.1s interval). First fires immediately, the rest coalesce.
	Count = 0;
	Router.Tick(1.0); // advance the clock baseline
	Router.Broadcast(Channel, SignalNexusTest::MakePayload(FSignalIntPayload(10)), ESignalNexusPriority::Throttled, 10.f);
	TestEqual(TEXT("first throttled signal fires immediately"), Count, 1);

	Router.Broadcast(Channel, SignalNexusTest::MakePayload(FSignalIntPayload(20)), ESignalNexusPriority::Throttled, 10.f);
	Router.Broadcast(Channel, SignalNexusTest::MakePayload(FSignalIntPayload(30)), ESignalNexusPriority::Throttled, 10.f);
	TestEqual(TEXT("subsequent throttled signals coalesce (not delivered yet)"), Count, 1);

	// Not enough time elapsed — still coalesced.
	Router.Tick(1.05);
	TestEqual(TEXT("throttled still coalesced before interval elapses"), Count, 1);

	// Interval elapsed — the latest coalesced payload fires once.
	Router.Tick(1.2);
	TestEqual(TEXT("one coalesced throttled signal fired after interval"), Count, 2);
	TestEqual(TEXT("coalesced signal carried the most recent value"), LastValue, 30);
	return true;
}

// --------------------------------------------------------------------------------------------
// 5) Unsubscribe stops delivery.
// --------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalNexusUnsubscribeTest, "SignalNexus.Routing.Unsubscribe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSignalNexusUnsubscribeTest::RunTest(const FString& /*Parameters*/)
{
	const FGameplayTag Channel = SignalNexusTest::Tag(TEXT("Event.Score.Changed"));
	if (!Channel.IsValid())
	{
		AddInfo(TEXT("Could not register gameplay tag — skipping."));
		return true;
	}

	FSignalNexusRouter Router;
	int32 Count = 0;
	const FSignalNexusHandle Handle = Router.Subscribe(Channel, [&](FGameplayTag, const FSignalNexusPayload&) { ++Count; });
	TestTrue(TEXT("subscribe returns a valid handle"), Handle.IsValid());

	Router.Broadcast(Channel, SignalNexusTest::MakePayload(FSignalIntPayload(1)), ESignalNexusPriority::Immediate, 0.f);
	TestEqual(TEXT("delivered while subscribed"), Count, 1);

	Router.Unsubscribe(Handle);
	Router.Broadcast(Channel, SignalNexusTest::MakePayload(FSignalIntPayload(2)), ESignalNexusPriority::Immediate, 0.f);
	TestEqual(TEXT("no delivery after unsubscribe"), Count, 1);
	TestEqual(TEXT("channel has no subscribers left"), Router.NumSubscribersExact(Channel), 0);
	return true;
}

// --------------------------------------------------------------------------------------------
// 6) Performance: 10,000 immediate signals in a frame should be cheap. We assert a generous
//    upper bound to avoid CI flakiness and log the real per-frame cost against the 0.5ms target.
// --------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalNexusPerformanceTest, "SignalNexus.Performance.TenThousandPerFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSignalNexusPerformanceTest::RunTest(const FString& /*Parameters*/)
{
	const FGameplayTag Channel = SignalNexusTest::Tag(TEXT("Bench.Signal"));
	if (!Channel.IsValid())
	{
		AddInfo(TEXT("Could not register gameplay tag — skipping."));
		return true;
	}

	FSignalNexusRouter Router;
	int64 Sum = 0;
	Router.Subscribe(Channel, [&](FGameplayTag, const FSignalNexusPayload& P)
	{
		FSignalIntPayload Out;
		if (P.Unpack(Out)) { Sum += Out.Value; }
	});

	constexpr int32 Count = 10000;
	const double Start = FPlatformTime::Seconds();
	for (int32 i = 0; i < Count; ++i)
	{
		Router.Broadcast(Channel, SignalNexusTest::MakePayload(FSignalIntPayload(1)), ESignalNexusPriority::Immediate, 0.f);
	}
	const double ElapsedMs = (FPlatformTime::Seconds() - Start) * 1000.0;

	AddInfo(FString::Printf(TEXT("SignalNexus: %d immediate signals dispatched in %.3f ms (target < 0.5 ms)."), Count, ElapsedMs));
	TestEqual(TEXT("every signal was delivered"), (int32)Sum, Count);
	// Generous bound: correctness of the benchmark harness, not a hard perf gate on arbitrary CI hardware.
	TestTrue(FString::Printf(TEXT("10k signals dispatched in a reasonable time (%.3f ms)"), ElapsedMs), ElapsedMs < 50.0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

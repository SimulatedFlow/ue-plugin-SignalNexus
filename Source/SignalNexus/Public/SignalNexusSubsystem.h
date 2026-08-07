// Copyright 2026 Silvan Teufel All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "GameplayTagContainer.h"
#include "UObject/ScriptInterface.h"
#include "SignalNexusTypes.h"
#include "SignalNexusPayload.h"
#include "SignalNexusRouter.h"
#include "SignalNexusSubsystem.generated.h"

class ISignalNexusInterceptor;

/**
 * USignalNexusSubsystem — the global signal bus.
 *
 * A GameInstance-scoped hub over an FSignalNexusRouter. Provides a type-safe C++ template API
 * (Subscribe/BroadcastSignal), UObject interceptor registration, and drives deferred/throttled
 * delivery through its tickable. Blueprint access lives in USignalNexusBlueprintLibrary.
 */
UCLASS()
class SIGNALNEXUS_API USignalNexusSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	/** Convenience accessor from any world context object. */
	static USignalNexusSubsystem* Get(const UObject* WorldContextObject);

	/**
	 * Subscribe to a channel for a specific payload type. The callback only fires for signals
	 * carrying a T (mismatched types are ignored with a logged warning). Returns a handle for Unsubscribe.
	 */
	template <typename T>
	FSignalNexusHandle Subscribe(FGameplayTag Channel, TFunction<void(const T&)> Callback)
	{
		UScriptStruct* Expected = T::StaticStruct();
		return Router.Subscribe(Channel,
			[Cb = MoveTemp(Callback), Expected](FGameplayTag /*Channel*/, const FSignalNexusPayload& Payload)
			{
				if (Payload.GetStructType() != Expected)
				{
					return;
				}
				T Value;
				if (Payload.UnpackRaw(Expected, &Value))
				{
					Cb(Value);
				}
			});
	}

	/** Broadcast a typed payload on a channel. */
	template <typename T>
	void BroadcastSignal(FGameplayTag Channel, const T& Payload,
		ESignalNexusPriority Priority = ESignalNexusPriority::Immediate, float ThrottleRate = 30.f)
	{
		FSignalNexusPayload Packed;
		Packed.Pack(Payload);
		Router.Broadcast(Channel, MoveTemp(Packed), Priority, ThrottleRate);
	}

	/** Remove a subscription previously returned by Subscribe. */
	void Unsubscribe(FSignalNexusHandle Handle);

	/** Register / unregister a UObject interceptor (implements ISignalNexusInterceptor) on a channel subtree. */
	UFUNCTION(BlueprintCallable, Category = "SignalNexus|Interceptors")
	void RegisterInterceptor(FGameplayTag Channel, TScriptInterface<ISignalNexusInterceptor> Interceptor);

	UFUNCTION(BlueprintCallable, Category = "SignalNexus|Interceptors")
	void UnregisterInterceptor(FGameplayTag Channel, TScriptInterface<ISignalNexusInterceptor> Interceptor);

	/** Direct access to the routing core (used by the Blueprint library and advanced C++ callers). */
	FSignalNexusRouter& GetRouter() { return Router; }
	const FSignalNexusRouter& GetRouter() const { return Router; }

	//~ Begin FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bInitialized; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual UWorld* GetTickableGameObjectWorld() const override;
	//~ End FTickableGameObject interface

	/** GC: keep UObject references inside queued payloads alive. */
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

private:
	FSignalNexusRouter Router;

	/** Monotonic in-game clock used to drive throttle intervals (accumulated tick delta). */
	double AccumulatedTime = 0.0;

	bool bInitialized = false;
};

// Copyright 2026 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "SignalNexusTypes.h"
#include "SignalNexusPayload.h"
#include "SignalNexusInterceptor.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, meta = (DisplayName = "Signal Nexus Interceptor"))
class USignalNexusInterceptor : public UInterface
{
	GENERATED_BODY()
};

/**
 * Middleware hook. An object implementing this interface can be registered on a Gameplay-Tag
 * channel (hierarchically — an interceptor on `Gameplay.Damage` also sees `Gameplay.Damage.Physical`).
 * For every matching signal it may modify the payload in place, Pass it, or Block it entirely.
 */
class ISignalNexusInterceptor
{
	GENERATED_BODY()

public:
	/**
	 * Inspect / mutate a signal before it reaches subscribers.
	 * @param Channel  The exact channel the signal was broadcast on.
	 * @param Payload  The signal payload, mutable — modifications are seen by later interceptors and subscribers.
	 * @return Pass to let it continue, Block to drop it.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SignalNexus")
	EInterceptorResult Intercept(FGameplayTag Channel, UPARAM(ref) FSignalNexusPayload& Payload);
	virtual EInterceptorResult Intercept_Implementation(FGameplayTag Channel, FSignalNexusPayload& Payload)
	{
		return EInterceptorResult::Pass;
	}
};

/**
 * A ready-to-use interceptor whose behaviour is supplied as a C++ lambda. Handy for tests and for
 * lightweight, code-side middleware without declaring a whole UClass.
 */
UCLASS(NotBlueprintable)
class SIGNALNEXUS_API USignalNexusLambdaInterceptor : public UObject, public ISignalNexusInterceptor
{
	GENERATED_BODY()

public:
	/** The interception logic. If unset, the interceptor passes everything through. */
	TFunction<EInterceptorResult(FGameplayTag, FSignalNexusPayload&)> InterceptFn;

	virtual EInterceptorResult Intercept_Implementation(FGameplayTag Channel, FSignalNexusPayload& Payload) override
	{
		return InterceptFn ? InterceptFn(Channel, Payload) : EInterceptorResult::Pass;
	}
};

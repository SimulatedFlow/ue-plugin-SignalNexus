// Copyright 2026 Silvan Teufel All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SignalNexusTypes.h"
#include "SignalNexusPayload.h"
#include "SignalNexusBlueprintLibrary.generated.h"

/**
 * Blueprint callback for a received signal: the exact channel plus the raw payload.
 * Use UnpackSignalPayload to extract the payload into a typed (wildcard) struct.
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FSignalNexusReceivedDelegate, FGameplayTag, Channel, const FSignalNexusPayload&, Payload);

/**
 * USignalNexusBlueprintLibrary — the Blueprint face of SignalNexus.
 *
 * Broadcast and Unpack are wildcard (custom-thunk) nodes: a designer can send and receive *any*
 * struct — including Blueprint-only structs — with no C++ registration and no manual casting.
 */
UCLASS()
class SIGNALNEXUS_API USignalNexusBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Broadcast any struct on a channel. `Value` is a wildcard pin — connect any struct.
	 * Throttled mode uses ThrottleRate (max dispatches per second for this channel).
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "SignalNexus",
		meta = (WorldContext = "WorldContextObject", CustomStructureParam = "Value",
			AdvancedDisplay = "Priority,ThrottleRate", DisplayName = "Broadcast Signal"))
	static void BroadcastSignal(UObject* WorldContextObject, FGameplayTag Channel, const int32& Value,
		ESignalNexusPriority Priority = ESignalNexusPriority::Immediate, float ThrottleRate = 30.f);
	DECLARE_FUNCTION(execBroadcastSignal);

	/**
	 * Subscribe to a channel. OnSignal fires for signals on this channel and any of its children
	 * (hierarchical). Returns a handle; keep it to Unsubscribe. The subscription is auto-removed
	 * when the delegate's bound object is destroyed.
	 */
	UFUNCTION(BlueprintCallable, Category = "SignalNexus",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Subscribe To Signal"))
	static FSignalNexusHandle SubscribeToSignal(UObject* WorldContextObject, FGameplayTag Channel, FSignalNexusReceivedDelegate OnSignal);

	/** Remove a subscription by handle. */
	UFUNCTION(BlueprintCallable, Category = "SignalNexus",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Unsubscribe From Signal"))
	static void UnsubscribeFromSignal(UObject* WorldContextObject, FSignalNexusHandle Handle);

	/**
	 * Extract a payload into a typed (wildcard) struct. `OutValue` is a wildcard out-pin.
	 * Returns true only if the payload's stored type matches the connected struct type.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "SignalNexus",
		meta = (CustomStructureParam = "OutValue", DisplayName = "Unpack Signal Payload"))
	static bool UnpackSignalPayload(const FSignalNexusPayload& Payload, int32& OutValue);
	DECLARE_FUNCTION(execUnpackSignalPayload);

	/** True if the payload holds a value. */
	UFUNCTION(BlueprintPure, Category = "SignalNexus", meta = (DisplayName = "Is Payload Valid"))
	static bool IsPayloadValid(const FSignalNexusPayload& Payload) { return Payload.IsValid(); }

	/** The struct type name carried by the payload ("<empty>" if none) — for debug UI. */
	UFUNCTION(BlueprintPure, Category = "SignalNexus", meta = (DisplayName = "Get Payload Type Name"))
	static FString GetPayloadTypeName(const FSignalNexusPayload& Payload) { return Payload.GetStructName(); }

	/** True if the handle refers to an active subscription value (non-zero id). */
	UFUNCTION(BlueprintPure, Category = "SignalNexus", meta = (DisplayName = "Is Valid Signal Handle"))
	static bool IsValidHandle(const FSignalNexusHandle& Handle) { return Handle.IsValid(); }

private:
	static void GenericBroadcast(UObject* WorldContextObject, FGameplayTag Channel,
		const FProperty* ValueProp, const void* ValuePtr, ESignalNexusPriority Priority, float ThrottleRate);

	static bool GenericUnpack(const FSignalNexusPayload& Payload, const FProperty* OutProp, void* OutPtr);
};

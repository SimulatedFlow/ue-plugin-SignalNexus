// Copyright 2026 Simulated Flow All Rights Reserved.

#include "SignalNexusBlueprintLibrary.h"
#include "SignalNexusSubsystem.h"
#include "SignalNexusLog.h"
#include "UObject/UnrealType.h"

void USignalNexusBlueprintLibrary::GenericBroadcast(UObject* WorldContextObject, FGameplayTag Channel,
	const FProperty* ValueProp, const void* ValuePtr, ESignalNexusPriority Priority, float ThrottleRate)
{
	USignalNexusSubsystem* Subsystem = USignalNexusSubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("BroadcastSignal: no SignalNexus subsystem for the given context."));
		return;
	}

	const FStructProperty* StructProp = CastField<FStructProperty>(ValueProp);
	if (!StructProp || !StructProp->Struct || !ValuePtr)
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("BroadcastSignal on '%s': the payload pin must be a struct."), *Channel.ToString());
		return;
	}

	FSignalNexusPayload Payload;
	Payload.PackRaw(StructProp->Struct, ValuePtr);
	Subsystem->GetRouter().Broadcast(Channel, MoveTemp(Payload), Priority, ThrottleRate);
}

bool USignalNexusBlueprintLibrary::GenericUnpack(const FSignalNexusPayload& Payload, const FProperty* OutProp, void* OutPtr)
{
	const FStructProperty* StructProp = CastField<FStructProperty>(OutProp);
	if (!StructProp || !StructProp->Struct || !OutPtr)
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("UnpackSignalPayload: the output pin must be a struct."));
		return false;
	}

	return Payload.UnpackRaw(StructProp->Struct, OutPtr);
}

DEFINE_FUNCTION(USignalNexusBlueprintLibrary::execBroadcastSignal)
{
	P_GET_OBJECT(UObject, WorldContextObject);
	P_GET_STRUCT(FGameplayTag, Channel);

	// Wildcard payload pin — read whatever property/struct was connected.
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProp = Stack.MostRecentProperty;
	const void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_GET_ENUM(ESignalNexusPriority, Priority);
	P_GET_PROPERTY(FFloatProperty, ThrottleRate);

	P_FINISH;

	P_NATIVE_BEGIN;
	GenericBroadcast(WorldContextObject, Channel, ValueProp, ValuePtr, Priority, ThrottleRate);
	P_NATIVE_END;
}

FSignalNexusHandle USignalNexusBlueprintLibrary::SubscribeToSignal(UObject* WorldContextObject, FGameplayTag Channel, FSignalNexusReceivedDelegate OnSignal)
{
	USignalNexusSubsystem* Subsystem = USignalNexusSubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("SubscribeToSignal: no SignalNexus subsystem for the given context."));
		return FSignalNexusHandle();
	}

	TWeakObjectPtr<UObject> Owner = OnSignal.GetUObject();
	return Subsystem->GetRouter().Subscribe(Channel,
		[OnSignal](FGameplayTag SignalChannel, const FSignalNexusPayload& Payload)
		{
			if (OnSignal.IsBound())
			{
				OnSignal.Execute(SignalChannel, Payload);
			}
		},
		Owner);
}

void USignalNexusBlueprintLibrary::UnsubscribeFromSignal(UObject* WorldContextObject, FSignalNexusHandle Handle)
{
	if (USignalNexusSubsystem* Subsystem = USignalNexusSubsystem::Get(WorldContextObject))
	{
		Subsystem->Unsubscribe(Handle);
	}
}

DEFINE_FUNCTION(USignalNexusBlueprintLibrary::execUnpackSignalPayload)
{
	P_GET_STRUCT_REF(FSignalNexusPayload, Payload);

	// Wildcard output pin.
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* OutProp = Stack.MostRecentProperty;
	void* OutPtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	bool bSuccess = false;
	P_NATIVE_BEGIN;
	bSuccess = GenericUnpack(Payload, OutProp, OutPtr);
	P_NATIVE_END;

	*static_cast<bool*>(RESULT_PARAM) = bSuccess;
}

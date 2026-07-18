// Copyright 2026 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#include "SignalNexusSubsystem.h"
#include "SignalNexusInterceptor.h"
#include "SignalNexusLog.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void USignalNexusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AccumulatedTime = 0.0;
	bInitialized = true;
	UE_LOG(LogSignalNexus, Log, TEXT("SignalNexus subsystem initialized."));
}

void USignalNexusSubsystem::Deinitialize()
{
	bInitialized = false;
	Router.Reset();
	UE_LOG(LogSignalNexus, Log, TEXT("SignalNexus subsystem deinitialized."));
	Super::Deinitialize();
}

USignalNexusSubsystem* USignalNexusSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		return GameInstance->GetSubsystem<USignalNexusSubsystem>();
	}
	return nullptr;
}

void USignalNexusSubsystem::Unsubscribe(FSignalNexusHandle Handle)
{
	Router.Unsubscribe(Handle);
}

void USignalNexusSubsystem::RegisterInterceptor(FGameplayTag Channel, TScriptInterface<ISignalNexusInterceptor> Interceptor)
{
	if (UObject* Obj = Interceptor.GetObject())
	{
		Router.RegisterInterceptor(Channel, Obj);
	}
	else
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("RegisterInterceptor: interceptor object is null."));
	}
}

void USignalNexusSubsystem::UnregisterInterceptor(FGameplayTag Channel, TScriptInterface<ISignalNexusInterceptor> Interceptor)
{
	if (UObject* Obj = Interceptor.GetObject())
	{
		Router.UnregisterInterceptor(Channel, Obj);
	}
}

void USignalNexusSubsystem::Tick(float DeltaTime)
{
	AccumulatedTime += static_cast<double>(DeltaTime);
	Router.Tick(AccumulatedTime);
}

TStatId USignalNexusSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USignalNexusSubsystem, STATGROUP_Tickables);
}

UWorld* USignalNexusSubsystem::GetTickableGameObjectWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}

void USignalNexusSubsystem::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);
	USignalNexusSubsystem* This = CastChecked<USignalNexusSubsystem>(InThis);
	This->Router.CollectReferences(Collector);
}

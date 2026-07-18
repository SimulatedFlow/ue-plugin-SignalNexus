// Copyright 2026 Simulated Flow All Rights Reserved.

#include "SignalNexus.h"
#include "SignalNexusLog.h"

DEFINE_LOG_CATEGORY(LogSignalNexus);

#define LOCTEXT_NAMESPACE "FSignalNexusModule"

void FSignalNexusModule::StartupModule()
{
	UE_LOG(LogSignalNexus, Log, TEXT("SignalNexus started."));
}

void FSignalNexusModule::ShutdownModule()
{
	UE_LOG(LogSignalNexus, Log, TEXT("SignalNexus shut down."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSignalNexusModule, SignalNexus)

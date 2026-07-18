// Copyright 2026 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

/**
 * SignalNexus — advanced runtime signal bus.
 * Runtime module: the payload container, the routing core, the Gameplay-Instance subsystem,
 * the interceptor interface, and the Blueprint (custom-thunk) function library.
 */
class FSignalNexusModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

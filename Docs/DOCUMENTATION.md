# SignalNexus — Advanced Runtime Signal Bus

**User Documentation** · Version 1.0.0 · Unreal Engine 5.8

A high-performance, fully decoupled signal & message broker built on hierarchical
**Gameplay Tags**. Systems, actors and UI communicate without ever holding a hard
reference to each other — broadcast a signal on a tag channel, subscribe wherever you
like, in C++ **or** Blueprints.

---

## Table of Contents

1. [Requirements & Supported Platforms](#1-requirements--supported-platforms)
2. [Installation](#2-installation)
3. [Core Concepts](#3-core-concepts)
4. [Quick Start — Blueprints](#4-quick-start--blueprints)
5. [Quick Start — C++](#5-quick-start--c)
6. [API / Class Overview](#6-api--class-overview)
7. [Code Examples](#7-code-examples)
8. [Scheduling Modes](#8-scheduling-modes)
9. [Interceptors (Middleware)](#9-interceptors-middleware)
10. [Type Safety & Payloads](#10-type-safety--payloads)
11. [Performance](#11-performance)
12. [Troubleshooting](#12-troubleshooting)
13. [Support](#13-support)

---

## 1. Requirements & Supported Platforms

| Item | Detail |
|------|--------|
| **Engine** | Unreal Engine **5.8** (`EngineVersion 5.8.0`) |
| **Module type** | `Runtime`, `LoadingPhase: PreDefault` |
| **Language** | C++ (source included) and Blueprints |
| **Engine dependencies** | `Core`, `CoreUObject`, `Engine`, `GameplayTags` only |
| **Third-party plugin deps** | None — fully self-contained |
| **Content** | Optional (`CanContainContent: true`); the module ships no required content |

**Supported platforms.** SignalNexus is pure gameplay-framework C++ with no
platform-specific code, no native/third-party libraries and no editor-only
dependencies. It therefore builds and runs on **all UE-supported platforms**,
including macOS, Linux, Android, iOS and the major consoles. The `.uplugin`
nevertheless lists **`Win64` only** in its `PlatformAllowList`, because Win64 is
the one platform this release was actually built and tested on. Add a platform to
that list and rebuild if you target it yourself — the source ships with the plugin.

---

## 2. Installation

### Option A — Per-project (recommended)

1. Copy the `SignalNexus` folder into your project's `Plugins/` directory:
   `YourProject/Plugins/SignalNexus/`.
2. Right-click your `.uproject` → **Generate Visual Studio project files**.
3. Open the project. When prompted to rebuild missing modules, choose **Yes**
   (or build `YourProjectEditor` from your IDE).
4. Enable the plugin under **Edit → Plugins → Code Plugins → SignalNexus** if it is
   not already enabled, then restart the editor.

### Option B — Engine-wide

Copy `SignalNexus` into `Engine/Plugins/Marketplace/` (or the Fab install location)
so it is available to every project on that engine install.

### Verifying the install

Open the **Output Log** and confirm you see the module load line under the
`LogSignalNexus` category on startup. If the category never appears, the module did
not load — see [Troubleshooting](#12-troubleshooting).

---

## 3. Core Concepts

- **Channel** — a `FGameplayTag` that names the topic of a signal, e.g.
  `Event.Player.Died`. Channels are **hierarchical**: a subscription on `Event.Player`
  also receives `Event.Player.Died`, `Event.Player.Spawned`, and any deeper child.
- **Payload** — the data carried by a signal, wrapped in `FSignalNexusPayload`. It can
  hold **any** reflected struct (C++ or Blueprint-only) and stays type-safe.
- **Subscription** — a callback bound to a channel. Identified by an opaque
  `FSignalNexusHandle` you keep in order to unsubscribe.
- **Interceptor** — optional middleware registered on a channel subtree that can
  modify, pass, or block a signal before subscribers see it.
- **Priority / scheduling** — how a broadcast is delivered: `Immediate`,
  `DeferredNextFrame`, or `Throttled`.

Everything is hosted by `USignalNexusSubsystem`, a **GameInstance subsystem** — one bus
per game instance, alive for the whole session and available from any world context.

---

## 4. Quick Start — Blueprints

No C++ required. A designer can send and receive any struct — including
Blueprint-only structs — with zero registration.

1. **Broadcast Signal** — set the `Channel` tag, connect **any** struct to the
   wildcard `Value` pin, and (optionally, under *Advanced*) pick a `Priority` and
   `ThrottleRate`.
2. **Subscribe To Signal** — set the `Channel` tag and bind an event to `OnSignal`.
   It fires with the exact `Channel` and a `Payload`. Keep the returned **handle**.
3. **Unpack Signal Payload** — feed the received `Payload` in and read your struct out
   of the wildcard `OutValue` pin. The `Return Value` is `false` if the stored type
   does not match the connected struct — check it before using the output.
4. **Unsubscribe From Signal** — pass the stored handle when you no longer need the
   subscription. (Subscriptions bound to an actor are also pruned automatically when
   that object is destroyed.)

Helper pure nodes: **Is Payload Valid**, **Get Payload Type Name**,
**Is Valid Signal Handle**.

---

## 5. Quick Start — C++

```cpp
#include "SignalNexusSubsystem.h"
#include "SignalNexusTypes.h"

void AMyActor::BeginPlay()
{
    Super::BeginPlay();

    USignalNexusSubsystem* Bus = USignalNexusSubsystem::Get(this);
    if (!Bus) { return; }

    // Subscribe — the callback only fires for signals carrying FSignalDamagePayload.
    DamageHandle = Bus->Subscribe<FSignalDamagePayload>(
        FGameplayTag::RequestGameplayTag(TEXT("Event.Combat.Damage")),
        [this](const FSignalDamagePayload& Dmg)
        {
            UE_LOG(LogTemp, Log, TEXT("Took %.1f damage"), Dmg.Amount);
        });
}

void AMyActor::DealDamage()
{
    if (USignalNexusSubsystem* Bus = USignalNexusSubsystem::Get(this))
    {
        FSignalDamagePayload Dmg;
        Dmg.Amount = 25.f;
        Dmg.Instigator = this;
        Bus->BroadcastSignal(
            FGameplayTag::RequestGameplayTag(TEXT("Event.Combat.Damage")),
            Dmg, ESignalNexusPriority::Immediate);
    }
}

void AMyActor::EndPlay(const EEndPlayReason::Type Reason)
{
    if (USignalNexusSubsystem* Bus = USignalNexusSubsystem::Get(this))
    {
        Bus->Unsubscribe(DamageHandle);
    }
    Super::EndPlay(Reason);
}
```

> **Build.cs** — add `"GameplayTags"` and this module: `PublicDependencyModuleNames.AddRange(new[]{ "SignalNexus", "GameplayTags" });`

---

## 6. API / Class Overview

All public types live in the `SignalNexus` runtime module and are exported with the
`SIGNALNEXUS_API` macro.

### `USignalNexusSubsystem` (`SignalNexusSubsystem.h`)
`UGameInstanceSubsystem` + `FTickableGameObject`. The global hub.

| Member | Description |
|--------|-------------|
| `static USignalNexusSubsystem* Get(const UObject* WorldContextObject)` | Convenience accessor from any world-context object. |
| `template<T> FSignalNexusHandle Subscribe(FGameplayTag Channel, TFunction<void(const T&)> Callback)` | Type-safe subscribe; callback fires only for payloads of type `T`. |
| `template<T> void BroadcastSignal(FGameplayTag Channel, const T& Payload, ESignalNexusPriority = Immediate, float ThrottleRate = 30.f)` | Pack a typed struct and broadcast it. |
| `void Unsubscribe(FSignalNexusHandle Handle)` | Remove a subscription. |
| `void RegisterInterceptor(FGameplayTag, TScriptInterface<ISignalNexusInterceptor>)` | *(BlueprintCallable)* Add middleware on a channel subtree. |
| `void UnregisterInterceptor(FGameplayTag, TScriptInterface<ISignalNexusInterceptor>)` | *(BlueprintCallable)* Remove middleware. |
| `FSignalNexusRouter& GetRouter()` | Direct access to the routing core for advanced callers. |

### `USignalNexusBlueprintLibrary` (`SignalNexusBlueprintLibrary.h`)
The Blueprint face. `Broadcast Signal` and `Unpack Signal Payload` are **custom-thunk
wildcard** nodes.

| Node | Description |
|------|-------------|
| **Broadcast Signal** | Broadcast any struct (wildcard `Value` pin) on a channel. |
| **Subscribe To Signal** | Bind `FSignalNexusReceivedDelegate`; returns a handle. Auto-removed when the bound object dies. |
| **Unpack Signal Payload** | Extract a payload into a wildcard `OutValue` struct; returns `false` on type mismatch. |
| **Unsubscribe From Signal** | Remove a subscription by handle. |
| **Is Payload Valid / Get Payload Type Name / Is Valid Signal Handle** | Pure helpers. |

### `FSignalNexusPayload` (`SignalNexusPayload.h`)
Generic, self-describing, value-semantic container for one reflected struct.

| Member | Description |
|--------|-------------|
| `template<T> void Pack(const T&)` | Store a typed struct. |
| `template<T> bool Unpack(T& Out) const` | Extract into `T`; `false` (+ warning) on type mismatch. |
| `void PackRaw(const UScriptStruct*, const void*)` / `bool UnpackRaw(const UScriptStruct*, void*) const` | Reflection-based pack/unpack (used by thunks). |
| `bool IsValid() const` · `const UScriptStruct* GetStructType() const` · `FString GetStructName() const` · `template<T> bool IsA() const` · `void Reset()` | Introspection & lifecycle. |

Correct value semantics (deep copy, proper destruction, GC of contained `UObject`
references) — safe to copy, move, queue and pass across Blueprint pins.

### `FSignalNexusRouter` (`SignalNexusRouter.h`)
The pure, UObject-free routing core (unit-testable in isolation). Holds subscriptions,
interceptors and scheduling queues; performs hierarchical delivery up the tag parent
chain. The subsystem forwards to it and drives its `Tick`.

### `ISignalNexusInterceptor` / `USignalNexusLambdaInterceptor` (`SignalNexusInterceptor.h`)
Middleware interface (`BlueprintNativeEvent Intercept`) plus a ready-made
lambda-driven implementation for C++/tests.

### Types (`SignalNexusTypes.h`)
- `enum ESignalNexusPriority { Immediate, DeferredNextFrame, Throttled }`
- `enum EInterceptorResult { Pass, Block }`
- `struct FSignalNexusHandle` — opaque subscription id.
- Ready-made payloads: `FSignalIntPayload`, `FSignalNamePayload`,
  `FSignalVectorPayload`, `FSignalDamagePayload`.

### Logging
All plugin logging uses the `LogSignalNexus` category (`SignalNexusLog.h`) with proper
Warning/Error levels — no `UE_LOG(LogTemp, …)` in shipping code paths.

---

## 7. Code Examples

### 7.1 Hierarchical routing

```cpp
USignalNexusSubsystem* Bus = USignalNexusSubsystem::Get(this);

// Subscribe to the parent channel...
Bus->Subscribe<FSignalNamePayload>(
    FGameplayTag::RequestGameplayTag(TEXT("Event.Player")),
    [](const FSignalNamePayload& P){ /* fires for ALL Event.Player.* */ });

// ...a broadcast on a child still reaches it:
Bus->BroadcastSignal(
    FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Died")),
    FSignalNamePayload(TEXT("Hero")));
```

### 7.2 Deferred (next-frame) broadcast

```cpp
// Queued now, delivered on the next world tick — avoids re-entrancy during iteration.
Bus->BroadcastSignal(MyTag, FSignalIntPayload(42),
    ESignalNexusPriority::DeferredNextFrame);
```

### 7.3 Throttled broadcast (noisy UI updates)

```cpp
// At most 10 dispatches/second on this channel; extras coalesce to the latest value.
Bus->BroadcastSignal(HealthBarTag, FSignalIntPayload(CurrentHealth),
    ESignalNexusPriority::Throttled, /*ThrottleRate=*/10.f);
```

### 7.4 A C++ interceptor that halves damage

```cpp
#include "SignalNexusInterceptor.h"

USignalNexusLambdaInterceptor* Clamp = NewObject<USignalNexusLambdaInterceptor>(this);
Clamp->InterceptFn = [](FGameplayTag /*Channel*/, FSignalNexusPayload& Payload)
{
    FSignalDamagePayload Dmg;
    if (Payload.Unpack(Dmg))
    {
        Dmg.Amount *= 0.5f;      // modify in place
        Payload.Pack(Dmg);       // write it back
    }
    return EInterceptorResult::Pass; // or EInterceptorResult::Block to drop it
};

USignalNexusSubsystem::Get(this)->RegisterInterceptor(
    FGameplayTag::RequestGameplayTag(TEXT("Event.Combat.Damage")), Clamp);
```

### 7.5 Blueprint-only struct, end to end

Define `S_QuestUpdate` in a Blueprint, then:
- **Broadcast Signal** → `Channel = Event.Quest.Updated`, wire an `S_QuestUpdate` into `Value`.
- **Subscribe To Signal** → `Channel = Event.Quest` → **Unpack Signal Payload** with an
  `S_QuestUpdate` on the `OutValue` pin. No C++ touched.

---

## 8. Scheduling Modes

| Mode | Behaviour | Use for |
|------|-----------|---------|
| `Immediate` | Synchronous, on the calling thread, right now. | Most gameplay events. |
| `DeferredNextFrame` | Queued, flushed on the next world tick by the subsystem's tickable. | Avoiding re-entrancy while iterating; batching. |
| `Throttled` | Rate-limited per channel; coalesces to at most one dispatch per interval (`ThrottleRate` = max dispatches/second, default 30). | High-frequency signals (UI/HUD refresh). |

The subsystem is tickable even when the game is paused (`IsTickableWhenPaused == true`)
and not tickable in the editor (`IsTickableInEditor == false`).

---

## 9. Interceptors (Middleware)

Register any `UObject` that implements `ISignalNexusInterceptor` on a channel. Matching
is **hierarchical**: an interceptor on `Gameplay.Damage` also inspects
`Gameplay.Damage.Physical`. For each matching signal it may:

- **modify** the payload in place (visible to later interceptors and all subscribers),
- return **`Pass`** to let it continue, or
- return **`Block`** to drop it entirely (subscribers never see it).

Implement it in Blueprints (`Intercept` is a `BlueprintNativeEvent`) or in C++ via
`USignalNexusLambdaInterceptor` for lightweight code-side middleware.

---

## 10. Type Safety & Payloads

`FSignalNexusPayload` stores a live copy of a struct tagged with its `UScriptStruct`.
Unpacking as the **wrong** type never reinterprets memory — it fails cleanly, returns
`false`, logs a `LogSignalNexus` warning and leaves the destination untouched. The
typed `Subscribe<T>` API silently ignores signals whose payload is not a `T`, so a
subscriber only ever sees the type it asked for.

Four ready-made payloads cover common cases with zero boilerplate:
`FSignalIntPayload`, `FSignalNamePayload`, `FSignalVectorPayload`,
`FSignalDamagePayload`. For anything else, just broadcast your own struct.

---

## 11. Performance

- Routing is a `TMap<FGameplayTag, …>` lookup per level of the channel's parent chain —
  no per-frame allocation on the `Immediate` path beyond the payload copy.
- The `Throttled` mode coalesces bursts so extremely frequent producers (e.g. per-frame
  UI updates) collapse to one delivery per interval.
- The plugin ships an **automation test suite** (`Private/Tests/SignalNexusTests.cpp`)
  covering hierarchical routing, type-safety, deferred/throttled delivery, interceptor
  pass/block/modify, and a throughput benchmark. Run it via **Session Frontend →
  Automation → SignalNexus** or:
  ```
  UnrealEditor-Cmd.exe YourProject.uproject -ExecCmds="Automation RunTests SignalNexus; Quit" -unattended -nop4 -nosplash
  ```

---

## 12. Troubleshooting

| Symptom | Likely cause / fix |
|---------|--------------------|
| `Get()` returns null | Called before the GameInstance exists, or from an object with no world context. Call from/after `BeginPlay`. |
| Subscriber never fires | Channel mismatch (check the tag spelling), or the payload type doesn't match the subscribed `T` / unpacked struct. Use **Get Payload Type Name** to inspect. |
| Blueprint wildcard pin shows an error | Reconnect the struct to the `Value`/`OutValue` pin so the node can resolve the type. |
| Throttled signals feel laggy | `ThrottleRate` is dispatches **per second** — raise it for snappier updates. |
| No `LogSignalNexus` output at all | Module not loaded — confirm the plugin is enabled and the project rebuilt. |

---

## 13. Support

- **Author:** Silvan Teufel
- **Support:** teufelsilvan@gmail.com
- **Docs:** https://github.com/SimulatedFlow

© 2026 Silvan Teufel. All Rights Reserved.

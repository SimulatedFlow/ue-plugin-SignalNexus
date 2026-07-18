# SignalNexus — Advanced Runtime Signal Bus

A high-performance, fully decoupled signal & message broker for Unreal Engine 5.8, built on
hierarchical **Gameplay Tags**. Systems, actors and UI communicate without ever referencing each
other — send a signal on a tag channel, subscribe wherever you like.

## Highlights

- **Typed wildcard payloads.** Broadcast and receive *any* struct — including Blueprint-only
  structs — with zero C++ registration. Custom-thunk nodes (`Broadcast Signal`, `Unpack Signal
  Payload`) keep it type-safe: unpacking the wrong type fails cleanly instead of corrupting memory.
- **Hierarchical routing.** A subscription on `Event.Player` automatically receives
  `Event.Player.Spawns`, `Event.Player.Deaths`, and any deeper child channel.
- **Middleware interceptors.** Register an `ISignalNexusInterceptor` on a tag subtree to
  **modify**, **pass**, or **block** signals before they reach subscribers (e.g. clamp damage,
  redirect events, mute channels).
- **Scheduling modes.** `Immediate`, `DeferredNextFrame` (queued and flushed on the next tick),
  and `Throttled` (rate-limited & coalesced per channel — perfect for noisy UI updates).
- **Self-contained.** Depends only on `Core`, `CoreUObject`, `Engine`, `GameplayTags`.

## C++ quick start

```cpp
USignalNexusSubsystem* Bus = USignalNexusSubsystem::Get(this);

// Subscribe (type-safe)
FSignalNexusHandle Handle = Bus->Subscribe<FSignalDamagePayload>(
    DamageTag, [](const FSignalDamagePayload& Dmg) { /* react */ });

// Broadcast
FSignalDamagePayload Dmg; Dmg.Amount = 25.f;
Bus->BroadcastSignal(DamageTag, Dmg, ESignalNexusPriority::Immediate);

// Later
Bus->Unsubscribe(Handle);
```

## Blueprint quick start

1. **Broadcast Signal** — pick a `Channel` tag, connect any struct to the wildcard `Value` pin,
   choose a `Priority`.
2. **Subscribe To Signal** — bind an event; it fires with the `Channel` and a `Payload`.
3. **Unpack Signal Payload** — feed the `Payload` in and read your struct out of the wildcard pin
   (returns `false` if the type doesn't match).

## Modules

- `SignalNexus` (Runtime, `PreDefault`) — payload container, routing core, subsystem, interceptor
  interface, and the Blueprint function library.

© 2026 Simulated Flow

<!-- SF-STORE-BLOCK:BEGIN -->
## 🛒 Source-available — see before you buy

This repository contains the **full source** of a commercial Unreal Engine plugin. It is **source-available, not open source**: read it, evaluate it, then buy a license to use it. See **LICENSE.md (Fab / UE EULA)**.

**Get it / Buy:**
- Listing folgt — Kontakt: simulatedflow@gmail.com

_© 2026 Simulated Flow. All rights reserved._
<!-- SF-STORE-BLOCK:END -->

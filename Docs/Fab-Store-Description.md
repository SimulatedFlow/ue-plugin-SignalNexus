# SignalNexus — Advanced Runtime Signal Bus

## Headline

**Decouple everything. SignalNexus is a high-performance, type-safe message bus for Unreal Engine 5.8 — send and receive any struct across your whole game on Gameplay-Tag channels, from Blueprints or C++, with zero hard references.**

---

## Pitch (1 paragraph)

Stop wiring your systems together with cast chains, singletons and brittle direct
references. SignalNexus gives Unreal Engine a proper **runtime signal bus**: broadcast a
signal on a hierarchical Gameplay-Tag channel and any subscriber — an actor, a widget, a
subsystem, anywhere in the game — reacts, without either side ever knowing the other
exists. Payloads are true wildcards: send **any** struct, including Blueprint-only
structs, with no C++ registration and full type safety (unpacking the wrong type fails
cleanly instead of corrupting memory). Add middleware **interceptors** to modify, pass or
block signals mid-flight, and choose **immediate, next-frame, or throttled** delivery to
keep even the noisiest channels cheap. It depends only on core engine modules, ships with
full source and an automation test suite, and drops into any UE 5.8 project in minutes.

---

## Feature Bullets

- ⚡ **Hierarchical Gameplay-Tag routing** — subscribe to `Event.Player` and receive
  every `Event.Player.*` child automatically.
- 🎯 **Typed wildcard payloads** — broadcast and receive *any* struct (C++ **or**
  Blueprint-only) with zero registration; wrong-type unpacks fail safely, never crash.
- 🧩 **Blueprint-first, C++-powered** — custom-thunk wildcard nodes (`Broadcast Signal`,
  `Unpack Signal Payload`) plus a clean templated C++ API (`Subscribe<T>` /
  `BroadcastSignal<T>`).
- 🛡️ **Middleware interceptors** — register on a tag subtree to modify, pass or block
  signals before subscribers see them (clamp damage, mute channels, redirect events).
- ⏱️ **Three scheduling modes** — `Immediate`, `DeferredNextFrame` (safe, batched), and
  `Throttled` (rate-limited & coalesced for high-frequency UI/HUD updates).
- 🔗 **Zero hard references** — kill cast chains, God-objects and dependency spaghetti.
- 🧹 **Self-cleaning subscriptions** — auto-pruned when the bound object is destroyed.
- 🪶 **Self-contained & lightweight** — depends only on `Core`, `CoreUObject`, `Engine`,
  `GameplayTags`. No heavy AI/StateTree/Navigation modules, no third-party libs.
- ✅ **Ships with full C++ source, an automation test suite, and a throughput benchmark.**

---

## Technical Details

| | |
|---|---|
| **Engine version** | Unreal Engine 5.8 |
| **Type** | Code Plugin (C++), usable from Blueprints & C++ |
| **Modules** | `SignalNexus` — Runtime, `LoadingPhase: PreDefault` |
| **Dependencies** | `Core`, `CoreUObject`, `Engine`, `GameplayTags` (no third-party plugins) |
| **Network replicated** | No — client-local signal bus (replicate your own state as usual) |
| **Platforms** | All UE-supported platforms (no platform-specific code, no native libs) |
| **Source** | Full C++ source included |
| **Content** | Optional; the module needs no accompanying content |
| **Number of C++ classes/types** | Subsystem, Blueprint library, router core, payload container, interceptor interface + lambda impl, enums & handle, 4 ready-made payload structs |

**Key classes:** `USignalNexusSubsystem`, `USignalNexusBlueprintLibrary`,
`FSignalNexusPayload`, `FSignalNexusRouter`, `ISignalNexusInterceptor`,
`USignalNexusLambdaInterceptor`, `ESignalNexusPriority`, `EInterceptorResult`,
`FSignalNexusHandle`.

---

## Target Audience

- **Gameplay & systems programmers** who want an event bus without hand-rolling
  delegates and singletons for every system.
- **Technical designers** who need to fire and handle events in Blueprints — including
  passing custom structs — without touching C++.
- **UI/UX engineers** wiring HUDs and menus to gameplay without coupling widgets to
  actors (throttled channels keep it cheap).
- **Teams** building modular, plugin-based projects where systems must stay decoupled
  and independently testable.
- Suitable for any genre: RPG, shooter, strategy, simulation, multiplayer or single-player.

---

## Price Idea

**€49** (one-time, per seat) — within Fab self-serve limits.

- Positioned as a professional, source-included gameplay-framework utility that saves
  days of architecture work and prevents costly coupling debt.
- Optional launch discount: **€39** for the first two weeks to seed reviews.

---

## Suggested Store Tags / Keywords

`event system`, `message bus`, `signal`, `gameplay tags`, `decoupling`, `observer`,
`pub/sub`, `messaging`, `middleware`, `blueprint`, `communication`, `architecture`,
`framework`, `utility`.

---

© 2026 Simulated Flow

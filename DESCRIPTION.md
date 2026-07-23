# SignalNexus — Advanced Runtime Signal Bus

Decouple everything. SignalNexus is a high-performance, type-safe message bus for Unreal Engine 5.8 - send and receive any struct across your whole game on Gameplay-Tag channels, from Blueprints or C++, with zero hard references.

Stop wiring your systems together with cast chains, singletons and brittle direct references. SignalNexus gives Unreal a proper runtime signal bus: broadcast a signal on a hierarchical Gameplay-Tag channel and any subscriber - actor, widget, subsystem, anywhere - reacts, without either side knowing the other exists. Payloads are true wildcards: send any struct (including Blueprint-only structs) with no C++ registration and full type safety. Add middleware interceptors to modify, pass or block signals mid-flight, and choose immediate, next-frame, or throttled delivery.

KEY FEATURES

- Hierarchical Gameplay-Tag routing - subscribe to Event.Player and receive every Event.Player.* child automatically.
- Typed wildcard payloads - broadcast and receive any struct (C++ or Blueprint-only) with zero registration; wrong-type unpacks fail safely, never crash.
- Blueprint-first, C++-powered - custom-thunk wildcard nodes plus a clean templated C++ API (Subscribe/BroadcastSignal).
- Middleware interceptors - modify, pass or block signals before subscribers see them.
- Three scheduling modes - Immediate, DeferredNextFrame, and Throttled (rate-limited and coalesced).
- Zero hard references, self-cleaning subscriptions, engine-only dependencies, full C++ source + automation tests.

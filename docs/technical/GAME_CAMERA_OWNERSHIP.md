# Game Camera Ownership â€” implementation and limits

This is a new independent ownership restriction in the v1.15 main build. It adds
no saved setting, hotkey, menu or companion program. Existing comfort and
movement policies remain in place.

## Implemented spectator integration

The reviewed source contract is ReSkate 1.1.8's
`Engine/Game/Build/20260929/native_party.h`: `spectate_active` at game RVA
`0x7572a0`, with signature `bool()`. Its 32-byte native prefix is reproduced
in `src/game_camera_ownership.cpp`. Bootstrap verifies the supported game
hash. The observer additionally requires the exact compiled ReSkate 1.1.8
profile and either the native prefix or a branch resolved to that stock DLL.
Automatic bridge compatibility does not enable this new native contract.

The observer calls the native entry so ReSkate's existing
`native_party_hooks.cpp::spectate_active_hook` remains involved. That hook
returns true for its selected remote-party spectator as well as the original
native spectator state. `source_spawn_camera.cpp::update_party_camera` confirms
this state uses a camera lease and must have first person released before
acquiring its camera. No new native detour is installed by this feature.

The query runs in the existing mod tick before forwarding to ReSkate's client
tick. A positive spectator signal inhibits custom camera, FOV, input capture
and facing overrides and queues the established first-person disable request.
The native queue continues to own camera/head cleanup. Temporary Spring
requests use the saved preference while ownership is suspended. Both entry
and return clear obsolete custom blends and invoke existing input re-entry
cleanup.

Return requires a negative spectator query plus the existing guarded camera
snapshot: mode zero, view enabled, input helper inactive, and selected camera
of the verified gameplay camera type in the current context. A changed pointer,
missing sample, movement change or input lock never creates a takeover.
Unknown evidence during an established takeover cannot clear its restriction.

All activation paths are gated. The master preference stays independent, and
return removes only ownership suppression. Existing bail/board restrictions
and camera readiness still control restoration. Explicit disable and existing
Freecam/Noclip cancellation remain effective.

## Unimplemented contracts

**Replay pre-entry release is not implemented.** The supplied replay source
describes video export/encoder handling, not a verified transition before
editor initialisation. Detecting a changed camera afterward would not meet the
requirement. No speculative hook or controller shortcut was added.

General scripted cameras and mode-specific takeovers are not independently
identified. Modes using the reviewed spectator signal are handled. The user
reported successful Game of Skate handoff testing; other modes still need
live confirmation. No movement/turn heuristic substitutes for missing contracts.
This build is therefore a partial implementation of the requested system.

## Changed source groups

- `game_camera_ownership.h/.cpp`: independent policy, observer and diagnostics.
- Runtime/hooks and comfort integration: observe before the stock tick, preserve
  master intent, request suspension/restoration through the stock queue.
- Stock bridge/feed and stick runtime: gate activation and overrides, restore
  temporary Spring settings and clear blends through existing code.
- Existing panel: ownership status text and build version; no new controls.
- CMake and `game_camera_ownership_tests.cpp`: policy regression target.

## Validation and remaining acceptance

The build script compiles the DLL and runs the full CTest suite, including
ownership policy cases for normal gameplay, missing evidence, context
replacement, takeover/return, manual disable and overlapping bail/board rules.
These tests validate policy, not real native camera timing or editor entry.

Live tests still required: spectator selection and switching, spectator exit,
held input on return, overlapping comfort restrictions, manual disable,
Freecam/Noclip, head visibility, Spring restoration and normal gameplay.
Game of Skate has user-reported successful testing. Additional turn scenarios
and other modes still need acceptance testing. Replay and scripted-camera requirements remain
open pending verified contracts.

Built DLL: `release/scripts/Custom/JishsFirstPersonMod.dll`.

## Promotion to main v1.15

The user reported successful Game of Skate testing on the experimental build.
The same ownership implementation is now included in main v1.15. This report
does not verify replay, scripted cameras or every multiplayer mode. The config
panel version comes from the CMake project version.

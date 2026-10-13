# JishsFirstPersonMod 1.19 for ReSkate 2.0.1

This source update is based on the working 1.15 / ReSkate 1.1.8 archive, not on the changed camera pipeline in 1.18. Existing settings remain compatible. No new saved settings are required.

## Upstream target

Official repository: https://github.com/Dingo-Shenanigans/ReSkate

Target: release tag `v2.0.1`, commit `4d77b859bcf7711bb778e50085d77bb2e76f7dbb`, verified on 2026-10-09. Unreleased main-branch changes are not the target.

Official DLL SHA-256: `abf77e2b21d133a8a7f635d601a0eff60feb2327f2c0e97bd7f38267552f3512`.

Exact reviewed profiles for 2.0.0 and 2.0.1 are included, alongside the existing legacy profiles. Unknown 2.x layouts are not assumed compatible. Modified DLLs can fail the compatibility checks.

## Findings from comparing 1.15 with 1.18

These are source-level regressions and behavior changes. They explain likely failures but do not replace reproducing symptoms in game.

| Area | Change in 1.18 | Result in this update |
| --- | --- | --- |
| Bail view | Held world orientation became a relative rotation composed with the moving native camera | Restore 1.15 held-view math; native flips do not rotate the held view |
| Head attachment | Native camera position replaced the calibrated animation-head position | Restore direct head attachment and its calibrated offset |
| On-foot stick look | New `NativeAim` adds displacement to native orientation and subtracts body turns | Restore 1.15 absolute stick aim and original character-turn handling |
| Controller first-person toggle | Mod toggle disabled whenever native controls were present | Keep the saved mod toggle; pause only bindings overlapping a native action |
| Board-only preference | Saved mod option ignored whenever native controls were active | Honor both options: either board-only setting restricts first person |
| Binding conflicts | Native controls need separate keyboard/controller encoding and per-action routing | Preserve unaffected analog/glance/toggle actions; reject new conflicting assignments and retain existing saved values |
| Native camera processing | Leaving native flips and stabilization ahead of the mod's head-lock camera creates competing transformations | Temporarily mask the native sampler during mod head locks |

The raw changes across all files are recorded in `SOURCE_COMPARISON.md`. Vendor code remains the working baseline's version; copying all upstream headers into this mod would change assumptions about the original camera math.

## Native features alongside the mod

**First-person hotkey:** The native hotkey still works. Its queued `firstperson toggle` (or `debug.first_person toggle`) command toggles the mod's master preference. This matters when a bail, board-only restriction or spectator camera temporarily displays third person: native code otherwise toggles the currently displayed view instead of the retained master preference. Explicit native enable/disable and unrelated console commands retain their existing handling.

The mod's own controller toggle also remains usable when its binding does not overlap a native controller action. A conflicting saved binding is retained but temporarily inactive. Other nonconflicting mod actions continue working. Tagged native keyboard chords are not imported as controller button masks. If ReSkate claims the panel's keyboard key, the mod panel remains accessible through ReSkate's Insert menu.

**True first person / stabilization and Follow flips:** Native preferences stay in ReSkate. During a mod-controlled head lock, the native sampler receives a local copy with Spring, stabilization and Follow flips disabled. The mod then applies the original camera behavior. This includes parkour head locks and the mod's held bail view when applicable. Outside those head locks, the original native sampler receives its original settings, so native options work normally.

This coordination does not write temporary values into the shared native settings or profile database. User changes to camera settings still synchronize through the existing explicit settings bridge. Mod look/handoff smoothing retains the working baseline's rules.

**Board-only:** Either the mod's saved board-only option or ReSkate's option restricts the view to riding. Both saved choices are retained. Bail comfort and verified spectator takeovers preserve the master preference and restore the desired view when their restrictions clear.

## Build and install

Use Windows x64 with Visual Studio C++ tools, MASM, CMake and the Windows SDK. The supplied `build.ps1` selects the Visual Studio 18 2026 generator; use that version or adjust the generator for your installed Visual Studio.

From this source directory:

```powershell
.\build.ps1
.\package.ps1
```

The build script runs CTest before staging the release. The packaging script produces runtime and source ZIPs named for mod 1.19 / ReSkate 2.0.1.

Close Skate before installation. Place the generated DLL and `jishs_first_person_mod.lua` together under `scripts/Custom/` in the ReSkate installation. Retain the official `reskate.dll`. Remove any old mod DLL beside Skate.exe. Existing `%LOCALAPPDATA%\JishsFirstPersonMod` preferences and logs are retained.

## Validation completed here

- Official 2.0.1 DLL/source compared with official 2.0.0 DLL/source using the hook inspector: all eight bridge addresses matched the compiled profiles; source layouts and executable/data regions checked.
- Native sampler and console callback prologues checked against both official DLLs (`tools/verify_native_interop.py` provides a reproducible read-only check). The sampler's MSVC x64 hidden-return calling convention was inspected at its callers.
- Seven portable C++ programs passed: native interop, bail view, camera transitions, look smoothing, hippy jump, game camera ownership and proven camera behavior.
- Six additional C++ programs passed using a temporary Linux WinAPI shim for file rename and discarding unused Windows worker code: binding conflicts, stock contract, Spring policy, stick look, comfort settings and config validation. These verify the real mod logic and persistence, but do not exercise Windows background threads.
- Four Python inspector regression tests passed. Profile import passed, including native keyboard bindings and checking the source SQLite database remains unchanged.
- Additional changed Windows translation units were syntax-checked with an isolated MinGW cross compiler. The baseline's MSVC SEH construct used a syntax-only stand-in for these checks; this does not validate actual exception handling or substitute for an MSVC build.

No Windows DLL was built here and no live game was available. Full CTest, Detours attachment, native activation timing and in-game camera behavior still need validation on Windows. Historical successful tests described elsewhere in this archive apply to prior releases.

## In-game acceptance checks

1. Enable native stabilization and Follow flips. With mod head locks disabled, confirm native behavior still works.
2. Enable the mod's parkour head locks. Test climbing, mounting, on-foot jumps and hippy jumps with parkour looking both off and on. Confirm native flips do not compete with the locked view.
3. Test bails with third-person-while-bailing both off and on; held first-person bail orientation should follow the 1.15 behavior.
4. Test walking, riding, mounting and dismounting with each board-only option separately, then both. Confirm original transition smoothing and right-stick re-entry behavior.
5. Test right-stick look, glances and Camera and character turning. Check the camera does not accumulate native-relative rotation.
6. Toggle using the native hotkey and a distinct mod controller binding, including while a comfort restriction temporarily displays third person. A master toggle off must not re-enable first person on recovery.
7. Deliberately overlap one native action with one mod binding. Confirm only that mod action pauses and the saved binding remains intact.
8. Change native options during normal movement and a head lock; restart the game and confirm saved values remain as chosen.
9. Test verified spectator/Game of Skate ownership release and restoration. Replay pre-entry and general scripted camera detection remain baseline limitations.

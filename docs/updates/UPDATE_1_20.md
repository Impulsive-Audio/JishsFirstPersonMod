# Version 1.20 / ReSkate 2.0.2

Built from the workspace 1.19 source, including its launcher, exact-combo and recording/preset fixes. Existing preferences are retained.

## Compatibility changes

Official tag v2.0.2, source commit 8452f88. DLL SHA-256: `082de47e7ce36f34ab395feee5a52f77531bdb5f73f0605c5a4f0ad17f94214e`.

The source-layout inspector verified all eight bridge addresses against 2.0.1. Relevant Skater, Overlay and game-build C++ source files are unchanged. Regenerated bounded bridge recognizers include the new release. Native sampler RVA is 0x273020; console callback RVA is 0x3ec00. The console callback was traced through StartV3 registration, not selected by prologue alone. Both sampler call destinations and hook prologues were verified. The native Binds recording reader is at 0x3eb480; its unique prologue and two menu-enabled callers were verified. Earlier verified profiles are retained.

## Install

Close Skate. Extract the runtime ZIP into the ReSkate folder containing Skate.exe and ReSkate.dll, merging scripts and Mods. Keep the official ReSkate.dll. Enable the mod in the launcher before starting. Remove any old mod DLL beside Skate.exe. Settings remain in %LOCALAPPDATA%/JishsFirstPersonMod.

## Validation

Windows x64 Release DLL built with MSVC 19.51. All 22 CTest checks passed. Four Python inspector tests passed. Native sampler/console and recording-reader binary checks passed. The C++ resolver passed compiled/forced recognition and rejection checks against all seven official DLL fixtures from 1.1.5 through 2.0.2. Live in-game testing is still required: native and mod toggles, glances, on-foot look, mounting/dismounting, bails, native stabilization/Follow flips, launcher enable/disable and spectator camera handoff. Replay pre-entry and general scripted-camera detection remain existing limitations.

# Jish's First Person Mod 1.23

First-person camera controls and comfort options for ReSkate, updated for **ReSkate 2.0.5**. Loads through the standard ReSkate launcher using the included DirectInput proxy.

## Install

Download `JishsFirstPersonMod-1.23-reskate-2.0.5.zip` from [Releases](https://github.com/Jish2637/JishsFirstPersonMod/releases/latest). Close Skate, then extract **Mods**, **scripts**, **dinput8.dll**, and **LICENSE.txt** into the ReSkate folder beside Skate.exe and ReSkateLauncher.exe.

Enable Jish's First Person Mod in the standard launcher. Press **Insert** in-game for settings. Existing preferences are preserved. See [installation details](LAUNCHER_INSTALL.md).

## Features

- Custom controller glance bindings and right-stick look on foot.
- Camera/head attachment options for mounting, parkour and hippy jumps.
- Foot/board transition smoothing and independent look smoothing.
- Optional third person while bailing and first person on board only.
- Coordination with native stabilization, Follow flips and spectator camera ownership.
- Saved settings, binding recording and a recommended preset.

## Compatibility mode

Enable **Compatibility mode (look controls only)** in the mod panel to retain right-stick camera look, directional glances, the First Person checkbox and hotkey, and both comfort options. Camera position, FOV and Spring remain controlled by ReSkate; head attachment, bail camera holds, foot/board transition smoothing and character turning are disabled. Your saved settings are preserved when switching back to full mode.

The compatibility update passed all 24 Windows tests and was confirmed working in-game. See [compatibility release notes](UPDATE_1_21_COMPATIBILITY.md).

## Recommended controls

Load Jish's recommended preset through the Insert panel to apply these bindings:

- LB/L1 + D-pad direction: glance in that direction; release to return.
- LB/L1 + RB/R1 + D-pad Down: toggle first person.
- Insert: open settings.
- F9: hide or show the mod panel.

Bindings and the panel shortcut can be changed. See [detailed settings](SETTINGS.md).

## Diagnostics

Settings and logs are under `%LOCALAPPDATA%/JishsFirstPersonMod`. Bail trace files rotate at 8 MiB; mod and proxy text logs rotate at 1 MiB. Each keeps three older files, with `.1` the newest archive. Logging continues across rotations. Approximate retained budget is 40 MiB; a blocked archive can temporarily exceed it while rotation retries.

## Build and validation

Windows x64, Visual Studio C++ tools and CMake are required. Run `./build.ps1`, then `./package.ps1`. The release install ZIP contains only Mods, scripts, dinput8.dll and LICENSE.txt.

All 25 Windows tests, ten release-fixture bridge checks and 31 game-offset checks pass. The 2.0.5 bridge addresses and layouts are verified against the official tagged source and exact release DLL. The final 1.23 startup-fix build initialized successfully on 2.0.5; logs confirm camera/input hooks, menu binding, native recording and controller First Person activation. Replay pre-entry and general scripted-camera detection remain existing limitations; successful attachment alone does not establish every multiplayer or movement transition.

See [2.0.5 update notes](UPDATE_1_23.md), [2.0.5 offset audit](OFFSET_AUDIT_2_0_5.md), [2.0.4 notes](UPDATE_1_22.md), [previous release notes](UPDATE_1_21.md), and [historical notes](HISTORY.md).

## License

See [LICENSE.txt](vendor/LICENSE) and dependency license files under vendor/External. The install package contains the project's LICENSE.txt.

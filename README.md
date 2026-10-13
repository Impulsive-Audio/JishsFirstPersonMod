# Jish's First Person Mod 1.29

First-person camera controls and comfort options for ReSkate, updated for **ReSkate 2.0.7**. Loads through the standard ReSkate launcher using the included DirectInput proxy.

Adds named personal presets and a clearer General, Controls and Camera menu while retaining the ReSkate styling. See [1.29 release notes](docs/updates/UPDATE_1_29.md).

Includes glance emote blocking using existing ReSkate/mod handlers. Hold LB/L1 and tap a bound D-pad direction to glance; double tap within 250 ms to emote, and hold the second tap for a held emote. Toggle bindings using LB/L1 + D-pad also block accidental emotes. See [1.28 release notes](docs/updates/UPDATE_1_28.md).

Native DualSense and DualSense Edge controllers now use the same gesture-blocking policy as Xbox, including USB with Steam Input disabled. HID completion adapters cover large read buffers and batched reports alongside DirectInput state/event adapters. Your independent binding reader stays available. Technical validation is recorded in the source test notes.

## Install

Download `JishsFirstPersonMod-1.29-reskate-2.0.7.zip` for ReSkate 2.0.7. Published packages are available from [Releases](https://github.com/Jish2637/JishsFirstPersonMod/releases/latest). Close Skate, then extract **Mods**, **scripts**, **dinput8.dll**, and **LICENSE.txt** into the ReSkate folder beside Skate.exe and ReSkateLauncher.exe.

Enable Jish's First Person Mod in the standard launcher. Press **Insert** in-game for settings. Existing preferences are preserved. See [installation details](LAUNCHER_INSTALL.md).

## Features

- Custom controller glance bindings and right-stick look on foot.
- Optional Toggle glance controls: press to hold a direction, press again to return; another direction replaces it.
- Camera/head attachment options for mounting, parkour and hippy jumps.
- To see the effect of the jump attachment switches, turn Disable Spring on foot off. With it enabled, camera Spring is already disabled during off-board movement, so attachment changes may be less noticeable.
- Foot/board transition smoothing and independent look smoothing.
- Optional third person while bailing and first person on board only. In full mode with third-person bailing off, first-person bails follow the headÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢s full rotation and position; right-stick camera look resumes when native movement control is verified.
- Optional third person while getting up: keep the fall in first person, then return to first person when control resumes. This option defaults off and is available with Third person while bailing off.
- Bald in first person keeps long hair out of view locally; other players see your original hairstyle.
- Camera collision during first-person bails (**WIP**). The camera can still stick near floors and objects; third person while getting up provides an alternative.
- Coordination with native stabilization, Follow flips and spectator camera ownership.
- Named personal presets for settings and bindings, plus Jish’s recommended preset.
- Simple General, Controls and Camera tabs with persistent save feedback.
- Short menu explanations with (?) hover help, including disabled options.

## Compatibility mode

Enable **Compatibility mode (look controls only)** in the mod panel to retain right-stick camera look, directional glances, the First Person checkbox and hotkey, and the bail/board comfort options. Third person while getting up and bail collision require full mode. Camera position, FOV and Spring remain controlled by ReSkate; head attachment, bail camera holds, foot/board transition smoothing and character turning are disabled. Your saved settings are preserved when switching back to full mode.

The compatibility update passed all 24 Windows tests and was confirmed working in-game. See [compatibility release notes](docs/updates/UPDATE_1_21_COMPATIBILITY.md).

## Recommended controls

Load Jish's recommended preset through the Insert panel to apply these bindings:

- LB/L1 + D-pad direction: glance in that direction; release to return.
- LB/L1 + RB/R1 + D-pad Down: toggle first person.
- Insert: open settings.
- F9: hide or show the mod panel.

Bindings and the panel shortcut can be changed. See [detailed settings](SETTINGS.md).

## Diagnostics

Settings and logs are under `%LOCALAPPDATA%/JishsFirstPersonMod`. The current camera trace is `bail-camera-trace-v10.csv`, covering jumps, mounting, hippy jumps, bails and live settings changes. It records why attachment was selected and whether head data/calibration allowed it, alongside native/final camera positions. Bail diagnostics include uncontrolled/recovery phases, verified control flags, calibrated head target rotation and local recovery look. Camera trace files rotate at 8 MiB; mod and proxy text logs rotate at 1 MiB. Each keeps three older files, with `.1` the newest archive. Logging continues across rotations. Approximate retained budget for current log families is 40 MiB; older trace versions are preserved, and a blocked archive can temporarily exceed the budget while rotation retries.

## Build and validation

Windows x64, Visual Studio C++ tools and CMake are required. Run `./build.ps1`, then `./package.ps1`. The release install ZIP contains only Mods, scripts, dinput8.dll and LICENSE.txt.

All 32 Windows tests pass. ReSkate 2.0.7 uses verified native camera, physics query and cosmetic capture contracts. Bail collision remains **work in progress**; the new third-person getting-up option provides an alternative. Technical research and gameplay checklists stay in source, including [bail collision research](docs/technical/BAIL_COLLISION_RESEARCH.md), [local hair checks](docs/testing/TEST_LOCAL_HAIR.md) and [getting-up checks](docs/testing/TEST_THIRD_PERSON_RECOVERY.md).

See [1.28 release notes](docs/updates/UPDATE_1_28.md), [1.26 release notes](docs/updates/UPDATE_1_26.md), [1.25 release notes](docs/updates/UPDATE_1_25.md), [1.24 release notes](docs/updates/UPDATE_1_24.md), [2.0.5 update notes](docs/updates/UPDATE_1_23.md), [2.0.5 offset audit](docs/technical/OFFSET_AUDIT_2_0_5.md), [2.0.4 notes](docs/updates/UPDATE_1_22.md), [previous release notes](docs/updates/UPDATE_1_21.md), and [historical notes](docs/updates/HISTORY.md).

## License

See [LICENSE.txt](vendor/LICENSE) and dependency license files under vendor/External. The install package contains the project's LICENSE.txt.

With **Third person while bailing** or **Third person while getting up** enabled on ReSkate 2.0.7, **First Person In Camera Cycle** is automatically disabled. The menu explains this override. Your previous preference is retained across game restarts and restored when both automatic third-person options are off. The mod's First Person checkbox and hotkey remain available.

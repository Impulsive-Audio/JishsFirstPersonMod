# Jish's First Person Mod 1.33.2

Includes modifier-held on-board right-stick free look and a controller-navigable settings panel. Insert opens both settings menus. F9 hides or shows the first-person panel inside Insert. Standalone opening is disabled. See [on-board controls](ON_BOARD_FREE_LOOK.md) and [menu controls](CONTROLLER_MENU.md).

First-person camera controls and comfort options for ReSkate, updated for **ReSkate 2.0.7**. Loads through the standard ReSkate launcher using the included DirectInput proxy.

## Install

Close Skate before extracting a locally built package's **Mods**, **scripts**, **dinput8.dll**, and **LICENSE.txt** beside Skate.exe and ReSkateLauncher.exe.

Enable Jish's First Person Mod in the standard launcher. Press **Insert** in-game for settings. Existing preferences are preserved. See [installation details](LAUNCHER_INSTALL.md).

## Features

- Custom controller glance bindings and right-stick look on foot.
- Optional Toggle glance controls: press to hold a direction, press again to return; another direction replaces it.
- Camera/head attachment options for mounting, parkour and hippy jumps.
- To see the effect of the jump attachment switches, turn Disable Spring on foot off. With it enabled, camera Spring is already disabled during off-board movement, so attachment changes may be less noticeable.
- Foot/board transition smoothing and independent look smoothing.
- Optional third person while bailing and first person on board only. In full mode with third-person bailing off, first-person bails follow the headÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢s full rotation and position; right-stick camera look resumes when native movement control is verified.
- Optional third person while getting up: keep the fall in first person, then return to first person when control resumes. This option defaults off and is available with Third person while bailing off.
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
- F9: hide or show the first-person panel inside Insert.

Controller bindings can be changed. The panel uses Insert and F9. See [detailed settings](SETTINGS.md).

## Diagnostics

Settings are under `scripts/Custom/FirstPerson`; logs are in its `logs` subfolder. The current camera trace is `bail-camera-trace-v11.csv`, covering jumps, mounting, hippy jumps, bails and live settings changes. It records why attachment was selected and whether head data/calibration allowed it, alongside native/final camera positions. Bail diagnostics include uncontrolled/recovery phases, verified control flags, calibrated head target rotation and local recovery look. Camera trace files rotate at 8 MiB; mod and proxy text logs rotate at 1 MiB. Each keeps three older files, with `.1` the newest archive. Logging continues across rotations. Approximate retained budget for current log families is 40 MiB; older trace versions are preserved, and a blocked archive can temporarily exceed the budget while rotation retries.

## Build and validation

Windows x64, Visual Studio C++ tools and CMake are required. Run `./build.ps1`, then `./package.ps1`. The release install ZIP contains only Mods, scripts, dinput8.dll and LICENSE.txt.

The combined fork retains upstream tests and adds on-board/menu regressions; see the packaged validation report. ReSkate 2.0.7 uses verified native camera, physics query and spectator contracts. Upstream reports spectator gameplay validation; this combined fork still needs its own gameplay check. Bail collision remains **work in progress**; the new third-person getting-up option provides an alternative. Technical research and gameplay checklists stay in source, including [bail collision research](docs/technical/BAIL_COLLISION_RESEARCH.md) and [getting-up checks](docs/testing/TEST_THIRD_PERSON_RECOVERY.md).

See [1.28 release notes](docs/updates/UPDATE_1_28.md), [1.26 release notes](docs/updates/UPDATE_1_26.md), [1.25 release notes](docs/updates/UPDATE_1_25.md), [1.24 release notes](docs/updates/UPDATE_1_24.md), [2.0.5 update notes](docs/updates/UPDATE_1_23.md), [2.0.5 offset audit](docs/technical/OFFSET_AUDIT_2_0_5.md), [2.0.4 notes](docs/updates/UPDATE_1_22.md), [previous release notes](docs/updates/UPDATE_1_21.md), and [historical notes](docs/updates/HISTORY.md).

## License

See [LICENSE.txt](vendor/LICENSE) and dependency license files under vendor/External. The install package contains the project's LICENSE.txt.

With **Third person while bailing** or **Third person while getting up** enabled on ReSkate 2.0.7, **First Person In Camera Cycle** is automatically disabled. The menu explains this override. Your previous preference is retained across game restarts and restored when both automatic third-person options are off. The mod's First Person checkbox and hotkey remain available.

This 1.32 fork includes the working 1.29.1 Sony HID menu fixes. See [local integration notes](docs/updates/LOCAL_1_30_INTEGRATION.md).

Local test update: button glances and on-foot right-stick smoothing are independent. Radial-menu input changes are deferred.

Version 1.32 fixes spectator startup hook ordering and keeps the spectator preference selectable. Tested working on both affected machines; all 34 automated tests pass. See [spectator validation notes](docs/testing/TEST_SPECTATOR_HOOK_1_32.md).

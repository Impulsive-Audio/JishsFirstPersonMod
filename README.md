# Jish's First Person Mod — Impulsive-Audio fork

Version **2.1.1-1** (mod revision 1 for ReSkate 2.1.1).

First-person camera controls and comfort options for ReSkate, supporting **ReSkate 2.1.1** (2.0.7 retained). Loads through the ReSkate launcher using the included DirectInput proxy. See [on-board controls](ON_BOARD_FREE_LOOK.md) and [menu controls](CONTROLLER_MENU.md).

## Install

ReSkate 2.1.x keeps its own copy of the game in its own folder (for example `C:\Users\admin\Games\ReSkate`) instead of the Steam install. Install this mod into **that** folder:

1. Close Skate — the DirectInput proxy cannot be replaced while it runs.
2. Extract the package's **Mods**, **scripts**, **dinput8.dll** and **LICENSE.txt** beside `Skate.exe`.
3. Enable Jish's First Person Mod in the launcher.

For release packaging the launcher installs the ZIP itself, so no script is needed. See [installation details](LAUNCHER_INSTALL.md).

## Settings

**Insert → Skater → First Person.** The mod's settings are drawn *inside* ReSkate's own First Person page, using the stock widgets, fonts and navigation — mouse, keyboard and controller all work exactly as they do elsewhere in that menu.

The older standalone panel is disabled by default. Set the environment variable `JISH_COMPANION_MENU=1` before launching to restore it; **F9** then shows or hides it inside Insert.

ReSkate's own first-person settings remain on the same page. Both can be enabled; this mod's camera takes precedence while it is active.

## Hotkeys

| Key | Action |
|---|---|
| **F8** | Mod on/off. Off hands the camera back to ReSkate without leaving first person or changing ReSkate's own settings. |
| **F9** | Show/hide the companion panel (companion mode only). |
| **F7** | Debug HUD — dev builds only. |
| **F10** | Start/stop a camera recording — dev builds only. |

## Features

- On-board modifier-held free look and on-foot right-stick look, with custom controller glance bindings and optional toggle glances.
- On-foot camera composition: absolute world-up heading and level pitch, with the character's real head attitude (bob, sway, tilt, roll) riding on top. Adjustable head-motion smoothing and head roll.
- Walk where you look: the skater always faces the view, so the left stick is movement only.
- Independent per-mode camera offsets — on foot and on board each keep their own Right/Up/Forward values.
- Look-down lean on both modes, with separate forward and down distances.
- World-locked view option.
- Dismount and bail recenter as a per-axis servo: player input always applies, and pushing away from the return hands that axis over instead of blocking the stick.
- Jump camera keeps head attitude through the launch instead of yielding to the native camera.
- Bail camera phases: head-follow while tumbling, head-relative look during controllable recovery, a smoothed leveling pass while getting up, then a seamless return to the normal camera. Optional third person while bailing, third person while getting up, and camera collision (**WIP**).
- Bald in first person: a local-only scalp swap. Other players still see your normal hair; saved loadouts are not edited.
- Named personal presets and Jish's recommended preset.
- Compatibility mode for setups that must leave camera position, FOV and Spring to ReSkate.

## Compatibility mode

Enable **Compatibility mode (look controls only)** in the mod page to retain right-stick camera look, directional glances, the First Person checkbox and hotkey, and the bail/board comfort options. Third person while getting up and bail collision require full mode. Camera position, FOV and Spring remain controlled by ReSkate; head attachment, bail camera holds, foot/board transition smoothing and character turning are disabled. Saved settings are preserved when switching back to full mode.

## Supported ReSkate versions

- **2.1.1** — current target; contract, layout and byte patterns verified against the released DLL.
- **2.0.7** — retained from the previous build.

The supported game build did not change (`fbce74d5…`), so game-side contracts are shared. Spectator, controller HID emote blocking and cosmetic hair capture are selected per ReSkate version; when a build has no verified contract for one of them, that feature logs why it is unavailable and stays off rather than guessing.

## Diagnostics

Settings live under `scripts/Custom/FirstPerson`; logs are in its `logs` subfolder. The camera trace `bail-camera-trace-v11.csv` covers jumps, mounting, hippy jumps, bails and live settings changes, recording why attachment was selected and whether head data and calibration allowed it. Trace files rotate at 8 MiB; mod and proxy text logs rotate at 1 MiB, keeping three older files each.

Dev builds additionally write `debug-recording-N.csv` from **F10**: per-frame native/displayed/applied camera pose, look aim, yield reason, off-board substate, and raw controller axes and buttons. The **F7** HUD shows the same values live.

## Build and validation

Windows x64, Visual Studio C++ tools and CMake are required. Run `./build.ps1`, then `./package.ps1`. The release install ZIP contains only Mods, scripts, dinput8.dll and LICENSE.txt. Configure with `-DJISH_DEV_TOOLS=OFF` to exclude the debug HUD and recorder from a release build.

38 automated tests cover the contract profiles, bridge resolution, camera math, menu layout, bail view, spectator and the embedded page. Technical research and gameplay checklists stay in source, including [bail collision research](docs/technical/BAIL_COLLISION_RESEARCH.md) and [getting-up checks](docs/testing/TEST_THIRD_PERSON_RECOVERY.md).

See [1.28 release notes](docs/updates/UPDATE_1_28.md), [1.26 release notes](docs/updates/UPDATE_1_26.md), [1.25 release notes](docs/updates/UPDATE_1_25.md), [1.24 release notes](docs/updates/UPDATE_1_24.md), [2.0.5 update notes](docs/updates/UPDATE_1_23.md), [2.0.5 offset audit](docs/technical/OFFSET_AUDIT_2_0_5.md), [2.0.4 notes](docs/updates/UPDATE_1_22.md), [previous release notes](docs/updates/UPDATE_1_21.md), and [historical notes](docs/updates/HISTORY.md).

## License

GPLv3. See [LICENSE.txt](vendor/LICENSE) and dependency license files under `vendor/External`. The install package contains the project's LICENSE.txt.

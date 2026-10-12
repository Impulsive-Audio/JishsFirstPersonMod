# Jish's First Person Mod 1.25

Updated for ReSkate 2.0.7.

- Automatically disables ReSkate's First Person In Camera Cycle while Third person while bailing is enabled, with a visible explanation beside the option in the mod menu.
- Retains your native camera-cycle preference across restarts and recommended preset changes, and restores it when third-person bails is turned off. Native edits during the override update the preference to restore.
- Applies in full and Compatibility modes on ReSkate 2.0.6 and 2.0.7. The mod's First Person checkbox and hotkey remain available.
- Adds the exact official 2.0.7 DLL profile and verified bridge, camera sampler, console and recording-reader addresses. Earlier supported profiles remain available.

Close Skate and extract Mods, scripts, dinput8.dll and LICENSE.txt from JishsFirstPersonMod-1.25-reskate-2.0.7.zip into your ReSkate folder. Enable the mod in the standard launcher and press Insert for settings. Saved mod preferences are preserved.

Validation: Windows Release build and all 27 CTest checks pass. All 12 supported DLL fixtures reproduce the reviewed bridge addresses and pass rejection checks. The 2.0.7 source layouts, native sampler/console hooks and native recording reader/callers pass inspection. Regression checks cover restoration after restart, presets, native preference edits, delayed/rejected requests and rapid comfort toggles.

Jish confirmed this update working in-game on ReSkate 2.0.7 on 12 October 2026. First-person bail improvements inherited from 1.24 remain work in progress.

## Clearer menu help

Shorter help text, with (?) hover tips for extra detail, including disabled options. The camera-cycle explanation is beside Third person while bailing. Simplified status messages keep technical details in the mod log. Slider widths adapt so option names remain readable. Camera behavior, saved settings and defaults are unchanged.

Windows Release build and all 27 tests pass. Offline previews of the actual menu were inspected at 100% and 150% scaling, including full/Compatibility modes, both bail modes, linked/separate smoothing, unavailable controls, recording, button conflicts, save errors and disabled hover help. Jish confirmed the menu cleanup working in-game on 12 October 2026.

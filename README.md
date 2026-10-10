# JishsFirstPersonMod 1.20 � ReSkate 2.0.2

Adds the exact verified official 2.0.2 compatibility profile, native sampler and console callback addresses. Retains 1.19 camera behavior, launcher support, recording and preset fixes. See [UPDATE_1_20.md](UPDATE_1_20.md).

# JishsFirstPersonMod

**Version 1.19 — ReSkate 2.0.1:** Rebuilt from the working 1.15 source. Retains its camera mathematics and head-lock behavior, coordinates native first-person controls, and adds verified 2.0.x compatibility. Read [UPDATE_1_19.md](UPDATE_1_19.md) for current behavior, build instructions and validation limits. The version notes below describe historical releases, not validation of this update.

**Version 1.15:** Adds Game Camera Ownership awareness. First Person temporarily releases during verified spectator camera takeovers and restores when local gameplay camera ownership returns, preserving the master preference and existing comfort/input rules. Game of Skate handoff tested successfully by the user. The config panel now shows the build version. Replay pre-entry release and general scripted camera detection remain unimplemented. See GAME_CAMERA_OWNERSHIP.md.

**Version 1.14:** Adds checked automatic detection for unknown ReSkate releases in the unchanged 1.1.5-1.1.8 bridge family. Compiled profiles remain preferred; all required functions, callbacks and live camera slots are validated before activation. Unsupported structures leave the mod inactive. Mount-fix camera behavior is retained, with no jump-camera or input-resume experiments and no new saved settings. No Python, downloads or extra compatibility files are needed by players.

**1.13 mount-fix for ReSkate 1.1.8:** Built directly from JishsFirstPersonMod-1.12-mount-fix-reskate-1.1.7-source.zip with the verified official 1.1.8 bridge profile added. Retains that archive's mount smoothing and original jump/input behavior. Later jump smoothing and held-stick resume edits are excluded. No settings migration is required. Release: JishsFirstPersonMod-1.13-mount-fix-reskate-1.1.8.zip.

**1.12 mount handoff fix:** With right-stick look enabled, Smooth on/off board transitions now blends position and direction from the displayed view into the mounting head camera as soon as mounting begins. The blend continues through the on-board state change and input-lock release, using Mount smoothing even when mounting begins on foot. Right-stick input stays locked during mounting. Disable transition smoothing for an immediate handoff. No new settings are required.

**Version 1.12:** Adds the verified ReSkate 1.1.7 bridge profile while retaining 1.1.5 and 1.1.6 support. Camera/input behavior and recommended settings remain unchanged from 1.11. Install the mod DLL/bootstrap only; retain the stock ReSkate DLL. Release: `JishsFirstPersonMod-1.12-reskate-1.1.7.zip`.

**Version 1.11:** Removes the 1.10 predictive mounting state and special camera/Spring handoff. Restores the 1.9 camera and transition behavior, with one additional input gate: the native mounting flag disables custom right-stick/glance look regardless of parkour looking or jump-lock settings. No Y/Triangle prediction, confirmation window, mounting latch or new movement/facing behavior remains. Recommended settings and bail behavior are unchanged. Release: `JishsFirstPersonMod-1.11-reskate-1.1.6.zip`.

**Version 1.9:** Non-bail handoffs now retain the last displayed attached-camera pose through mounting, dismounting, climbing and selected jumps. Resuming native movement or analog look blends from that final pose, including pitch and glance rotation. While head attachment remains active, position follows the head directly and only orientation blends. Foot/board, camera and look smoothing retain their separate controls; attachment turning native Spring off does not erase saved handoff strengths. Parkour looking stays optional and off by default. Bail release keeps the reverted 1.5 behavior. Release: `JishsFirstPersonMod-1.9-reskate-1.1.6.zip`.

**Version 1.8:** Reverts the 1.6 bail handover changes to the 1.5 behavior: native orientation returns immediately when bail evidence clears, with no settle wait or orientation reconciliation. Stick/glance controls retain neutral re-entry after bails. The optional jump/climb looking from 1.7 remains, default off; head attachment and the separate jump toggles are retained. Release: `JishsFirstPersonMod-1.8-reskate-1.1.6.zip`.

**Version 1.7:** **Allow looking during jumps and climbing** is a saved option (`look_during_parkour`), default off and off in the recommended preset. Off restores the pre-1.6 native view during climbing and enabled mounting/on-foot/hippy jump attachments, releasing custom stick/glance control until normal movement returns. On enables the 1.6 camera-only look behavior described below. Head attachment and separate jump toggles remain unchanged.

Close Skate, then copy both the scripts and Mods folders from this package into the ReSkate folder containing Skate.exe and reskate.dll (Skate\ReSkate for this installation). Both JishsFirstPersonMod.dll and jishs_first_person_mod.lua now belong together in scripts/Custom/. Remove the previous JishsFirstPersonMod.dll beside Skate.exe when upgrading. Keep the same Lua filename and remove the old MyMod bootstrap if still present. Keep normal reskate.dll. Settings and logs remain under %LOCALAPPDATA%/JishsFirstPersonMod.

Press Insert to configure controls and camera settings in-game. Settings save automatically to %LOCALAPPDATA%\JishsFirstPersonMod\settings.ini. The startup and failure log is JishsFirstPersonMod.log in the same directory.

## Look smoothing

Link look smoothing to camera spring is enabled by default. Unlink it to edit the hotkey look smoothing checkbox and four strengths independently. FOV, camera offsets and base rotations still sync with ReSkate. First unlink copies the current smoothing; subsequent relinks retain the independent values. Changing linkage during a held glance takes effect after returning to centre. Spring off turns immediately.

## Comfort options

Enable First Person is the master switch in either menu and for the controller toggle. Both options below are separate, default off, and save automatically:

- Third person while bailing: a positive native wipeout signal selects third person. First person returns when the documented native riding or normal on-foot movement resumes, wipeout requests clear, and recovery/runout is inactive for two consecutive samples.
- First person on board only: walking or bailing selects third person; riding selects first person. With both options enabled, first person requires riding and no active bail/recovery.

The master stays checked while the camera temporarily uses third person. Turning it off cancels automatic restoration. Freecam, Noclip and explicit native camera restoration also disarm it. Turning a comfort option off restores the unrestricted first-person view if the master is still enabled.

Unknown or unavailable player state suspends automatic changes. A mismatched native bail/movement contract makes the bail option unavailable with a clear message. Gameplay look controls remain inhibited while in third person or while Insert is open. Bail restoration and focus/device/activation changes require neutral entry.

ReSkate owns camera activation, spring, head hiding and restoration. This mod submits requests through ReSkate's existing queue and does not modify the reskate.dll file.

This release builds successfully and passes the automated checks. Native code and movement vtable contracts were also checked against the installed executable. Live bail/foot/board transitions, recovery, respawns and multiplayer still require player testing.

## On-foot right-stick look

Press **F9** to hide/show only the first-person companion panel, including in the build editor. The panel remembers its hidden state until toggled again during that game session. It still requires ReSkate's menu to be open; Insert remains unchanged. Use **Bind menu key**, **Clear menu key** and **Cancel key recording** at the top of the panel to customize the saved keyboard binding. Insert, Escape and modifier-only bindings are reserved. Losing focus or closing the panel cancels recording. Loading the recommended preset preserves your menu key.

For a tested starting point, press **Load Jish's recommended preset** in the mod's Insert panel. This applies the bundled tested controller bindings, camera/spring values, independent look smoothing, comfort options and on-foot stick settings, and saves automatically. The recommended snapshot now enables Disable Spring on foot, keeping camera Spring on board and making walking and parkour more responsive. Existing settings are retained until you press the button. The preset does not toggle First Person or change stock Noclip/boost bindings; conflicting stock chords are marked and paused while the other preset settings apply. The bundled file is scripts/Custom/presets/recommended.ini.

Climbing and mantling attach camera position to the validated animation head while allowing right-stick and glance looking. Enabled mounting/on-foot jump and hippy-jump attachment options use the same positional behavior. Looking continues across entry and exit without another neutral-input cycle. During these actions Camera and character mode rotates only the camera; native movement retains character facing. Facing resumes on verified normal ground with its bias rebased to the current character heading.

Non-bail animation/state changes, including hippy-jump-style changes within riding mode, use Camera Spring and its directional strengths. Ordinary native movement is not smoothed twice.

Normal foot/board camera handoffs work with stick look disabled and have their own Smooth foot/board transitions checkbox and separate Getting on board / Getting off board sliders. Defaults are 70% mounting and 35% dismounting. Higher values soften the handoff for longer; zero or disabling the checkbox switches immediately. These settings save automatically and are independent of camera and look Spring. Bails, invalid player state and lifecycle resets cancel the blend. Board input is released immediately, independently of the camera transition.

Enable Right-stick look on foot under Insert. Choose Camera only or Camera and character. Camera and character turns the native physics-facing target even while stationary. The game retains native movement/collision processing; forward movement alignment and multiplayer should be checked in-game.

The feature defaults off, Camera only, 120 degrees/second horizontally, 90 vertically, 15 percent radial deadzone and non-inverted Y. Sensitivity, deadzone and invert-Y save automatically. The analog pitch range is +/-85 degrees. Releasing the stick retains the view. Horizontal turns use world-up yaw with a level horizon in both modes, preserving the native camera position and spring arm. Existing glance combos temporarily offset it using the selected smoothing.

Validated Offboard movement in active first person owns the right stick. With Allow looking during jumps and climbing enabled, this includes special climbing and jump movement; off restores native looking during climbing and enabled jump locks. Board ollies/airtime retain native trick input. Bails, invalid ownership/head data, third person, menus, focus loss and disconnection release capture and retain neutral-entry safeguards.


## ReSkate compatibility

This package supports the verified stock 1.1.5, 1.1.6 and installed 1.1.7 DLLs. The bridge selects addresses by exact DLL SHA-256 and status-export RVA. The 1.1.6 profile fixes the camera-hook rejection after updating ReSkate. Replace only the mod DLL/bootstrap; keep your current reskate.dll.


Under Spring arm and offsets, enable **Disable Spring on foot** for more responsive walking and parkour. While First Person is active, verified Offboard ownership (including climbing and mantling) temporarily turns native camera Spring off. Getting back on board, turning this option off or leaving First Person restores your configured Spring setting. Your saved Spring enable, strengths and offsets are preserved; this option never turns a manually disabled Spring on. Linked look smoothing follows the effective Spring switch; independent look smoothing and foot/board transition controls retain their own settings. The option defaults off for existing profiles.

Bails and active get-up recovery retain the previous stable-direction hold and calibrated head attachment. Once bail evidence clears, native orientation returns immediately; there is no recovery settle or post-bail orientation alignment. Stick/glance input follows the previous neutral re-entry safeguards. The jump/climb look toggle changes only looking during special actions.

For bail diagnostics, keep First Person enabled, turn Third person while bailing off, then bail and stand up. The bounded trace writes `%LOCALAPPDATA%\JishsFirstPersonMod\bail-camera-trace-v3.csv`, preserving older CSVs. It records native/final camera and animation poses, Spring/input/recovery evidence, attachment validity and facing eligibility. Writing runs on the settings worker rather than the camera thread.

**Hippy-jump attachment:** The option defaults off and requires Right-stick look on foot. Confirmed hippy jumps attach head position and bypass positional Spring/handoffs while permitting looking. Ollies, flip tricks and ordinary board airtime are excluded. This option is separate from mounting/on-foot jumping and Disable Spring on foot.

**Version 1.6:** Preserves displayed direction through bail handover with immediate input restoration and independent orientation reconciliation. Climbing and enabled jump attachments now allow looking. Releases use `JishsFirstPersonMod-1.6-reskate-1.1.6.zip`.

**Version 1.5 recommended preset:** Updated from Jish’s current saved settings. Hippy-jump locking, mounting/on-foot jump locking, Disable Spring on foot and Third person while bailing are enabled. Load the preset explicitly through the mod panel; loading retains your current menu key and stock reserved bindings.


## Game camera ownership (1.15)

A new independent spectator ownership observer preserves the First Person master preference and all existing restrictions. It is enabled for verified ReSkate 1.1.8 only. Replay pre-entry and general scripted/game-mode takeover contracts remain unimplemented; see GAME_CAMERA_OWNERSHIP.md for signals, limitations and acceptance checks.

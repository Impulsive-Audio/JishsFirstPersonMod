# Jump attachment test build for ReSkate 2.0.5

This local test build keeps the 1.23 version and existing setting names. It is not a published release.

Jump/mount attachment and hippy-jump attachment now take precedence over Disable Spring on foot. Disable Spring still controls smoothing independently. Hippy candidates must be confirmed before attaching; a confirmed hippy jump uses only its own switch even if native mounting/jumping flags overlap. The second test build removes unconditional special off-board attachment: animation, climbing and ragdoll states use native camera position when Disable Spring on foot is off and no enabled action lock or bail applies. Ground attachment, bail holds and compatibility mode retain their existing policies.

## Install and test

Close Skate and extract the test ZIP into the ReSkate folder, overwriting the included mod files. Use the standard launcher. The ZIP contains only Mods, scripts, dinput8.dll and LICENSE.txt; saved settings are preserved.

Enable right-stick look and disable Compatibility mode. For each test, repeat with Disable Spring on foot both on and off:

1. Toggle mounting/jumping attachment off and on, testing an ordinary on-foot jump and mounting the board. Keep the hippy switch fixed.
2. Toggle hippy-jump attachment off and on, testing a hippy jump. Keep the mounting/jumping switch fixed, then repeat with it changed.
3. Check landing and ordinary board airtime. Check right-stick/glance look with Allow looking during jumps and climbing enabled, and native action look with it disabled.
4. Check the comfort options and bail recovery, then enable Compatibility mode and confirm no custom position attachment remains while look and comfort controls still work.

After gameplay, inspect `%LOCALAPPDATA%/JishsFirstPersonMod/JishsFirstPersonMod.log` for matching "Attachment settings accepted" and "Attachment settings reached live camera tick" revisions. In `bail-camera-trace-v5.csv`, compare detected_action, attachment_reason, attachment_requested, attachment_status, head_valid, offset_calibrated and custom_position_delta_m between off/on runs. A zero position delta alone does not mean failure: native and attached positions may coincide. The status and moving head samples distinguish that from an invalid head, missing calibration or ineligible camera.

Trace output remains background-written, rate-limited, queue-bounded and rotated at 8 MiB with three archives. GitHub publication waits for gameplay confirmation.

## Full jump sequence test

The latest test build keeps ordinary-jump attachment through verified Trajectory, Falling and SimulatedRagdoll stages. Standalone ragdoll cannot start a jump episode. Hippy confirmation uses only its own switch. Climbing/mantling and bail-facing behavior are unchanged. For the baseline, turn Disable Spring on foot off and enable both attachment switches, then repeat ordinary jumps and hippy jumps with each switch off/on separately. Check landing, remount, bail recovery and compatibility mode. In the v5 trace, compare native_child, episode_type, episode_reset_reason, attachment_status and final camera positions throughout each sequence. Publication waits for gameplay confirmation.

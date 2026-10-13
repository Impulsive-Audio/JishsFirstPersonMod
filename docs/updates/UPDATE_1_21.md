# Version 1.21 - ReSkate 2.0.3

Adds the verified official ReSkate 2.0.3 camera bridge and updated overlay layout, native sampler, console callback and binding recorder. Existing camera behavior, launcher enable/disable handling and preferences are preserved.

ReSkate 2.0.3 disables Custom Lua startup. The included dinput8.dll proxy forwards the genuine Windows DirectInput exports and waits for ReSkate graphics initialization before loading the companion DLL. Users continue using the standard launcher. Official ReSkate.dll is unchanged.

Logs rotate automatically: 8 MiB for bail traces, 1 MiB for each text log, with three archives each. CSV headers are retained and logging continues after rotation; a blocked archive is retried without permanently stopping capture.

## Validation

All 24 Windows tests pass. Binary/source offset checks and native hook/recording checks pass. The user confirmed working startup and gameplay through the standard launcher. The latest logs show successful hook initialization, native activation and bail switching/restoration. The trace was verified appending beyond the former permanent 8 MiB stop; rotation subsequently replaced unlimited growth and passed retention/header/recovery tests.

Replay pre-entry and general scripted-camera detection remain existing limitations. See OFFSET_AUDIT_2_0_3.md for the distinction between static contracts, logged execution and remaining scenario coverage.

## Installation

Close Skate and extract the install ZIP into the ReSkate folder containing Skate.exe and ReSkateLauncher.exe. Merge Mods and scripts; keep the official ReSkate.dll. Do not overwrite an existing third-party dinput8.dll. Enable the mod in the standard launcher. Press Insert for settings.

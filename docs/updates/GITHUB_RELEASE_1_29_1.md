## Sony USB controller menu crash fix

Fixes the reproduced menu-opening crash with a PS5 controller connected over USB and Steam Input disabled.

- Removes the opposing HID/render lock order introduced in 1.28.
- Passes batched HID reads to ReSkate menu capture one physical report at a time, preserving USB/Bluetooth layouts.
- Retains 1.29 camera controls, personal presets and settings menu. No 1.30 features are included.

Validated with all 32 Windows tests and confirmed working in-game by the user with PS5 USB and Steam Input disabled.

## Installation

Close Skate. Extract the install ZIP into the ReSkate folder beside Skate.exe and ReSkateLauncher.exe, merging Mods and scripts and replacing this mod's existing files. Enable Jish's First Person Mod in the launcher. Existing saved settings and personal presets are preserved.

Use `JishsFirstPersonMod-1.29.1-reskate-2.0.7.zip` to install. The `-source.zip` contains source and build tools.

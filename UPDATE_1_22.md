# First Person Mod 1.22 for ReSkate 2.0.4

Adds an exact-hash profile for the official ReSkate v2.0.4 DLL, updates the relocated camera bridge, native first-person sampler, console queue and Binds recording reader, and preserves earlier reviewed releases.

Compatibility mode remains available with right-stick look, directional glances, First Person activation and both comfort options. Log rotation and the standard-launcher proxy remain in place. Existing settings are preserved.

The supported Skate build is unchanged (25414733). Source inspection confirms unchanged bridge layouts and camera command IDs; binary inspection verifies the relocated entries and sampler/recording call destinations. All 24 Windows tests pass, and the compiled bridge matches all nine official release fixtures, including 2.0.4. The official-source layout probe and 31 game-offset checks also pass. The final install package was tested in-game on ReSkate 2.0.4 and confirmed working.

Close Skate, update ReSkate to 2.0.4 through its normal launcher, and extract the install ZIP beside Skate.exe, merging Mods and scripts. The ZIP contains only Mods, scripts, dinput8.dll and LICENSE.txt. Open the mod panel with Insert.

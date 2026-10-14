## Automatic log rotation

Bail trace rotates at 8 MiB. Mod and proxy text logs rotate at 1 MiB. Each keeps three older files (.1 newest, .3 oldest); the oldest is removed during successful rotation. Approximate combined retained budget: 40 MiB. CSV archives and fresh files retain their header. Capture continues across rotations, and trace disk work runs on the background writer. If an archive is blocked, append continues and rotation is retried on later writes; storage can temporarily exceed the budget until the lock is released. Existing data is rotated rather than discarded at the first threshold. All 24 Windows tests pass, including retention and blocked-rotation recovery.

# Install with the standard ReSkate launcher

Close Skate. Copy **dinput8.dll**, **scripts** and **Mods** into the ReSkate game folder beside Skate.exe, ReSkate.dll and ReSkateLauncher.exe. Merge scripts and Mods. Start using the normal ReSkate launcher and enable Jish's First Person Mod there. Press Insert for the companion panel.

The dinput8 proxy forwards DirectInput to the genuine System32 DLL and waits for ReSkate graphics initialization before loading scripts/Custom/JishsFirstPersonMod.dll. Stock ReSkate 2.0.3 disables Custom Lua scripts; the included Lua bootstrap is retained for older releases. Saved preferences remain in scripts/Custom/FirstPerson.

Do not overwrite another mod's existing dinput8.dll. Proxy diagnostics are in scripts/Custom/FirstPerson/logs/proxy-loader.log; native mod initialization is in JishsFirstPersonMod.log.

To remove this installation, remove this package's dinput8.dll, scripts/Custom/JishsFirstPersonMod.dll, scripts/Custom/jishs_first_person_mod.lua and Mods/JishsFirstPersonMod. Keep your preferences and other mods.

Standard-launcher startup and gameplay were confirmed by the user for this release.

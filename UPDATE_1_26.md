# Jish's First Person Mod 1.26

Updated for ReSkate 2.0.7.

**Glances without accidental emotes:** Hold LB/L1 and tap or hold a bound D-pad direction to glance. Double tap the same direction within 250 ms to emote; hold the second tap for a held emote. The camera returns to centre for the emote. Works with both hold and toggle glances. Release the D-pad to start another glance; LB/L1 can stay held.

This applies while first-person glance controls are active and only to bindings using LB/L1 plus one D-pad direction. Other bindings and normal third-person emotes are unchanged. Menu help explains the controls.

Uses existing ReSkate and mod handlers for XInput and native PlayStation USB/Bluetooth. Emote blocking requires the verified ReSkate 2.0.7 handler; earlier supported versions retain their previous behavior.

**Bail improvements - WIP:** First-person head following and smoother stand-up camera movement are still being refined. The character can still spin when getting up.

Close Skate and extract Mods, scripts, dinput8.dll and LICENSE.txt from JishsFirstPersonMod-1.26-reskate-2.0.7.zip into your ReSkate folder. Enable the mod in the standard launcher and press Insert for settings. Saved preferences are preserved.

Windows Release build and all 29 tests pass, including tap timing, quick/held emote forwarding, camera clearing, controller interruptions, private reads, duplicate reports and Bluetooth checksums. Automated checks do not establish behavior on every controller setup.

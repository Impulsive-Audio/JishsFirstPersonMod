# First Person Mod 1.23 for ReSkate 2.0.5

Adds an exact-hash profile for the official 2.0.5 hotfix DLL and updates the relocated native mutex lock/unlock addresses. The first-person sampler, console queue, camera bridge layout and recording reader retain their 2.0.4 addresses. Recording call-site verification includes the relocated menu caller.

Right-stick look, directional glances, First Person checkbox/hotkey, both comfort options and compatibility mode are preserved. The standard-launcher proxy and rotating diagnostics remain in place. Existing settings and earlier reviewed releases remain supported.

Update ReSkate to 2.0.5 through its normal launcher, close Skate and extract the install ZIP beside Skate.exe, merging Mods and scripts. It contains only Mods, scripts, dinput8.dll and LICENSE.txt. Open the mod panel with Insert. All 25 Windows tests, ten release-fixture bridge checks, 31 game-offset checks and four inspector guard tests pass. Native sampler/console and menu-enabled recording-reader verification also pass. The final startup-fix build initialized successfully in-game on 2.0.5. Logs confirm the menu binding, camera/input hooks, native recording and controller First Person activation.

## Startup follow-up

The first 2.0.5 in-game attempt recognized the correct DLL but failed to attach the controller tick hook with Windows error 5 (access denied), before menu installation. The original log omitted the failing operation. The follow-up build logs the hook operation, Windows error, thread ID and retry count, and retries thread collection/enlistment failures for at most two seconds before refusing safely. It never skips an inaccessible live thread. Bootstrap logging now uses the actual build version instead of the old hardcoded 1.21 label. The corrected build was subsequently observed initializing successfully and binding its menu in-game.

Follow-up validation: all 25 Windows tests pass, including injected transient access denial, persistent live-thread refusal, subsequent recovery and rollback.

Official asset: `JishsFirstPersonMod-1.23-reskate-2.0.5.zip`. It is the exact startup-fix install ZIP renamed for publication. SHA-256: `4CE00AC235977486778E0F8EEC76B4FAF6977748AD26291AAF3B51C96B2A7C47`.

# Local 1.30 spectator test 3

The previous source ZIP and test 2 use the same native party-camera callback.
The observed hotkey is accepted, but the blend repeatedly restarts. This is
consistent with a runtime rejection; the previous build did not log its reason.

Match ReSkate 2.0.7's active party-camera ownership check: input inactivity is
a prerequisite for entering spectate, not an additional requirement after
activating the party camera. Retain the native snapshot, active-camera, mode,
view, thread and transform checks.

Log native snapshot rejection and spectator reset reasons at most once per
second. Keep the rotating-buffer fixes from test 2 and the stable Sony HID fixes.

This is a candidate fix and diagnostic build. Native in-game behavior must be
verified; mocked runtime tests cannot establish native TLS scope or camera flags.
No GitHub publication or installed-game changes are part of this build.

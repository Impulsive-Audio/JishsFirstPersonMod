# 1.27 DirectInput gesture test — ReSkate 2.0.7

Built from the packaged 1.27 source, separately from later camera experiments.
Adds mod-owned DirectInput 8 A/W adapters for immediate state reads and buffered
events, including custom data formats. ReSkate's handlers still run first.
Sony VID/PID and HID object usages verify the L1 and POV mapping. Unknown devices
and formats pass through unchanged. The ReSkate DLL is unchanged.

The first bound L1 + D-pad glance press is consumed. A matching second tap within
250 ms reaches the game, including holding the second tap. The existing enter/exit
hotkey always consumes its D-pad direction, in either camera mode, and keeps that
direction consumed until physical release, even when L1 releases first or the
camera changes. The adapter never fires the toggle action; the existing independent
controller reader still detects the configured hotkey.

DirectInput events retain their original sequence, timestamps, unrelated values,
return status and caller flags. Repeated buffered peeks reuse the original edit.
Buffered double taps use event timestamps, rather than batch delivery time.
Historical buffered events already seen in immediate reads reuse their decisions.
An older event absent from the retained snapshot history is conservatively treated
as a first press rather than creating a new emote in the current live policy.
Digital face/shoulder/system buttons are mapped for additional hotkey modifiers;
DirectInput analog-trigger modifiers are not mapped in this test build.

## Installation and live validation

Close Skate and extract the test ZIP's contents into the ReSkate folder, replacing
the matching mod files. Existing saved settings are retained.

1. Connect a DualSense by native USB, without Steam Input or DS4Windows translation.
2. Check the mod log for `DirectInput gesture adapters prepared`, then
   `DirectInput Sony gesture adapter mapped L1 and POV`. On the first consumed
   press, look for `DirectInput Sony gesture filtering observed`.
   Preparation alone does not prove the game uses DirectInput. If those Sony
   messages never appear, the reported bleed-through may use a different path.
3. Start in third person. Press the configured L1 + D-pad enter hotkey once:
   first person should start once, with no gesture. Release and press again:
   third person should start once, with no gesture.
4. Repeat rapidly, hold the hotkey, and release L1 before the D-pad. No gesture
   should occur, and a held press must not toggle repeatedly. Test during camera
   transitions and with the enter/exit direction also assigned to a glance.
5. In first person, test a different glance direction: one press glances without
   a gesture; two taps within 250 ms allow the emote; holding the second tap
   retains the emote. A pair more than 250 ms apart must remain glances.
6. Confirm ordinary D-pad input, other buttons and sticks still work. Open/close
   the menu and change focus, then verify no stale double tap survives.
7. Recheck Xbox enter/exit and glance controls. Optional: repeat with native Sony
   Bluetooth and DualShock devices.

## Automated validation

The adapter regression executable exercises standard DIJOYSTATE/DIJOYSTATE2,
A/W object metadata, custom offsets, immediate/buffered reads, DX3 event records,
event timing, repeated peeks, mixed historical reads, toggle/glance precedence,
L1-first release, camera transitions, input loss, acquire/unacquire, COM disposal,
unknown identities/formats and private reads. It also prepares, deduplicates,
activates and removes hooks against real Windows DirectInput factory/device
implementations. The existing XInput and Sony HID regressions remain included.

Live Skate/DualSense behavior requires the user's controller and running game;
the automated checks do not establish which API the game reads on that system.

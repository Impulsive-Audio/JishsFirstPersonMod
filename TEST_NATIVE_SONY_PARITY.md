# 1.27 native Sony parity test 2 — ReSkate 2.0.7

## Evidence and correction

The user's logs recognize a DualSense Edge (Sony 054c:0df2) over USB with
64-byte input reports. The first DirectInput test build loads, but there are no
Sony DirectInput mapping/consumption messages. Steam Input supplies a virtual
Xbox controller through gameoverlayrenderer64.dll and the existing XInput policy
works there. Adapter preparation alone did not establish native-path coverage.

A read-only probe of the connected Edge parsed a real 64-byte USB report and
verified its DirectInput L1 usage (Button 5) and POV usage (Generic Desktop 0x39).
The Edge product ID and USB layout were already supported.

Installed Skate.exe inspection found the native HID worker calling ReadFile and
GetOverlappedResultEx via a shared helper at RVA 0x547cdf0. Its callers at
0x5478493 and 0x547a03d supply a buffer which can hold 32 input reports. The setup
at 0x54781e2 and 0x5479c68 shifts the input-report length by five: a 64-byte USB
report therefore allows a 2048-byte buffer. ReSkate 2.0.7's captured_read_file
only identifies an unknown report handle when the requested buffer is at most
512 bytes. A separate game read handle can consequently miss shared neutral_hid
coverage. The new completion adapters remove that dependency. Live logs must
still confirm the path and buffer size used in the user's next game session.

The mod now identifies the native game's actual Sony handle and gamepad HID
collection, using its declared report length independently of read capacity.
It filters successful synchronous reads, overlapped results, completion-port
results, callback reads and HidD_GetInputReport results. A batch of USB reports
remains USB; reports are processed individually. Shared/native completion edits
are deduplicated across the entire batch. Failed, incomplete and cancelled reads
retain their API status and callback behavior. Ordinary file I/O stays unchanged.

ReSkate binding readers, this mod's private sampler, DirectInput's underlying
stream and Steam's translation stream remain unfiltered. The game-facing
DirectInput/XInput filters still apply the same policy to their outputs.
The native adapter runs the existing glance and toggle policy; it never invokes
another camera toggle or changes the controller's firmware settings.

The user's configured enter/exit chord is 770 (0x302): L1 + R1 + D-pad Down.
Regression coverage verifies that complete chord in both camera modes, repeated
taps, camera transitions and releasing L1 first. Enter/exit never gains a glance
double-tap bypass. Ordinary glance pairs retain the 250 ms rule.

## Install and validate

Close Skate and extract the native-Sony-parity-test2 install ZIP into the ReSkate
folder. It contains the mod, standard loader and existing scripts/manifest/preset.
It does not include or modify ReSkate.dll or saved preferences.

1. Disable Steam Input for Skate and connect the Edge by USB.
2. Enter a world and test L1 + R1 + Down to enter and exit first person.
   Both actions should happen once per press with no gesture.
3. Hold the chord, release L1 before the D-pad, and repeat quickly. No held or
   repeated enter/exit press should become an emote.
4. Test L1 + another bound D-pad direction: first tap glances with no gesture;
   a second tap within 250 ms allows the emote and its hold; slower pairs do not.
5. Check the mod log for these separate markers:
   - `Native HID completion adapters prepared` (installation only).
   - `Native HID game-read handle identified` (Sony PID, report length and capacity).
   - `Native HID game report observed` (native read completion reached the policy).
   - `Native HID gesture consumed` (actual D-pad suppression, physical and forwarded buttons).
6. Recheck Xbox and Steam Input emulation with the same hotkeys and glances.

If an actual gesture-consumed marker appears but a gesture still plays, retain
the logs: that means another game-facing source is still supplying that press.
Do not treat an installation marker by itself as proof of successful blocking.

## Validation

The release regression executable includes native Edge synchronous, overlapped,
completion-port, callback and input-report adapters; 2048/8192-byte buffers;
batched 64-byte USB reports; shared/native duplicate handling; cancellation;
private-reader/focus exclusions; and the user's complete enter/exit chord.
It also activates the Windows API hooks and verifies ordinary file forwarding
and rollback. Existing DirectInput, Xbox and Sony HID tests remain included.

The Windows x64 Release build succeeded and all 31 CTest cases passed. The native
adapter tests also verify failed callback submission and x64 NTSTATUS failure
handling without editing failed report payloads. Build/test logs are retained
in this source workspace; generated logs are excluded from the source ZIP.

The connected Edge probe verified recognition and report parsing. A fresh live
game run is required to verify end-to-end gesture suppression on that controller.

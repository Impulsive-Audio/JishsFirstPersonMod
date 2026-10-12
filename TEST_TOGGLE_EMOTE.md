# First-person toggle emote suppression — local test 4

Test 5 keeps toggle suppression eligible during transient camera-readiness and
camera-ownership changes. A toggle press always takes priority over the glance
double-tap emote path. The consumed hold survives transient skater-camera
snapshots within the same world; changing world still clears it.

LB/L1 plus a D-pad direction bound to First Person now consumes the bound
D-pad input for the game in both third and first person. The mod reads the
original input and switches camera through ReSkate's existing request queue.
Toggle double taps and holds have no emote bypass. LB/L1 and unrelated buttons
are retained; a consumed D-pad hold stays consumed until that direction is
released, even if LB/L1 is released first or the view changes.

This extends the existing XInput/Raw Input and verified shared ReSkate report
handler. No new HID interception, input reader, polling thread or camera-switch
API has been introduced. Menus, focus loss, paused/conflicting bindings,
controller disconnects and binding/character changes keep their existing guards.
The binding recorder and mod controller sampler remain unfiltered.

Test entering/exiting first person using LB/L1 + D-pad, repeated presses,
double taps, holds and releasing LB/L1 first. Confirm unrelated directions
still reach the game and ordinary glance double-tap emotes still work. Check
Xbox and PlayStation USB/Bluetooth if available. This package retains collision
test 2 and the temporary bald hairstyle from test 3.

Install ZIPs contain only Mods/, scripts/, dinput8.dll and LICENSE.txt.
Documentation stays in the source repository. No publication before gameplay
acceptance.

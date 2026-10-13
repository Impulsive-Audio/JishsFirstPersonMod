# Toggle emote suppression test9

Fixes two reproducible leaks in test8: the published toggle filter expired after
500 ms without a producer refresh, and per-controller consumed holds expired
after 500 ms without a game input read. Long camera/cosmetic work could therefore
pass the toggle's D-pad press, or its held release tail, back to the game.

Toggle suppression now lasts until explicit eligibility, world, controller or
binding invalidation. It remains independent of first-person camera readiness.
Only ordinary glance tap state expires on a stale producer/read. Focus/menu
gates, private controller reads, binding conflicts and disconnect handling stay
in place. Glance binding edits reset glance state without discarding toggle hold
state. The existing client callback publishes the toggle configuration before
head/camera processing, independently of glance publication.

Automated checks cover producer and game-read gaps of 1200 ms, modifier-first
release, rapid re-entry, both Sony pad types over USB/Bluetooth, checksums,
focus reset and unchanged glance double-tap emotes. No new input hooks/readers.

Gameplay: repeatedly enter/exit first person with LB/L1+D-pad; try immediately
after loading, camera recovery and hair swaps. Release LB/L1 before D-pad and
hold D-pad during the switch. Check unbound directions, menu navigation, focus
loss, recording and deliberate glance double-tap emotes. Includes test8's
third-person getting-up option. No GitHub push or publication.

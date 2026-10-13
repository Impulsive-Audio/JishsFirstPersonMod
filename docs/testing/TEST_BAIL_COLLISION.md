# 1.27 bail camera collision — local test build

Test 2 fixes a camera remaining on the floor after getting up. Collision now
ends with the bail/recovery transition. If the camera stalls for 150 ms during
controlled recovery, a reverse sweep from the ordinary camera target attempts
to free it. Failed queries keep the last position until recovery ends. The
desired stand-up trajectory is independent of the collision-corrected view.

Requires ReSkate 2.0.7 and the currently verified Skate executable. This build
has not been published. Install the ZIP using the same layout as the current
mod package, with Skate closed. Keep your existing ReSkate installation.

In the first-person menu, turn off **Compatibility mode** and **Third person
while bailing**, then enable **Camera collision during bails** (on by default).
The recommended preset currently enables third-person bails, so check that
setting before testing. Your existing other settings are preserved.

Try:

- A face-first fall onto flat ground and a slope.
- A bail into a wall, corner, rail or low ceiling.
- Sliding sideways along a floor or wall while bailing.
- Fast bails, getting up, and another bail during recovery.
- Respawning, teleporting, opening menus, changing views and returning to play.
- Turning collision off and on to compare the camera.
- Menu text and hover help at your normal and increased display scaling.

The camera uses a 10 cm sphere with 1 cm clearance, changes position only, and
smooths its return after an obstruction. World queries run from the final
camera write, including ReSkate's latest-animation render handoff, on the
observed engine thread. No additional input or polling system is used.

As requested, this experiment assumes the camera starts clear of geometry and
does not add skater-body exclusions. Clipping after spawning inside geometry
is outside this test. If the camera pushes away from your own body, sticks,
jitters, or causes a frame-time spike, describe the bail and keep the mod log.

Logs record native verification, the first engine-thread/world confirmation,
query failures and queries taking over 2 ms (rate limited). A mismatched build
disables this feature. A native exception pauses collision until restart. The
old camera behavior remains when the toggle is off or its thread/world guard
does not permit a query.

Automated checks exercise the resolver and settings; gameplay/native-query
behavior and actual frame time still need your test. Do not publish this build
before gameplay acceptance.

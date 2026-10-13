# Faster bail recovery test7

Includes test6's local-only bald hairstyle and previous controller fixes.
No GitHub push or publication.

- Getting-up position follows the live calibrated head with a 50 ms smoothing
  time constant; collision corrections never replace the desired trajectory.
- Controlled position return tracks the current final normal-camera position
  and completes after 100 ms. The existing rotation blend remains 300 ms.
- Floor sticking triggers reverse recovery after 50 ms without 2 mm progress,
  provided the normal target is at least 10 cm away and control has returned.
- Controlled collision return uses a 30 ms smoothing time constant, shortened
  near the position deadline to avoid leaving lag for the final release.
- Collision history releases at position completion, including while the bail
  flag or rotation blend remains active. New wipeouts can reacquire protection.
- Failed queries hold the safe position during recovery; ownership still ends
  on time. A failed recovery can therefore require a final normal-camera handoff.

Gameplay: check face-first falls, standing up and immediately walking away,
slopes, walls, corners, low ceilings, repeated bails, respawns and the collision
toggle. Confirm getting-up follows the head without the previous floor delay
or delayed snap, and rotation remains smooth. Starting inside geometry remains
outside scope. Docs remain in source; the ZIP contains installation files only.

# Local marker glance and on-foot alignment update

Based on released 1.33; this is a local test build, not a published release.

- General: Refresh presets rereads the personal preset directory. Existing
  selection remains if present; a deleted selection falls back to Recommended.
- Controls: Remember glance on return to marker defaults off, requires Toggle
  glance controls, persists in settings and personal presets. It restores the
  selected direction after the native local-entity teleport transition. That
  signal can also cover other teleports. Menus, world/device changes, loss of
  focus, bails and disabling first person discard the pending direction. Memory
  expires after five seconds. Held buttons cannot toggle the restored glance
  again until released. Hold-to-glance behavior is unchanged.
- Camera and character look: recenter yaw to the skater and level pitch when
  on-foot character-facing control resumes. Wait for a neutral right stick
  before taking control. This includes dismount and bail recovery handoffs.
  Camera-only look keeps its existing initialization behavior.

Gameplay checks:

1. Add/remove a personal INI while running; Refresh presets should update the
   list without loading a preset or changing current settings.
2. Toggle a glance, return to marker with memory off/on, and check the difference.
   Verify same-direction release/repress centers and another direction replaces.
   Test marker return from near and far away, on board and on foot.
3. In full first person with Camera and character look, turn, get on/off board,
   and run forward: camera and movement should agree. Repeat after bails and
   while holding/releasing the right stick. Confirm Camera Only is unchanged.

Automated tests cover marker-memory timeout/world/controller safety, restoration
and press-edge behavior, preference persistence, neutral recentering and
camera-only behavior. Native marker timing and real animation handoffs require
the gameplay checks above.

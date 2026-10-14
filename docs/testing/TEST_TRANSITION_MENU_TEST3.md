# Transition smoothing and Insert/F9 test3

Based on the bail/dismount outward-facing reset test2 for ReSkate 2.0.7.

Foot/board transition smoothing now retains its own settings while character-lock input rearms or the native movement child changes during a dismount. New targets rebase the residual from the last displayed pose, preserving continuity without switching to look/animation strengths. Mount/dismount interruption and disabled smoothing retain their existing behavior.

Menu behavior:
- Insert opens ReSkate and the first-person panel together.
- F9 hides/shows the companion while ReSkate stays open.
- Insert restores a hidden companion while ReSkate stays open; a subsequent Insert closes both.
- F9 outside ReSkate opens/closes the first-person panel by itself.
- Insert with a standalone panel open joins it to ReSkate; the next Insert closes both.

In-game checks: compare low and high mount/dismount smoothing strengths with character lock enabled. Test immediate and delayed neutral-stick entry, several headings, and rapid remount/dismount. Verify disabled smoothing remains immediate. Exercise the menu sequence above, including held/repeated keys and key release.
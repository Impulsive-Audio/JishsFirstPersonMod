# Bail recenter direction test

Based on JishsFirstPersonMod 1.33 marker-recenter-folders-test2 for ReSkate 2.0.7.

With right-stick Camera and character look enabled, the post-bail recenter now places camera yaw 180 degrees from the character transform heading. The camera uses +Z as its backward axis, so this points the view outward instead of into the body. Character heading stays unchanged during the reset, and subsequent stick turns preserve the offset.

Dismount recenter uses the same outward-facing 180-degree offset. Camera-only look keeps its existing behavior. The neutral-stick requirement remains in place.

In-game check: enable right-stick look with character lock, turn the view, bail, recover, and release the stick to neutral. Verify the view faces away from the head/body, then turn left and right and confirm the character remains coupled. Repeat from several initial headings.
Dismount check: while using character lock, mount and dismount at several headings, release the right stick to neutral, then turn left/right. Confirm the camera faces outward and stays coupled without rotating the character on recenter.

# Local 1.30 integration on the working 1.29.1 foundation

This development source combines the existing 1.30 work with the working Sony HID menu fixes from 1.29.1. The original local source directories remain untouched. Publishing this source does not create a release or change installed game files.

Retained 1.30 features include optional body-flip head lock, first-person spectator view, their settings/presets, camera handoffs and diagnostics. See UPDATE_1_30.md and the body-flip/spectator gameplay checklists.

The integrated 1.29.1 fixes query overlay status outside the HID mutex and pass batched native Sony input to stock menu capture one descriptor-sized report at a time. Their native report handling and regression coverage are retained alongside the 1.30 changes.

The version is 1.30.0 and packages target ReSkate 2.0.7. Saved preferences and personal presets are not bundled. Verify Insert, clothing and server-browser menus with PS5 USB and Steam Input disabled, then check the 1.30 body-flip and spectator handoffs. Automated validation does not replace those gameplay checks.

Combined validation: the clean Windows Release build passed all 35 tests on 13 October 2026, including body-flip, mocked spectator camera/render/restoration, and native Sony HID report/lock-order checks. In-game validation of this combined 1.30 build remains pending.

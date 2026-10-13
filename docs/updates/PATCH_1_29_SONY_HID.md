# 1.29 Sony HID menu patch

Fixes opening menus with a native PlayStation controller connected over USB and Steam Input disabled. This is a source patch to 1.29, packaged as release 1.29.1. No 1.30 features are included.

The HID adapters now query overlay eligibility before acquiring their input mutex. Completion bookkeeping releases that mutex before filtering. This avoids the opposing HID/render lock order introduced in 1.28 when menu rendering polls an independent Sony input stream.

The shared ReSkate menu-capture adapter also forwards each descriptor-sized report separately. Previously a batch of 64-byte USB reports was passed as one large report, causing ReSkate 2.0.7 to select the Bluetooth layout and modify the wrong input offsets.

All 32 Windows tests pass, including regression checks for HID/overlay lock ordering, USB/Bluetooth report batches, report sequence bytes, CRCs, buffer boundaries, completion delivery and existing gestures. The user confirmed test build 2 resolves the reproduced in-game menu crash with a PS5 controller connected via USB and Steam Input disabled. Clothing and server-browser screens were previously reported as triggers; separate confirmation for those screens has not been recorded.

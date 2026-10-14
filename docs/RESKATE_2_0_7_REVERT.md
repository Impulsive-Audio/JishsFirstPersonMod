# Local 1.30 for reverted ReSkate 2.0.7

Base: the supplied JishsFirstPersonMod-1.30-reskate-2.0.8-source.zip.
Target: the exact official ReSkate 2.0.7 DLL, SHA-256
8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427.

Restore the 2.0.7 party/remote getters, state pointers, render hook and camera/FOV
callback sites. Selected spectator ID/epoch are at 0x3828/0x3830 rather than
2.0.8's 0x3850/0x3858. Enable the spectator bridge for verified 2.0.7.

Also restore the shared Sony HID and outgoing cosmetic capture contracts, so the
2.0.8 addresses do not disable controller protection or local-only bald handling.
Keep the tested Sony report-boundary and lock-order fixes and all existing 1.30
camera and spectator behavior. The historical camera bridge already includes the
verified 2.0.7 profile; its native sampler, FOV, recording and bindings offsets
are selected automatically, including bindings at 0x5f78.

Version remains 1.30.0. This package targets 2.0.7. The original attachment and
previous 2.0.8/2.0.9 source trees remain unchanged. No GitHub publication or
installed-game replacement is included.

Compatibility validation does not establish that the earlier reported spectator
camera behavior is fixed. In-game testing is still required.

Validation: all 35 CTest tests passed; 13 real release fixtures through 2.0.8 matched all eight camera bridge addresses, including 2.0.7. Both MSVC layout probes compiled against official 2.0.7 source. Spectator signatures, native camera sampler, controller reader, Sony HID handler/callers and complete cosmetic capture/read/array/caller bytes passed official-DLL audits. The mocked spectator runtime now explicitly selects the 2.0.7 profile rather than assuming the newest table entry is the target.

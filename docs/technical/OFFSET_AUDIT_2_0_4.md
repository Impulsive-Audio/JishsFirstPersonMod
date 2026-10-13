# ReSkate 2.0.4 offset audit

Official release: https://github.com/Dingo-Shenanigans/ReSkate/releases/tag/v2.0.4
Tagged source commit: 6b3d2ba.
Released DLL SHA-256: `9259c3de99626d6a9d6083b337927bc948ee144dbf4e698f7733148979aeb528` (matches GitHub asset digest and launcher.json).

| Contract | 2.0.4 RVA |
| --- | --- |
| Overlay status export | 0x3cbaa0 |
| Overlay state pointer | 0x905458 |
| Native mutex lock / unlock | 0x6cd424 / 0x6cd450 |
| Native FOV writer | 0x217f30 |
| Source initialized / game base / transform callback | 0x8c22a0 / 0x8c2538 / 0x8c2548 |
| First-person sampler | 0x2196c0 |
| Sampler render / FrameSpring call sites | 0x217cd2 / 0x2187bb |
| Console queue | 0x411b0 |
| Binds recording reader | 0x3d00f0 |
| Recording menu-enabled call sites | 0x3eab0d / 0x420198 |

Bridge layouts are unchanged from 2.0.3: callbacks 0x4d0, model 0x500, debug prefix 0x80, bindings 0x5f50, camera issue 0xa0, status 0x1a8. Camera action IDs and native sample ABI are unchanged. These are checked against the tagged declarations and unique binary matches, not inferred from a global offset delta.

The upstream changes include DualSense HID neutralization and replay/entity-memory fixes. First-person, source-camera and controller-binding declarations match 2.0.3. The supported game executable is unchanged: SHA-256 `fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`. All 31 game code/vtable checks pass.

Static verification is complemented by the user's successful in-game test of the final 1.22 package on ReSkate 2.0.4.

Validation completed: official-source layout probe compiled successfully; 24 Windows tests passed; all nine released DLL fixtures passed compiled/forced structural recognition and rejection tests; native sampler/console and recording-reader checks passed; four inspector guard tests passed.

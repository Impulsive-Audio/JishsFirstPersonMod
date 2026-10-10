# ReSkate 2.0.5 offset audit

Official release: https://github.com/Dingo-Shenanigans/ReSkate/releases/tag/v2.0.5
Official DLL SHA-256: `ca9c103f85167ac6957e15ed45b999f59afc387d2722277114047f8e04fe6aaa` (matches GitHub asset digest and launcher.json).
The upstream v2.0.5 tag points to the same source commit as v2.0.4: `6b3d2bad9709e57a1b07f0b57c0c6810126f5dc7`. The released DLL has a different hash, so its entries were independently inspected rather than inferred from that tag.

| Contract | 2.0.5 RVA |
| --- | --- |
| Overlay status / state | 0x3cbaa0 / 0x905458 |
| Mutex lock / unlock | 0x6cd1c4 / 0x6cd1f0 |
| Native FOV writer | 0x217f30 |
| Source initialized / game base / transform callback | 0x8c22a0 / 0x8c2538 / 0x8c2548 |
| First-person sampler | 0x2196c0 |
| Sampler render / FrameSpring callers | 0x217cd2 / 0x2187bb |
| Console queue | 0x411b0 |
| Binds reader | 0x3d00f0 |
| Menu-enabled recording callers | 0x3eab0d / 0x41fed8 |

The bridge prefix layout and native command/sample ABI are unchanged. Supported game build 25414733 and executable SHA-256 `fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9` are unchanged. Static checks are complemented by successful final-build startup logs: the camera/input hooks initialized, the companion menu bound, the recording reader was selected and the controller First Person request reached the native queue.

Validation: 24 Windows tests passed; all ten released DLL fixtures passed structural recognition/rejection checks; 31 game-offset checks and four inspector guard tests passed. Native sampler/console prologues, sampler call destinations and menu-enabled recording reader/callers were independently verified. The tagged source is byte-for-byte identical to the already compiled 2.0.4 official-layout probe source.

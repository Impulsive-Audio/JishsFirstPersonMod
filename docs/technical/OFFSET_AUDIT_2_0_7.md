# ReSkate 2.0.7 compatibility audit

Base mod source: GitHub main commit `623e9d9` (1.24). Official ReSkate source: tag `v2.0.7`, commit `0f8af62`. Official DLL SHA-256: `8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427`.

The reviewed Settings, DebugAction, Model and State blocks match 2.0.6. First-person camera-cycle flag remains at settings byte 47; queue action 33 sets it, and action 34 resets the arm while preserving it. Callbacks remain at 0x590, model at 0x5c0 and bindings at model offset 0x5f78.

| Contract | 2.0.7 RVA |
| --- | --- |
| Status export | 0x3f4190 |
| Overlay state pointer | 0x94cf80 |
| Mutex lock / unlock | 0x6fbc74 / 0x6fbca0 |
| FOV writer | 0x22a390 |
| Source initialized / base / transform | 0x9063f0 / 0x906690 / 0x9066a0 |
| Native camera sampler | 0x22bb20 |
| Sampler callers | 0x22a132 / 0x22ac1b |
| Console queue | 0x42210 |
| Recording reader | 0x3f87e0 |
| Recording callers with allow_menu=true | 0x410f08 / 0x446898 |

Verified source layouts, unique masked instruction matches, PE sections, exact prologues and relative call targets. Generated bridge recognizers include 2.0.7. All 12 official historical DLL fixtures pass compiled and forced discovery paths plus mutation/rejection checks; unknown 2.x hashes remain rejected.

The published repository accidentally omitted vendor/Engine/Game/Build because its unanchored build/ ignore pattern also matched that source directory on Windows. Restored the unchanged game contracts from the retained 1.24 workspace and anchored the build output ignore to the repository root.

Windows Release build: 27/27 CTest checks pass. Hook inspector unit tests: 4/4 pass. Jish confirmed the 1.25 update working in-game on ReSkate 2.0.7 on 12 October 2026. Automated checks do not establish every in-game bail recovery timing case.

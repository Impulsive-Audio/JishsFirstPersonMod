# Local 1.30 smoothing split test

Target: ReSkate 2.0.7. Radial-menu behavior is unchanged.

Bald mode and its native cosmetic-copy and outgoing cosmetic-capture hooks are removed. Legacy bald_first_person keys are ignored and dropped on save.

Network audit: active src code has no remaining socket/send/receive hooks, packet rewriting, or outgoing cosmetic mutation. Multiplayer pose-layout helpers in proven_first_person and spectator_runtime read native poses. Spectator head visibility changes only leased render buffers and restores them; received pose vectors and cosmetics are untouched. stock_bridge sends local camera-setting requests to the native settings queue, not the network. Vendor multiplayer source remains available as dependency headers and is not compiled as multiplayer runtime code by this mod.

Migration copies prior effective look settings into independent stick settings, including linked Spring suppression on foot. Migrated settings are saved once. Recommended/default presets explicitly store stick settings.

Gameplay regression checklist:
- Try all four glance/stick smoothing toggle combinations and each directional strength.
- Change camera Spring and glance linkage; stick smoothing must remain unchanged.
- Walk, glance on board, mount/dismount, and repeat in compatibility mode.
- Toggle first person, change outfits, and join multiplayer: no temporary bald change.
- Restart and reload a personal preset; both sets of smoothing settings must persist.

Automated validation: final Windows Release build and all 34 CTest tests passed, including migration, legacy bald-key removal, four smoothing toggle combinations, directional response, settings commands, serialization and preset application. Menu DPI/layout checks passed. The user confirmed this update was tested and working in gameplay on 14 October 2026. Individual checklist results were not separately recorded.

# Source comparison

All supplied source files were compared byte-for-byte, including bundled vendor files. Unchanged files are omitted below. Generated Python caches are excluded. This inventory distinguishes changed files from diagnosed bugs; not every version change is a regression. Read UPDATE_1_19.md for the behavioral findings.

| File | 1.15 → 1.18 | Delivered source |
| --- | --- | --- |
| `BINDING_CONFLICT_REVIEW.md` | Added in 1.18 | Absent |
| `CMakeLists.txt` | Changed in 1.18 | Updated for 1.19 |
| `INTEGRATION.md` | Changed in 1.18 | Updated for 1.19 |
| `NATIVE_TANDEM_REVIEW.md` | Added in 1.18 | Absent |
| `README.md` | Changed in 1.18 | Updated for 1.19 |
| `RESKATE_2_0_1_REVIEW.md` | Added in 1.18 | Absent |
| `RESKATE_2_0_REVIEW.md` | Added in 1.18 | Absent |
| `build.ps1` | Changed in 1.18 | Updated for 1.19 |
| `package.ps1` | Changed in 1.18 | Updated for 1.19 |
| `src/bail_view.h` | Changed in 1.18 | Identical to 1.15 |
| `src/binding_conflicts.h` | Added in 1.18 | Updated for 1.19 |
| `src/bootstrap.cpp` | Changed in 1.18 | Updated for 1.19 |
| `src/bridge_patterns.h` | Changed in 1.18 | Identical to 1.18 |
| `src/bridge_resolver.cpp` | Changed in 1.18 | Identical to 1.18 |
| `src/comfort.cpp` | Changed in 1.18 | Updated for 1.19 |
| `src/config.cpp` | Changed in 1.18 | Identical to 1.18 |
| `src/first_person_runtime.cpp` | Changed in 1.18 | Updated for 1.19 |
| `src/game_camera_ownership.cpp` | Changed in 1.18 | Updated for 1.19 |
| `src/menu.cpp` | Changed in 1.18 | Updated for 1.19 |
| `src/menu_input.cpp` | Changed in 1.18 | Updated for 1.19 |
| `src/mod.h` | Changed in 1.18 | Updated for 1.19 |
| `src/native_aim.h` | Added in 1.18 | Absent |
| `src/native_controls.h` | Added in 1.18 | Updated for 1.19 |
| `src/settings.cpp` | Changed in 1.18 | Updated for 1.19 |
| `src/settings.h` | Changed in 1.18 | Identical to 1.18 |
| `src/stick_look.h` | Changed in 1.18 | Identical to 1.15 |
| `src/stick_runtime.cpp` | Changed in 1.18 | Identical to 1.15 |
| `src/stock_bridge.cpp` | Changed in 1.18 | Updated for 1.19 |
| `src/stock_bridge.h` | Changed in 1.18 | Identical to 1.15 |
| `src/stock_camera_feed.cpp` | Changed in 1.18 | Identical to 1.15 |
| `src/stock_contract.h` | Changed in 1.18 | Identical to 1.18 |
| `tests/binding_conflict_tests.cpp` | Added in 1.18 | Updated for 1.19 |
| `tests/bridge_resolver_tests.cpp` | Changed in 1.18 | Identical to 1.18 |
| `tests/config_validation.cpp` | Changed in 1.18 | Identical to 1.18 |
| `tests/native_tandem_tests.cpp` | Added in 1.18 | Absent |
| `tests/stock_contract_tests.cpp` | Changed in 1.18 | Identical to 1.18 |
| `tools/generate_bridge_patterns.py` | Changed in 1.18 | Identical to 1.18 |
| `tools/inspect_hooks.py` | Changed in 1.18 | Identical to 1.18 |
| `tools/official_layout_probe.cpp` | Added in 1.18 | Identical to 1.18 |
| `tools/review_2_0.py` | Added in 1.18 | Absent |

New 1.19 files: `src/native_camera_interop.h`, `src/native_camera_interop.cpp`, `tests/native_interop_tests.cpp`, `UPDATE_1_19.md`, `SOURCE_COMPARISON.md`, `tools/verify_native_interop.py`.

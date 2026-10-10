# Controller recording and preset fix

Controller recording now calls ReSkate's own Binds-page reader with
`allow_menu=true`, preserving its controller selection, menu input bypass,
focus checks and device identity. The full 48-byte 2.0.x input structure is
used, including its keyboard-state tail; only the controller prefix is copied
into the mod's recorder. Exact reviewed DLL hashes and function prologues gate
the native call. Reviewed reader RVAs: 2.0.0 `0x3e55a0`, 2.0.1 `0x3e5f40`.
Older releases retain the previous fallback reader.

The recommended preset now applies camera and comfort settings even when a
hotkey overlaps a native control. Only marked conflicting hotkeys are paused;
re-record them to resolve the conflict. Native bindings and the saved menu key
remain unchanged. Conflict feedback remains visible after settings save.

Automated checks cover the native recording ABI, menu bypass flag, controller
buttons/device/style, unavailable snapshots, preset conflict handling and
persistence. Physical recording still requires an in-game check.

Native controller conflicts compare complete button masks. L1 + D-pad down
and L1 + R1 + D-pad down are distinct chords; only identical chords are
marked as conflicts. Regression checks cover both subset directions,
recommended glance-down activation and recording acceptance.

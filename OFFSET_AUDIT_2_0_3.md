## Automatic log rotation

Bail trace rotates at 8 MiB. Mod and proxy text logs rotate at 1 MiB. Each keeps three older files (.1 newest, .3 oldest); the oldest is removed during successful rotation. Approximate combined retained budget: 40 MiB. CSV archives and fresh files retain their header. Capture continues across rotations, and trace disk work runs on the background writer. If an archive is blocked, append continues and rotation is retried on later writes; storage can temporarily exceed the budget until the lock is released. Existing data is rotated rather than discarded at the first threshold. All 24 Windows tests pass, including retention and blocked-rotation recovery.

# ReSkate 2.0.3 offset and attachment audit

No offset mismatch found in the audited contracts. No runtime behavior or installed DLL changed during this audit. GitHub was not modified.

## Binary identity

Installed ReSkate.dll: 083bd7c9636d7ff1432cb643842ba58159f26cf8ab3bbb1d2c22df322f865912.
Installed supported Skate.exe: fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9.
Installed companion and proxy exactly match the staged tested DLLs.

## Static verification

- Eight bridge addresses: compiled and forced-recognition C++ fixture checks pass for all eight official profiles, including 2.0.3. Unknown hashes and modified code/layout rejection tests pass.
- Official MSVC source layout probe passes: State callbacks 0x4d0, model 0x500; Model debug 0x80, bindings 0x5f50; DebugModel arm 0x10, issue 0xa0, status 0x1a8. Native command IDs, callback size and recording ABI checks pass.
- Native sampler 0x210080 and its two callers, console queue 0x40e30, recording reader 0x3c6c80 and menu-enabled callers match installed DLL bytes.
- Read-only audit_game_offsets.py passes 31 checks: six player/bail/owner prefixes, pre-remap selector call destination, three foot-substate vtable update slots, camera mode/transform, physics update, skater motion, client tick and spectator prefixes, camera/view-manager vtable slots, and engine address bounds. Bounds checks prove location, not semantics; unchanged official source and runtime identity checks supply additional evidence.
- Camera/player/pose address headers and relevant source contracts are unchanged upstream between 2.0.2 and 2.0.3. Vendored implementation differences predate this update; the corresponding official files are unchanged. The 2.0.3 overlay State/Model edits are explicitly reviewed by the inspector.
- All 23 Windows tests pass, including proxy forwarding and host refusal.

## Runtime evidence from the latest 1.21 session

The latest log starts at uptime 5000984. It selects the verified 2.0.3 profile and reports successful bridge, input tick, bail observer, pre-remap selector, spectator observer, sampler, native motion/input filters, SourceTrial transform/FOV feed and menu hooks. The DX12 queue/presentation pairing and panel binding complete. The tick reaches the native client; native Binds recording is used. Controller toggles queue enable and disable. Two wipeout sequences temporarily select third person and restore first person on movement recovery. No initialization rejection/failure appears in this session.

Successful attachment alone does not prove every feature was exercised. Mounting/dismounting, parkour/head locks, hippy jumps, Camera and character turning, native stabilization/Follow flips yielding, and multiplayer spectator takeover/restoration still need explicit gameplay coverage. Dynamic object offsets are guarded by identity, ownership, bounds, finite-value and validity checks; this audit did not read live process memory.

## Diagnostic limit

The audit observed the former 8 MiB stop. This was subsequently removed in the continuous-trace revision: existing CSV files continue appending and no permanent stop flag remains. The regression verifies repeated appends above 8 MiB. Existing trace data was preserved.

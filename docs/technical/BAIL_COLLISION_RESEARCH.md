# Bail camera collision: native verification gate

Historical verification notes from the initial research pass. A local 1.27
experiment is now implemented; see TEST_BAIL_COLLISION.md for the current
behavior and test procedure. The user subsequently removed initial-overlap
recovery and skater-body exclusions from this experiment's scope.

The final-write integration is guarded by the observed client engine thread
and a live world obtained from that thread's TLS context. ReSkate publishes its
tick camera after the native update and its latest head pose at the render
handoff, which explicitly checks the same engine thread. Both writes reach the
mod's existing transform hook. The native query performs its own read locking;
the reviewed lock bytes are now part of the runtime fingerprint checks.
Gameplay results and performance remain to be validated by the local test.

## Supported executable and reproducible audit

Inspected installed Skate.exe, SHA-256:
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.
This is the executable named by ReSkate 2.0.7's skater-state/body contracts.

Run `tools/audit_bail_collision.py <Skate.exe>` using the same pefile dependency
as the existing game-offset audit. It reads the file only; it does not load the
game or call physics. Success verifies the listed static facts, **not** a safe
complete camera-collision implementation.

## Verified sphere-query wrapper

The sphere wrapper at RVA `0x26c0fd0` and ray wrapper at `0x26afdc0` both call
`0x2664c00`. Their Windows x64 arguments are world (RCX), output result (RDX),
request (R8), and debug label (R9); both return the result pointer.

The sphere request differs from ReSkate's ray request. Do not pass its
`NativeSurfaceRay` structure to the sphere function:

| Sphere request offset | Field |
| --- | --- |
| 0x00 | start float[4] |
| 0x10 | end float[4] |
| 0x20 | radius float |
| 0x24 | filter uint32 |
| 0x28 | mode uint32 |
| 0x2c | tolerance float |
| 0x30 | flags uint32 |
| 0x38 | allocator vtable |
| 0x40 | allocator |
| 0x48 | native ignored-body list |

The shared implementation reads a begin/end list of 16-byte handles, then takes
each handle's uint32 index at +8. It does not consume the skater's 0x130-byte
simulation-body records directly.

The native sphere caller at `0x7cc4f0` constructs the request at stack RBP+0xf0,
sets filter `0x40080303`, mode **1**, and calls the sphere query at `0x7cc996`.
Do not copy the park-editor wrapper's default mode of zero without investigating
the mode's semantics; the common query validates it.

Result layout is shared with the existing `NativeSurfaceResult`: world/data at
+0/+8, allocator vtable/allocator at +0x10/+0x18, first/count at +0x20/+0x24,
temporary allocation scope at +0x30. ReSkate's ray wrapper documents the scope
pop and explicit allocator cleanup. Both native caller paths use the same
cleanup functions (`0x2640f50` and `0x25fa1d0`).

## Required verification still missing

1. **Complete player-body exclusions.** ReSkate's skater-state layout exposes
   board and skeleton simulation records via core+0x430 -> +0x18 and
   core+0x438 -> +0x2f10, respectively. Those records contain counts and owner
   pointers, not the required native ignore handles. The native board update at
   `0xfe5100` reads a separate handle array at its own receiver+0x718, validates
   the handle through `0x25d7b10`, and forwards it to `0xfc06b0`. The receiver
   relationship to the mod's board/skeleton objects, and the complete ragdoll
   handle enumeration, have not been established. An arbitrary pointer scan or
   guessed category-mask change is not a verified substitute.
2. **Initial penetration.** The sweep wrapper/result fractions alone do not
   establish how overlap at the start is reported, including zero-length
   sweeps or a sphere fully inside geometry. A no-hit result cannot yet certify
   a seed or recovery position as outside the floor. Verify the relevant native
   overlap behavior/query before implementing seeding and recovery.
3. **Camera update scope.** Existing park-editor probes run on ReSkate's client
   update using the current client TLS context. The mod already chains this
   update in `src/hooks.cpp`, but its final camera transform callback must be
   proven to execute in the appropriate scope before querying synchronously.
   Querying on a different thread, or consuming an unchecked stale correction,
   would not meet the plan's final-position requirement.

World distance uses metres in ReSkate's documented skater/body and collision
capsule layouts. The planned starting radius is 0.10 m, but near-plane clearance
and the runtime result must still be assessed during gameplay.

## Implementation after the gate passes

Use the existing update/camera chain and native queries, without a polling
thread. Resolve only final position after attachment/look/blending; keep
rotation unchanged. Maintain a validated safe position, immediate obstruction
correction and collision-tested smooth return. Feed corrected positions back
into bail recovery history. Cover bail and get-up/release, and reset on camera,
character, world, focus, menu and teleport discontinuities.

Add the saved, default-on **Camera collision during bails** option only when the
supported runtime can actually perform those checks. Disable it in compatibility
mode and third-person bails, and on unverified builds. Preserve the requested
short menu help. Test native request/cleanup and exclusions as well as the
position resolver, then build Release and provide a clearly labeled local test
ZIP. No push or publication before gameplay acceptance.

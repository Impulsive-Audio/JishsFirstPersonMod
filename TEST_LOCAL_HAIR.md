# Local-only bald hairstyle test

This test retains the bail camera, bald hairstyle and toggle emote fixes from
test5. No release or GitHub publication is included.

## Verified integration

ReSkate 2.0.7 DLL SHA-256:
`8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427`.

The complete outgoing appearance capture at RVA `0x101c70`, read callback at
`0x102e20`, native item array copy at `0xfdc40` and outgoing call site at
`0x9e349` are checked against inspected bytes before hooks are enabled.
Windows x64 arguments are hidden result storage in RCX, game base in RDX,
NativeFrame in R8 and diagnostic string in R9. The frame entity is at +0x18.
Appearance is 0xe0 bytes; optional engagement is at +0xe0. Compile-time checks
guard the corresponding local type sizes. ReSkate owns all capture allocations.

Only during the outgoing call, a thread-local scope filters the exact copied
native item array belonging to the cached local cosmetic component. Only the
scalp slot with the temporary bald hash is replaced. It borrows an immutable
snapshot of the original asset and counted parameter array until the original
capture returns. The original read still runs first, and ReSkate performs its
usual deep copies and cosmetic validation. The native recipe is never edited
by the network filter. Other threads, board entries, other slots, incoming
cosmetics and externally changed hair pass through normally.

The snapshot shares a mutex with live hair ownership changes. Failed snapshots
return an empty optional and retain the previously sent appearance. Unsupported
capture fingerprints or failed hook installation disable the bald option, so
an unprotected temporary bald swap is not enabled.

## Gameplay checks

- Join a lobby with another player. Enable Bald in first person. Verify your
  local long hair disappears and the other player still sees your original hair.
- Stay in first person longer than one second; enter/exit repeatedly and check
  the remote hairstyle does not flicker.
- Have another player join while you are already in first person. Their first
  appearance snapshot should contain your original hair.
- Turn the option off in first person and verify exact local restoration.
- Change hair, hair colour and clothing in customization; enter first person
  again. Verify the remote outfit is current and the current original hair is
  retained. Naturally bald hair should remain unchanged.
- Check respawns, character/slot changes, map changes, reconnects and bails.
- ReSkate Local Echo can check the outgoing appearance on one machine, but a
  second-player test is required to confirm real multiplayer behavior.

The automated hair tests cover copied-array selection, asset/hash restoration,
native parameter counts, unchanged live items and unrelated slots, external
hair changes, existing restoration/failure handling and settings persistence.

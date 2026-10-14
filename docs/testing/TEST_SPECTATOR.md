# First-person spectator verification — 1.30

## Automated results

- All 35 Windows tests passed, including the spectator contract/persistence tests and mocked camera/render integration test.
- The read-only binary audit passed for official ReSkate 2.0.7 SHA-256 `8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427`.
- Official-source MSVC release-layout assertions verify party publication, selected ID/epoch, record identity/slot, remote entity/component/context/generation and rendered-buffer offsets. No stock STL object is constructed, owned or invoked by the bridge.
- Tests cover ambiguous IDs, local/wrong-world targets, invalid slots, stale/future timestamps, rejoining, slot reuse and changes during capture. Code-signature changes and an unsupported game reject the adapter.
- Pose tests cover moving-head entry, direct full lock, smooth exit, 250 ms timing, target changes, teleports and sample gaps. Existing body-flip tests retain quaternion wraparound coverage for the shared handoff.
- The mocked runtime drives the real spectator adapter, skeleton reader and render hook. It verifies default follow view, binding switching, configured eye-camera offsets, camera-specific FOV restoration, render-only head hiding, restored scale on exit/interruption, session reset and disabling.
- Alignment checks cover ReSkate's head-to-camera axis conversion, eye midpoint and forward clearance, rotated/scaled skeletons and rejected collapsed eye spacing. Native stabilization, Follow flips, smoothing, pitch, roll and bob settings are copied into the spectator look pipeline. The ReSkate 2.0.7 Spring implementation is reused in a separate namespace without changing the existing bridge ABI.
- Same-frame render refinement tracks an animated head movement after the initial camera update. Repeated Spring samples are checked against a single update from the frame-start state, preventing double integration. Render-camera ownership rejection restores head visibility and discards the view.
- Persistence tests verify default off, older-file fallback, invalid boolean rejection, settings commands and personal presets. Existing binding-conflict, recording, camera-ownership and settings-preservation tests remain passing.
- Narrow 360-pixel panels were rendered and inspected at 100%, 150% and 200% DPI, including the available control and its disabled reason. ReSkate fonts, square styling, top tabs and scrolling content remain readable. The menu harness also verifies tab changes, recording cancellation and preset text entry.

## Bridge audit

Run `python tools/audit_spectator_bridge.py <official-ReSkate-2.0.7.dll>` with dependencies from `tools/requirements.txt`. The audit reads the PE file without loading it and verifies code/data sections, exact signatures, pointer references, remote stride and party transform/FOV call sites.

Compile `tools/spectator_layout_probe.cpp` against the official ReSkate 2.0.7 source headers using MSVC `/std:c++20 /EHsc /MT /DNOMINMAX /D_ITERATOR_DEBUG_LEVEL=0`. All static assertions must pass. Runtime support is restricted to the matching existing stock profile and verified game build; unknown layouts remain unavailable.

The adapter reads selected ID/epoch from the party state, scans a bounded POD copy of published records for an exact ID/slot match and verifies the remote actor generation/context/component. Capture executes only at the verified party-camera transform call. A separate render callback must confirm the watched skeleton buffer before head view begins. Neither player proximity nor root-position matching is used for identity.

## Gameplay checks

Pending: no Skate process was available for gameplay validation during this build.

- With the option off, confirm existing spectator takeover/restoration behavior.
- With it on, join throwdowns and Game of Skate. Begin each spectator session in follow view, switch using the existing keyboard and controller bindings and return to the local turn with the original First Person preference.
- Manually spectate a party player and switch repeatedly. Retain the view choice while changing watched players; exit and re-enter in follow view.
- Verify eye alignment and configured offsets/FOV against the ordinary local first-person view. Exercise stabilization, Spring, fast flips and bails, and smooth 250 ms entry/exit. Check for camera lag relative to rendered geometry. Remote right-stick look cannot be reconstructed. Check that the face, hair and headwear do not obscure head view and return in follow view.
- Exercise player disconnect/rejoin, actor respawn, map changes, loading, teleports, delayed packets and unavailable head/render poses. Confirm the normal follow camera and a readable reason, with no stale actor writes.
- Exercise pause/menu focus, text entry, binding recording and conflict states. No view switch while input is captured; saved local settings remain unchanged by spectator toggles.
- Verify native spectator targets outside ReSkate's party path and other game-owned cameras remain unchanged.

Gameplay and network latency may affect received animation. This reconstructs first person from that animation; it does not reproduce remote right-stick look or remote camera preferences. ZIPs remain local 1.30 test packages; GitHub publication is separate.

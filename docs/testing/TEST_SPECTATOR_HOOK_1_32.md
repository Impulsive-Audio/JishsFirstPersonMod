# 1.32 spectator render-chain compatibility

The affected installation reported resolved address=0 after passing all spectator function-signature checks. The native jump-chain resolver only follows E9 relative jumps and FF25 indirect jumps, up to eight links, and returns zero for other forms or unreadable chains. The exact differing chain on that machine is unknown.

This update makes native render-chain resolution advisory. It attaches directly to the independently signature-verified ReSkate spectator render function even if the game entry chain is unrecognized. No change is made to the native chain or network data. Camera, pose, identity and ownership checks remain in place. The checkbox remains selectable and enabling retries an unavailable initialization.

Regression fixture deliberately returns zero from render-chain resolution and exercises successful hook installation, camera/render behavior and restoration. The user confirmed 1.32 works on both machines; a third-party hook that never calls ReSkate render may prevent render evidence, in which case the mod retains the follow camera.

Provided logs confirm the exact supported 2.0.7 DLL SHA-256 (8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427). Spectator initialization failed before ReSkate logged remote-player hook installation at 11:27:13.048. Upstream session.cpp installs these hooks once the world/map is ready. This establishes a startup ordering dependency; there is no evidence of a bad rollback. Version 1.32 removes that prerequisite by hooking the verified stock render function directly. Final Release build: all 34 tests passed. The user confirmed version 1.32 tested and working on both machines on 14 October 2026.

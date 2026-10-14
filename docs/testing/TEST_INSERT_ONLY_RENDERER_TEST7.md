# Insert-only renderer rollback test7

Renderer restored byte-for-byte from commit cef1b1b6a557271fe99dd56072592941b3cd8cb7 (1.29.1-reskate-2.0.7).

Input routing restored from that commit with minimal compatibility support for controller navigation and closing the current panel. No standalone renderer, cursor warp/clipping hooks, window-wide capture, or overlay status queries from the async-key hook remain.

Insert opens ReSkate and the companion panel. F9 hides/shows the companion only inside Insert. Closing Insert closes both; opening Insert restores the companion. Standalone F9 opening is removed. Keyboard binding controls were removed to reflect this fixed behavior.

Camera direction, head position, bail/dismount recenter and transition smoothing changes remain untouched.

Validation: renderer hash matches the local checkout at the exact release commit. Full build/test suite required. In-game menu opening must be verified after installation.
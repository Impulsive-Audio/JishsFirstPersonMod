# Local 1.30 spectator toggle test 2

The user's toggle was accepted, but logs showed repeated entry transitions for the same watched player. A rotating-buffer runtime test reproduced the failure.

The camera now stages its transition when a new animation buffer appears, retains the native follow camera until the render callback verifies that buffer, and writes the refined head view during that render callback. Missing prior render evidence no longer discards the selected transition. FOV remains native until render verification.

Head hiding retains bounded restoration leases for temporarily unpublished buffers. It restores a hidden scale only when the same actor/generation publishes that buffer again; destroyed actors and overwritten animation scales are not modified. This avoids a reused buffer keeping the tiny hidden-head scale and failing eye-pose validation.

Regression coverage includes alternating pose buffers, same-frame camera writes, completed entry/exit handoffs, FOV restoration and deferred head-scale restoration. In-game validation of this fix is pending. No GitHub changes or installed-game changes were made.

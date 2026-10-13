# Jish's First Person Mod 1.24 — ReSkate 2.0.6 test build

Adds optional Toggle glance controls and compatibility with the official ReSkate 2.0.6 DLL. Existing hold-to-glance controls remain the default. The jump-episode attachment fixes and bounded v6 diagnostics are included.

## Glance controls

Enable Toggle glance controls beside the glance bindings. Press a bound combo to look and hold, press it again to return, or press another direction to replace the current glance. Only one direction stays active. Releasing the direction button rearms the combo even if its modifier remains held. Simultaneous new directions are ignored. Existing glance angles, smoothing, compatibility mode and right-stick look continue through the same camera output path.

Menus, focus loss, controller disconnection/change, leaving first person, blocked camera states, world/player changes and binding/angle/mode edits clear the selection. Resume requires neutral input. The preference saves as toggle_glances; the selected direction does not persist. Existing profiles and the recommended preset default to hold mode.

## Compatibility audit

Official v2.0.6 source: Dingo-Shenanigans/ReSkate commit 314d31f. Installed runtime matches the official asset SHA-256 dff7b0211996b596611019f44e2c6de7ca8c18a6fc65106c0e32100e06520d0f.

Verified bridge RVAs: status 0x3e42f0, state pointer 0x933fc0, mutex lock/unlock 0x6ea434/0x6ea460, FOV writer 0x21d4c0, SourceTrial initialized/base/transform 0x8ee2a0/0x8ee540/0x8ee550. Overlay callbacks/model move to +0x590/+0x5c0 because of the additional swapchain hooks. Binds availability moves to Model+0x5f78, confirmed in compiled copy code at 0x4042c; binding values start four bytes later.

The camera-cycle preference occupies Settings+47 within the unchanged 64-byte native settings. The mod preserves it. Native camera-cycle entry/exit is accepted as manual first-person intent when no mod request, comfort suppression or game-owned camera transition is pending. Native reset moves from command 33 to 34; command 33 passes through as the new cycle setting.

Native sampler 0x21ec50 is called at 0x21d262 and 0x21dd4b. Console queue is 0x41d90. Controller recording reader is 0x3e8940, with allow_menu callers 0x4037b8 and 0x439138. Older supported exact-hash profiles remain available.

## Validation and gameplay test

All 27 Windows tests pass, all 11 release fixture bridges reproduce the reviewed addresses and rejection checks, and all 40 executable code/vtable checks pass. Native sampler/console, recording reader, source layout changes, native-cycle preservation and toggle-glance behavior are verified. The base 2.0.6/toggle-glance build was confirmed working by Jish; this head-following bail update still requires gameplay confirmation.

Close Skate and extract Mods, scripts, dinput8.dll and LICENSE.txt from JishsFirstPersonMod-1.24-reskate-2.0.6-test.zip into the ReSkate folder. Check Insert, first-person activation/camera cycling, hold/toggle glances on board and on foot, independent smoothing, compatibility mode, jump attachment and bail comfort/recovery. No GitHub release is published until gameplay is confirmed.

## Head-following bail test update

With Third person while bailing off in full mode, calibrated bails now follow the head's position and full rotation instead of freezing camera direction. Recovery right-stick offsets follow the moving head and never turn the character. Glances are cleared/inhibited throughout bails. No settings, presets or compatibility-mode behavior change.

Recovery look requires the owned Ground child (+0x60, vtable RVA 0x65e8c20), native offboard control flags +0x9c0 bits 13/14 set, animation override bit 0 and trajectory request bit 17 clear, Ground motion override +0x140 clear, and no mounting or current wipeout request. Animation/ragdoll movement stays locked, including get-up animation while ordinary control remains unavailable. Static executable contracts verify selection at 0x481833b and Ground input consumers at 0x48319f5/0x4831a87. Recovery/runout at animation+0x9e is not used as proof of control.

The v6 rotating background trace records the bail phase, control evidence, head target quaternion and recovery look offsets. Existing file-size/queue limits remain. Test with Third person while bailing off, full mode, and Right-stick look enabled: spend a moment in normal first person to calibrate, bail, tumble, recover, release the stick to neutral, and look around as movement control returns. Repeat with stick look off, Third person while bailing on, and compatibility mode. Gameplay confirmation is required before publishing.

Automated bail acceptance: all 27 CTest checks pass, including full head pitch/yaw/roll and positional target assertions, local recovery look, neutral re-entry, controller changes, unavailable evidence and contract rejection, character-turn exclusions, foot Spring combinations, missing-head/calibration fallback, lifecycle resets, release blending and bounded v6 diagnostics. Installed game audit passes 40 code/vtable checks. ReSkate 2.0.6 bridge/source, native sampler and controller recording inspectors pass. Replayed the retained v2 trace: 3,410 samples, 612 head-facing bail samples and 587 head-attached samples. Replay validates the new math against captured poses; it does not prove current gameplay recovery timing.

## Camera-only get-up lock test

First step for the recovery spin observed in gameplay. Native ragdoll flags20/21 and bit2 select animation action6 (RVA 0x4832e8f/0x4833228); verify both executable consumers and the Ragdoll/AnimatedRagdoll vtable update slots before recognition. Detection is limited to the current owned children and applies only to first-person bails in full mode with Third person while bailing off.

Capture the last displayed facing before the get-up rotation, then smoothly decay its pitch/roll toward a level view (120 ms decay time) while keeping the facing target fixed. Head position continues to track. Keep the held orientation through controllable recovery, permit local right-stick offsets after native control returns, and release through the existing normal-camera transition. No body-facing hook or physics change is introduced. Existing lifecycle, head-data and calibration safeguards remain.

The bounded v7 trace adds getup_requested/getup_locked and a getting_up view phase. Install the get-up-camera-lock-test ZIP and repeat the spinning bail/get-up sequence. Check camera stability, return of look/movement, and whether the character still spins. Body-facing intervention is deferred until this gameplay result is known; no GitHub publication.

## Get-up body-facing test

The camera-only get-up hold was confirmed working, but body spin persisted. This follow-up uses the existing verified skater motion target hook to hold the owned character's facing to the camera's fixed get-up view direction. Camera +Z is its back axis; character +Z is facing, so the target is camera heading +180 degrees. Preserve native position, surface-up and matrix metadata, with no skeleton-body or velocity writes. This operates independently of right-stick enabling/character-turn preference and only during the existing attached get-up lock in full first person with Third person while bailing off.

Require the current rig/context/core/entity, active/requested first person, an unsuppressed owner, a recent camera request (100 ms), and live validated offboard bail movement. Drop the facing override as soon as native movement control returns, even before the bail latch clears. Camera resets clear the request. Compatibility mode, third-person bail and game-owned camera states bypass it. The v8 bounded trace adds requested body lock, body target heading and the count of applied motion overrides so actual gameplay can confirm that the native hook is used. Test stand-up facing, absence of body spin, head attachment and immediate return of normal player turning. GitHub publication remains pending.

## Get-up animation-root lock test

The motion-target override applied during gameplay, but root/head animation still rotated. This follow-up chains the native skater animation update (RVA 0xf96b80, SDK's existing remote-pose publication callback) and updates the owned local output's joint1 quaternion after the original native/SDK animation finishes. Other actors and native evaluation remain unchanged.

Calibrate the normal grounded relationship between rig motion-facing and the world animation root; the recorded skeleton basis differs by approximately -90 degrees. Derive the desired world root from the held body/view heading and this calibration, then convert it into local joint1 space using the current parent joint0 rotation. During the active get-up body hold only, write these 16 quaternion bytes to the validated 395-bone output. Preserve the root position/scale/metadata and all descendant joints so the native get-up articulation continues. This locks animation output rotation; it does not freeze physics bodies or write velocities.

Require current factory ownership, matching core/client/entity/context/rig, a recent held camera request, valid parent/root quaternions, matching live output buffer/count and first-person/full-mode bail eligibility. Recheck these before writing. Missing calibration/output skips the correction and records rejection. Native control return clears eligibility immediately; subsequent animation evaluation publishes its normal pose. The camera lock is unchanged. The bounded v9 trace adds root calibration/applied/rejected counters and records the resulting root/head transforms for gameplay confirmation.

Install JishsFirstPersonMod-1.24-reskate-2.0.6-get-up-animation-root-test.zip, repeat the spinning get-up, and check actual body stability and facing as movement resumes. No GitHub publication until confirmed.

## Animation-root startup fix

The executable animation detour failed at startup and left a pending hook that prevented the facing hook and menu from initializing. The animation-root update now chains the verified SkaterComponent vtable callback at +0x140 instead, preserving the native/SDK evaluation without allocating an executable trampoline. Startup rollback restores this callback only while owned by the mod. Install the get-up-animation-root-startup-fix-test ZIP and confirm Insert opens before repeating the get-up test. Gameplay acceptance and GitHub publication remain pending.


## Detached get-up camera test

Remove the get-up motion-facing override and animation-root callback entirely. During verified stand-up, capture the displayed camera pose, carry its position with owner-validated rig world displacement, and keep stable facing with gradual pitch/roll leveling. Animated head motion no longer drives camera position during stand-up. Native character animation/physics receives no get-up writes.

Once native movement control returns, blend position toward the moving calibrated head target and orientation into ordinary camera behavior over 300 ms using smoothstep, allowing safeguarded right-stick look. The detached return owns its transition to prevent a second handoff blend. Renewed ragdoll resumes head-following; invalid head/calibration, owner changes, menus/focus loss, suppression, teleports and observation gaps clear the detached state. No settings changes.

The bounded v10 trace records detached/blending/attached phase, progress, calibrated head target and final displayed position. Install JishsFirstPersonMod-1.24-reskate-2.0.6-detached-get-up-test.zip with Third person while bailing off; confirm natural body recovery, stable camera, smooth return and movement/look control. Gameplay acceptance and GitHub publication remain pending.

## Smoothed head-position get-up follow

Replace the displaced-player stand-up anchor with a continuous 100 ms exponential follower of the live calibrated head position. Capture the last displayed position at entry so there is no entry snap. Head rotation remains held/leveled independently; no character-facing or animation-root overrides are installed.

At verified control return, smoothly shorten the positional follower's time constant to zero over the existing 300 ms orientation return. Its target becomes the ordinary native camera position, avoiding a final switch between calibrated-head and native positions. Same-frame reads do not advance positional smoothing. The existing v10 trace labels this phase smoothed_head_follow. Lifecycle safeguards, saved settings and publication restrictions remain.

Install JishsFirstPersonMod-1.24-reskate-2.0.6-smoothed-head-get-up-test.zip and test with Third person while bailing off. Confirm continuous head-position following, stable get-up facing and no final snap.

## Smooth stand-up orientation handoff fix

Gameplay v10 traces show the camera rotating 129 degrees in 15 ms when stand-up flags clear on AnimatedRagdoll, immediately before Ground movement control returns. The previous renewal condition incorrectly treated that normal transition as a fresh tumble and discarded the camera hold, preventing the 300 ms return from running.

Keep smoothed head position and held/leveled facing through the flag-clearing transition. Interrupt a get-up episode only on a new owner-validated positive wipeout request edge; repeated requests and unavailable evidence cannot interrupt it. Verified control return starts the existing full quaternion blend from the last displayed rotation, including roll, and it survives bail-latch clearing. No player-character overrides or saved setting changes. Existing bounded v10 diagnostics remain.

Install JishsFirstPersonMod-1.24-reskate-2.0.6-smooth-orientation-return-test.zip and repeat the reported stand-up sequence with Third person while bailing off. Confirm no sideways roll/snap at recovery completion. GitHub publication remains pending gameplay acceptance.

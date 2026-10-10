# Native integration and comfort behavior

**Version 1.19 — ReSkate 2.0.1:** Rebuilt from the working 1.15 source. Retains its camera mathematics and head-lock behavior, coordinates native first-person controls, and adds verified 2.0.x compatibility. Read [UPDATE_1_19.md](UPDATE_1_19.md) for current behavior, build instructions and validation limits. The version notes below describe historical releases, not validation of this update.

**Version 1.15:** Adds Game Camera Ownership awareness. First Person temporarily releases during verified spectator camera takeovers and restores when local gameplay camera ownership returns, preserving the master preference and existing comfort/input rules. Game of Skate handoff tested successfully by the user. The config panel now shows the build version. Replay pre-entry release and general scripted camera detection remain unimplemented. See GAME_CAMERA_OWNERSHIP.md.

**Version 1.14:** Adds checked automatic detection for unknown ReSkate releases in the unchanged 1.1.5-1.1.8 bridge family. Compiled profiles remain preferred; all required functions, callbacks and live camera slots are validated before activation. Unsupported structures leave the mod inactive. Mount-fix camera behavior is retained, with no jump-camera or input-resume experiments and no new saved settings. No Python, downloads or extra compatibility files are needed by players.

**1.13 mount-fix for ReSkate 1.1.8:** Built directly from JishsFirstPersonMod-1.12-mount-fix-reskate-1.1.7-source.zip with the verified official 1.1.8 bridge profile added. Retains that archive's mount smoothing and original jump/input behavior. Later jump smoothing and held-stick resume edits are excluded. No settings migration is required. Release: JishsFirstPersonMod-1.13-mount-fix-reskate-1.1.8.zip.

**Version 1.12:** Adds the verified ReSkate 1.1.7 bridge profile while retaining 1.1.5 and 1.1.6 support. Camera/input behavior and recommended settings remain unchanged from 1.11. Install the mod DLL/bootstrap only; retain the stock ReSkate DLL. Release: `JishsFirstPersonMod-1.12-reskate-1.1.7.zip`.

**Version 1.11:** Removes the 1.10 predictive mounting state and special camera/Spring handoff. Restores the 1.9 camera and transition behavior, with one additional input gate: the native mounting flag disables custom right-stick/glance look regardless of parkour looking or jump-lock settings. No Y/Triangle prediction, confirmation window, mounting latch or new movement/facing behavior remains. Recommended settings and bail behavior are unchanged. Release: `JishsFirstPersonMod-1.11-reskate-1.1.6.zip`.

**Version 1.9:** Non-bail handoffs now retain the last displayed attached-camera pose through mounting, dismounting, climbing and selected jumps. Resuming native movement or analog look blends from that final pose, including pitch and glance rotation. While head attachment remains active, position follows the head directly and only orientation blends. Foot/board, camera and look smoothing retain their separate controls; attachment turning native Spring off does not erase saved handoff strengths. Parkour looking stays optional and off by default. Bail release keeps the reverted 1.5 behavior. Release: `JishsFirstPersonMod-1.9-reskate-1.1.6.zip`.

**Version 1.8:** Reverts the 1.6 bail handover changes to the 1.5 behavior: native orientation returns immediately when bail evidence clears, with no settle wait or orientation reconciliation. Stick/glance controls retain neutral re-entry after bails. The optional jump/climb looking from 1.7 remains, default off; head attachment and the separate jump toggles are retained. Release: `JishsFirstPersonMod-1.8-reskate-1.1.6.zip`.

**Version 1.7:** **Allow looking during jumps and climbing** is a saved option (`look_during_parkour`), default off and off in the recommended preset. Off restores the pre-1.6 native view during climbing and enabled mounting/on-foot/hippy jump attachments, releasing custom stick/glance control until normal movement returns. On enables the 1.6 camera-only look behavior described below. Head attachment and separate jump toggles remain unchanged.

## Loading and camera ownership

The radio project supplies only the loading architecture: minimal DllMain, a once-only initialization worker outside loader lock, luaopen_jishsfirstpersonmod and a Lua package.loadlib bootstrap. The DLL lives beside the Lua in scripts/Custom; the bootstrap resolves mod.directory (falling back to scripts/Custom/) and uses the same DLL path for every export. Initialization validates Skate.exe, waits for stock graphics readiness, and rejects the old MyMod module.

ReSkate owns camera mode, native spring, animation/render listeners, head hiding and restoration. The DLL samples controller input and submits typed requests through the existing stock queue_debug callback. Its held-look callback chains the validated stock SourceTrial transform data slot without modifying Skate.exe's native camera setter. Relative glance rotation is smoothed separately. A final residual layer blends verified non-bail state handoffs without continuously smoothing native camera position, head motion or offsets again. The FOV feed handles stock's unsupported zero/game-default FOV request.

Stock exposes no exported camera setter. The private bridge retains the verified stock 1.1.5 DLL SHA-256:
822F05DC4CE4AACF2123FF6A29BD595F2FEAA099498936C0B35857FFFB7D91D9

Private stock callback/model/mutex/SourceTrial/FOV contracts are isolated in src/stock_contract.h. Stock-owned models are read under the original recursive render mutex, without transferring STL ownership. Native active/readiness are captured before mirroring UI preferences. When comfort options are selected, the stock menu checkbox reflects the user's master intent while actual native camera activity remains separately tracked. This lets either menu disarm a temporarily suppressed camera.

## Settings, rendering and input

Config adds link_smoothing (default true), independent look_spring enable/strength values and a first-unlink initialization flag. Unlink copies smoothing only; offsets and rotations remain native. Separate commands persist look values without sending them to stock's camera sink. A linkage change during a glance is deferred until neutral. Input latches reset for control changes rather than unrelated sliders.

The DLL owns its ImGui/DX12 resources, verifies the focused swapchain/direct queue from stock's named overlay submission, and chains presentation hooks. Pointer input is isolated within the companion bounds while stock retains general cursor ownership. Existing resize/device-loss handling is retained.

Settings persist to LOCALAPPDATA with immediate option/binding saves and a 750 ms slider debounce. SQLite migration is read-only. Native preferences sync both ways without writing the stock database.

## Documented player state contracts

ReSkate-main.zip documents 100 riding ground (client_noclip.cpp), 103 crouch, 200 pop and 602 boneless foot plant (trainer_jump.cpp). no_bail.h names 300 WipeoutGround and 504 Offboard and explicitly documents remapping 300 to 504. no_bail.cpp identifies ground/air 100..299 and grind 400..499; trainer.cpp confirms air 200..299. Individual names for observed 101/201 are not invented. Definitions and addresses are isolated in src/player_state_contract.h.

The guarded local-player ownership chain is copied from custom.2's no_bail::resolve. It validates world/client, player manager, local/remote flags, entity/backlinks, component/core vtables, physics context, selector, causes and rig ownership. Teleporting and lost ownership invalidate the sample. Physics core identity changes and invalid samples reset bail tracking.

For board-only, documented riding families are accepted and 300/504 are off-board. Native Offboard parent ownership at core+0x3b0, vtable 0x65e8f40, is a stronger off-board signal and takes precedence. Unknown state families suspend changes.

## Bail lifetime

The existing stock selector handler is chained, preserving its original arguments, result and post-call LastError. The game caller is byte-checked and the handler must belong to the pinned stock module. A watched local selector expires after 500 ms. Return 300 is captured before the game's caller converts it to 504. No No Bail cause suppression or physics state modifications are copied. If no verified stock handler is available, animation requests remain the positive wipeout source; failure is logged.

The native publisher proves core+0x3b8 -> +0x38 owns explicit wipeout request +0x87, animation wipeout +0x8d, and recovery/runout +0x9e. Native instruction contracts are validated before guarded reads. Recovery/runout alone does not start a bail.

A positive selector/state 300 or wipeout request latches the bail. Transient request flags clearing cannot end it. Recovery requires validated riding or a documented normal Offboard child: FollowGround (+0x60, vtable 0x65e8c20), FollowGroundSlide (+0x1c0, 0x65e8c80), or Falling (+0x5b0, 0x65e8da0). Active child is parent+0x48; parent context, board-wrapper and rig-wrapper links are verified. Unknown animation/trajectory/ragdoll children do not clear the latch. Wipeout requests must be absent and recovery/runout inactive for two consecutive normal samples before clearing.

Six caller/constructor/publisher byte contracts and three movement vtable/update pairs were checked against the installed Skate.exe. Runtime validation repeats these checks. Bail-only is available when animation and normal movement contracts match; it does not depend on a speculative recovery timeout.

## Camera policy and validation

Master intent is separate from real native active mode. User requests are translated to the eligible native view before queueing, avoiding a first-person pulse when arming while off board or bailing. Automatic requests bypass master mutation. A temporary native disable cannot erase the master. Pending targets are not requeued; rejected writes retry at most once a second, while opposite transitions can act immediately. Clearing the restrictions restores first person when still armed.

Glances are inhibited during comfort suppression. Bails, focus, device and activation changes retain neutral-entry safeguards. Context loss, teleport, respawn and owner/camera changes reset history; unknown state is not assumed recovered.

All 17 automated checks pass, covering camera/input math, loader guards, hook chaining, persistence, native settings/activation, comfort policy, transitions, bail hold/release, hippy exclusions, parkour looking and bounded diagnostics. Live gameplay and multiplayer remain player acceptance checks.

## On-foot stick look

The companion-panel visibility key is a saved Windows virtual-key binding, default F9. The selected game HWND processes fresh key-down events even when the panel is hidden; repeats and key-up never toggle. Hidden state lasts for the game session and gates companion presentation only, retaining stock menu/cursor ownership. Keyboard recording cancels on focus loss, panel closure, Insert or Escape. Hiding clears pointer capture and pending panel events. Recommended-preset application preserves the user's menu key.

The recommended preset is a snapshot of Jish's saved settings shipped beside the DLL under presets/recommended.ini. The updated snapshot includes disable_spring_on_foot=true. Loading it is explicit through the companion panel and uses one validated settings transaction: current stock reserved chords are retained and conflicts reject the whole transaction. Camera/FOV changes use the existing camera sink/native queue; no activation request is generated. The settings worker saves it immediately. Missing or invalid preset files produce panel feedback without changing configuration.

Camera submission rechecks live Offboard ownership, animation/head validity and active bail evidence. Camera-look eligibility is independent of character-facing eligibility and head-position attachment. Verified special children permit camera-only looking without an invented climbing ID; normal ground permits facing writes. Invalid ownership/head data releases input capture and uses native position. Board right-stick axes remain native.

A final camera handoff layer records the displayed transform after head attachment and glance composition, then decays position and quaternion rotation residuals toward the live stock destination on verified foot/board transitions and stick activation/release. Physics and owned Offboard child address/vtable changes also seed the same residual for animation handoffs, including changes inside riding mode. Camera blend eligibility is independent of custom stick eligibility. Animation handoffs use camera Spring; stick handoffs use the selected look settings. Foot/board handoffs work with stick disabled and use independent mount/dismount strengths (70/35 percent by default) and an independent enable checkbox, without re-integrating ordinary native movement. The destination state chooses the strength; simultaneous events prioritize foot/board, then animation, then stick handoffs. Completion thresholds are 1 mm and 0.1 degrees. Reversed transitions seed from the last displayed pose; same-frame callbacks do not integrate twice. Invalid state, bail/recovery, changed owner/camera, time discontinuities and jumps over five metres reset the layer. Displayed base yaw and pitch seed subsequent neutral stick activation, removing the current glance offset once. Valid action input suspension retains transition history; direct action poses suspend residuals while recording each displayed pose. Attachment/look eligibility changes can start handoffs within the same movement state. Attached position bypasses residuals while rotation can blend. The three transition settings persist through the existing debounced settings store; native input ownership is unchanged.

Controller samples expose normalized axes without replacing latch behavior. Tick-owned analog yaw/pitch continues across normal movement, climbing and jumps when parkour looking is enabled. Bail, activation, focus and device changes retain neutral entry. Analog and glance smoothing remain independent.

The stock SourceTrial transform callback publishes the original pose heading. During active stick look the camera feed constructs absolute world-up yaw and horizontal-axis pitch, keeping the horizon level independently of native head tilt and character rotation. Native camera position, including spring-arm offsets, and matrix metadata are preserved; inactive stick look uses native orientation except during the active bail hold. A body-to-camera yaw bias is sampled on activation to preserve the existing anatomical orientation.

Camera and character chains the same native skater_motion function used by custom.2 noclip (RVA 0x4776a80, ABI void(rig,context,matrix,byte)). Disassembly confirms native on-foot FollowGround and FollowGroundSlide call this rig motion target consumer. The original routine consumes the modified facing matrix through its native rig/physics processing. Only orientation is changed: position, surface-up axis, SIMD metadata and flags are retained. This is not an entity teleport or a render-only model rotation. Only the existing hash-pinned stock handler is chained. Early loading waits asynchronously for the stock motion bridge, outside loader lock and game ticks.

Use-time gates recheck local physics/rig/entity links, teleport, validated Offboard ownership and head data, native first-person activity/request, focus/menu state and active bail evidence. Requests expire after 100 ms. Character-facing writes additionally require verified normal ground and no mounting, falling or hippy action. Re-entry rebases facing bias; board input is never captured.

Input chains standard/ordinal-100 XInput state exports and GetRawInputData/GetRawInputBuffer. It neutralizes only the right stick for active selected XInput slots and supported Sony HID reports. Bluetooth report CRC is recomputed after changing the two axis bytes. Unrelated axes, triggers and buttons are unchanged. Native device paths that do not use XInput or these raw-input APIs still require live verification.

The new automated suite covers normalization, radial deadzone, diagonal speed, frame-rate sensitivity, neutral entry, board handoff, mode/device changes, inverted Y, pitch limits, smoothing, absolute camera yaw, native facing matrix preservation, Sony input preservation and settings reload. Native forward-motion alignment, controller routing across real devices, stationary facing, mounting with a held stick and multiplayer remain player acceptance tests.


## Stock 1.1.6 bridge profile

Installed stock 1.1.6 SHA-256 is 5512A15EACD074D8C03972467AF82975FE033A0D5D5A5CF9001C4520FC1F8039. The latest ReSkate-main source confirms unchanged camera action IDs, callback prefix and DebugModel prefix. Binary disassembly verifies status export 0x3a77c0, state pointer 0x8918f0, native mutex lock/unlock 0x6811a4/0x6811d0, FOV writer 0x252030 and SourceTrial initialized/base/transform slots 0x856d80/0x856fb0/0x856fc0. Overlay construction still places callbacks at +0x4b0 and Model at +0x4e0. Both exact profiles live in stock_contract.h; wrong hashes or export RVAs remain rejected. Regression coverage selects both versions and rejects mismatched hashes/exports. This corrects the observed failure before any camera/input bridge was attached.


## Spring on foot

The persisted disable_spring_on_foot option defaults false and uses a separate settings command. Live movement ownership determines the effective camera settings: verified Offboard ownership includes native special children, while invalid ownership uses the configured Spring value. The stock bridge submits only the Spring enable action, retaining offsets, rotations and directional strengths. Automatic switches bypass preference writes; the mirror normalizes the native enable bit to the saved preference while the option is enabled or restoration is pending. Explicit stock-panel Spring requests still update the preference but send the state-appropriate effective enable value. Queued writes retry after one second if rejected or unacknowledged; reversed targets are sent immediately. Disabling the option or leaving first person restores the saved value. The pose feed and linked glance/analog smoothing use the same effective settings; independent look and mount/dismount smoothing are retained.

## Bail and action camera locks

Live wipeout/recovery flags and the bail latch suppress custom look/facing during active bails. Head-position attachment is a separate policy: climbing always attaches, selected jump toggles attach their respective actions, and Disable Spring on foot retains its existing coverage. Attachment bypasses positional Spring/handoff residuals while permitting camera-only look. Invalid head data falls back to native position. Native Spring changes retain the existing queue/preference restoration path.

The persisted lock_camera_mount_jump flag defaults true. With stick look enabled it attaches head position during the verified mounting request and owned Offboard Falling child, excluding candidate/confirmed hippy jumps. lock_camera_hippy_jump independently controls confirmed hippy attachment. Neither captures board trick input or locks camera rotation. Both INI keys and saved values remain unchanged.

Bail diagnostics sample at 20 Hz, keep 20 pre-event samples, capture up to 15 seconds of each bail window and two seconds after its last positive bail sample, and cap queued rows at 1024. The settings worker writes bail-camera-trace-v3.csv with an 8 MiB file limit. Anatomical root/head origins are read from the verified 395-joint pose; hidden Head scale does not affect its own origin. Camera-to-head distance measures anatomical head origin distance, not exact eye-position error. Validity columns distinguish missing samples. Quaternion columns avoid interpreting near-vertical Euler yaw changes as full camera turns. Native/final pose deltas show whether any remaining spin is already present in stock output.

## Version 1.3 bail view and hippy detection

BailView uses the restored 1.5 implementation. It holds the last displayed level direction during active bail evidence and follows the anatomical Head with an offset calibrated within 0.4 m during normal movement. Missing head data uses native position. Clearing bail evidence restores native orientation immediately without a settle wait or reconciliation. Lifecycle discontinuities reset history.

HippyJumpDetector advances only on controller ticks. It observes verified riding 100 -> Offboard 504 transitions, accepts two successive Falling samples rising above 0.5 m/s within 250 ms, then stays confirmed until landing/remount, bail or lifecycle reset. Vertical speed comes from the validated skater world transform. Camera callbacks only read owner-matched confirmation snapshots. Candidate/confirmed hippy jumps are separated from the generic on-foot Falling lock. The persisted lock_camera_hippy_jump toggle defaults false, requires stick look enabled, and uses Spring-off current calibrated Head position attachment while allowing custom camera rotation; ordinary board air states never arm the detector.

The v3 diagnostic CSV preserves existing trace files and schema. Removed reconciliation/manual-takeover fields now write zero; head attachment and facing eligibility remain recorded. package.ps1 defaults to 1.12 / ReSkate 1.1.7 and produces versioned release/source ZIPs.

Version 1.5 updates the bundled recommended.ini from the current saved settings, including lock_camera_hippy_jump=true. Parser defaults remain unchanged; applying the recommended preset explicitly enables that option.


## Stock 1.1.7 bridge profile

Installed DLL SHA-256: C501C57D12219BD124D7E6B889231D7C2342546E9F38B9D947FCF841668558DB. Verified disassembly maps overlay status to 0x3ba7c0, state pointer to 0x8a6950, recursive mutex lock/unlock to 0x693f14/0x693f40, FOV writer to 0x265040, and SourceTrial initialized/base/transform to 0x86bdd0/0x86c000/0x86c010. Source and binary retain callback/model offsets +0x4b0/+0x4e0, DebugModel prefix/status offsets and camera request IDs. Game camera setter RVAs remain unchanged. The locally extended glance math remains mod-owned; upstream removal of its look parameter does not change the stock Settings ABI. Exact hash and status RVA checks remain mandatory.

## Mount-fix source rebuilt for ReSkate 1.1.8

Base source is the supplied JishsFirstPersonMod-1.12-mount-fix-reskate-1.1.7-source.zip. Camera and input source files are byte-identical to that archive; later jump-camera and action-resume changes are not included.

Official ReSkate 1.1.8 DLL SHA-256: 6336A00B0A06178BEC069F0AF0F3922CC3A89C0E29B2B2EE363701DA4C63757D. Verified bridge RVAs: status 0x3ba7e0, state pointer 0x8a6950, recursive mutex lock/unlock 0x693f34/0x693f60, FOV writer 0x265060, SourceTrial initialized/base/transform 0x86bdd0/0x86c000/0x86c010. Official tagged source and DLL retain callback/model offsets +0x4b0/+0x4e0, camera action IDs and camera settings ABI. Prior exact-hash profiles remain supported. No new saved settings or migration. In-game validation remains a player acceptance check.
## Faster maintainer hook updates

Run tools/inspect-hooks.ps1 with a new release DLL/source and a verified baseline DLL/source. It reads the compiled NativeContract table, traces the status/state/mutex path, compares relocation-masked instruction patterns and verifies camera data slots and source layouts. Success prints the eight old/new addresses and a ready-to-paste table entry; missing or ambiguous evidence prints a failure without an entry. See tools/README.md for setup and the short inspect/edit/build/package workflow. No runtime code, camera behavior, external profiles or player installation steps were changed. The new inspector was checked in both directions against official 1.1.7 and 1.1.8 DLLs and their tagged source trees.
## Automatic compatibility (1.14)

The C++ resolver reads a bounded PE image snapshot and anchors at the named status export before calling it. Generated ABI-family recognizers preserve required instruction forms and data offsets while masking relative addresses and the getter's unrelated total-object allocation size. Detours supplies instruction lengths; explicit recognizers decode only observed relationships. State accessor/mutex/constructor, FOV writer, SourceTrial initializer and read-model/settings-queue callbacks must match uniquely. Constructor callback/model fields, executable sections and writable pointer slots are checked separately. Compiled profile addresses are checked against the resolved structure; unknown hashes use the process-local resolved contract. No disk cache, external profile or network access is used.

After graphics readiness, the live model prefix and stable callback pointers are checked under the validated mutex. Callback instruction shapes are rechecked before installation. SourceTrial initialization waits on the initialization worker; zero means not ready, while an invalid flag, game base or transform callback is incompatible. Feature gates remain off until startup completes. Failed installation restores the mod-owned transform slot by compare/exchange and removes only targets recorded by this DLL after successful preparation; another replacement is left untouched. Failed detachment keeps features inactive and retains relay lifetimes.

Maintainer check: build/Release/bridge_resolver_tests.exe ../upstream exercises compiled and forced automatic resolution for the official four DLL fixtures plus negative cases. Set JISHS_FP_FORCE_DETECTION=1 in the game's launching environment to exercise runtime detection even on a compiled profile. This is a diagnostic environment override, not a saved player option. Clear it for the normal compiled-profile path. Both startup paths still require gameplay smoke testing.

Patterns are checked in as src/bridge_patterns.h; tools/generate_bridge_patterns.py reproduces them from official fixture DLLs. Python dependencies remain maintainer-only. The offline inspector now tolerates the demonstrated unrelated allocation-size differences and tests consecutive 1.1.5-1.1.8 updates. Changes to required layouts, signatures, unknown instruction forms or the supported Skate game build still require an explicit mod update. Automatic detection is compatibility within an observed ABI family, not a guarantee for every future release.

## Game camera ownership (1.15)

A new independent spectator ownership observer preserves the First Person master preference and all existing restrictions. It is enabled for verified ReSkate 1.1.8 only. Replay pre-entry and general scripted/game-mode takeover contracts remain unimplemented; see GAME_CAMERA_OWNERSHIP.md for signals, limitations and acceptance checks.

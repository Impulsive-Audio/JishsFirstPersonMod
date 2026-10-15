# On-board Right Stick Free Look

Included with the controller menu changes in one contribution against Jish's
First Person Mod 1.32 source (ReSkate 2.0.7 / Skate build 25414733).

First person only. Enable on-board Right Stick Free Look at the bottom of
Camera. Hold the configured modifier (LB/L1 initially), centre the stick, then
look. Modifier Key Keeps Right Stick Input Held retains the native skating
position sampled at modifier press while subsequent stick movement looks around.
Release passes the current physical stick position back without mod interpolation.

Fresh/default settings: horizontal sensitivity 330; vertical 270; horizontal
limit 85 degrees each side; vertical limit 60 degrees about center pitch (also
capped at absolute ±85 degrees); manual look smoothing 12%; held skating input
on; right-stick-release auto-center off; modifier-release auto-center on; auto-center
smoothing 35%; center pitch 0 degrees. Feature is disabled by default; saved choices are preserved.
Existing saved values are preserved. Loading the recommended preset imports the
new board defaults along with that preset's existing other preferences.

Modifier-release auto-center takes priority even with a deflected stick. Manual
look smoothing and auto-center smoothing are independent. Button glances retain
their own angles. The small shared-modifier glance rearming improvement is kept.
All settings save automatically and are included in personal camera presets.

Gameplay-tested by the user through v2. The new default values were checked by
automated tests. The merged 1.32 integration requires a fresh gameplay check.

Default modifier: LB. Default deadzone: 10%. Saved user settings are preserved.

The 1.30 body-flip head lock takes priority over on-board free look, including its entry/exit blends. Native right-stick trick controls are restored while it owns the camera. Spectator camera ownership also suspends local free look.

Continue Auto Center Until New Look Input defaults on. A modifier-release return
continues through modifier re-press/release until complete or actual accepted
camera movement begins. A held native trick position does not count as camera
movement before the stick is centered. Turning this option off restores the
previous re-press behavior. Lifecycle/camera ownership resets still take priority.
The toggle and all board settings save/load with personal camera presets;
menu keyboard/controller shortcuts remain global.

On-board free look now rotates the stick look in the skater body's own axes,
layered over the untouched native head-driven camera. The controls anchor to
the player as a whole, so carving banks, ramp pitch and trajectory turns
carry the look, while the head's animation keeps its native feel and no
longer shifts what the stick means. Capture and release are seamless: the
look starts and ends exactly on the native view. Fakie stance is read from
the skater state's stance-signed blend (positive regular, negative fakie)
and mirrors the pitch axis there, since the body's nose opposes the view
in fakie. The mirror itself eases over 200 ms while looking, so a stance
flip during an active look swings through level instead of snapping.
Bail and recovery look remain head-relative as before.

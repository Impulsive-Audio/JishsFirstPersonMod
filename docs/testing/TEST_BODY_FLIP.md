# Body-flip gameplay checks

Status: pending in-game validation on ReSkate 2.0.7. Local test packages only.

- Enable Camera > Attachment and transitions > Lock head during on-board body flips. Verify off by default and disabled in compatibility mode.
- Test front flips, back flips and sideways rolls with and without grabs. Check smooth 250 ms entry, direct head following, and smooth exit.
- Test ordinary jumps, upright spins, kickflips and shuvits: no head lock. Test large transitions below 80 degrees and near-vertical transitions above it; record false positives.
- Test slow and accelerated flips, repeated flips in one flight, rotation settling before landing, landing mid-entry, dismounts, and live disabling. Check rearming after returning below 60 degrees and on the next flight.
- Confirm right-stick look may be disabled while body-flip lock remains functional; mounting/hippy-jump switches remain independent. Controller trick input must continue while glances are paused.
- Test bails/recovery, pause menu, focus loss, spectator/freecam ownership, respawn, teleport and missing head/body pose data: discard stale lock/blend history.
- Save/load a personal preset containing the option and load an older/recommended preset: older setups leave it off.
- Inspect bail-camera-trace-v11.csv phase/tilt/rate alongside native/final poses and mod.log transitions. Confirm configured camera offsets are preserved and native stabilization/Spring settings restore.

Record outcomes and trace evidence.

Fast-flip trace review (13 October 2026): the latest trace session beginning at 11803453 ms contains entry errors up to 0.86 m relative to the configured head target. Fully locked samples match that target at logged precision, including body rotation rates up to 568.54 degrees/second. The world-space handoff anchor allowed the moving head to pull away during entry. Handoffs now carry a decaying position/rotation residual in the destination's local frame, following its current motion throughout the same 250 ms interval. Regression tests exercise both pitch directions with simultaneous translation and fast rotation; gameplay verification of this revision remains pending. The trace does not label front versus back flips, so direction-specific attribution requires gameplay observation.

Front-flip offset fix: the flip target uses animated head pose plus configured position/rotation offsets directly. It no longer inherits the transient native-camera/head delta used by bail calibration. Regression tests cover both pitch directions, zero offsets, configured offsets and a Spring-lagged normal camera. Gameplay validation of this fix is pending.

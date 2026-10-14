# Jish’s First Person Mod 1.33 — ReSkate 2.0.7

## New features

- **On-board right-stick free look:** hold your configured modifier, centre the stick, then look around. Includes sensitivity, angle limits, smoothing and auto-centering settings. Enable it under Camera.
- **Body-flip head lock — WIP:** optionally follow your character’s animated head during on-board body flips. This still has issues with front flips.
- **First-person spectator view:** watch party players in first person during throwdowns, Game of Skate and manual spectating. Enable it under Camera, then use your existing First Person binding to switch between follow and head view.
- **F9 menu toggle:** open and close the mod panel independently after opening the Insert menu once per game session.
- **Controller menu navigation:** use the D-pad or left stick to navigate, A/Cross to select, B/Circle to go back, and LB/RB to switch tabs.

## Fixes and changes

- Fixed spectator initialization on affected installations.
- Separated button-glance smoothing from on-foot right-stick smoothing.
- Removed bald mode and its cosmetic hooks.

## Known issues

- Body-flip head lock remains WIP, particularly during front flips.
- Open Insert once before using F9 to display the panel independently.
- Bail camera collision remains WIP.

## Installation

Close Skate. Extract the install ZIP into the ReSkate folder beside Skate.exe and ReSkateLauncher.exe, merging Mods and scripts and replacing this mod’s existing files. Enable Jish’s First Person Mod in the launcher. Existing saved settings and personal presets are preserved.

The install ZIP contains the mod; the `-source.zip` contains source and build tools.

All **36 automated tests pass**.
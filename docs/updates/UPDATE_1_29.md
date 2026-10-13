# 1.29 - Personal presets and clearer settings

## Settings menu

General, Controls and Camera tabs now sit below the title/version, above First Person and presets. Equal-width square tabs use ReSkate blue for the selected tab and grey for inactive tabs. The existing ReSkate colors and embedded Montserrat/Permanent Marker fonts are retained.

Settings are grouped into scrolling tab content. Detailed explanations use hover help; disabled-option reasons and binding warnings remain visible.

## Personal presets

Save camera, comfort, stick and glance settings plus controller bindings as named personal presets. Load, Save as, Overwrite and Delete are available below the tabs. Overwrite and Delete require confirmation. Jish's Recommended remains available, read-only and unchanged.

Loading keeps the current First Person activation, panel shortcut, compatibility mode, native controls and camera-cycle restoration state. Existing user settings are retained. Personal presets are stored under `%LOCALAPPDATA%/JishsFirstPersonMod/presets`.

## Feedback

Current settings save automatically without overwriting personal presets. Pending writes show "Saving current settings". "Current settings saved" stays visible for three seconds after a successful write. Explicit preset confirmations name the preset and disappear after four seconds. Save failures retain a persistent explanation and retry button; camera-sync failures remain visible.

## Validation

All 32 Windows tests pass, including preset persistence, failed-write preservation, shared camera synchronization, confirmation timing, recording cancellation and preset text input. Narrow panels were rendered and inspected with embedded fonts at 100%, 150% and 200% DPI.

The user reported successful in-game testing of presets and the menu. The final three-second confirmation timing and exhaustive bail/recovery, camera-cycle restoration and focus-loss scenarios have not been individually confirmed in-game.

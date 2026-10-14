# Scripts data layout

Active user data now lives beside the installed scripts:

- scripts/Custom/FirstPerson/settings.ini — saved settings
- scripts/Custom/FirstPerson/presets/*.ini — personal presets
- scripts/Custom/FirstPerson/presets/bundled/recommended.ini — bundled recommendation
- scripts/Custom/FirstPerson/logs — current logs, traces and rotations
- scripts/Custom/FirstPerson/logs/legacy-appdata — historical AppData logs

On first initialization, settings, personal presets and historical logs are copied
from the previous LocalAppData/JishsFirstPersonMod folder. Existing destination
files take precedence. Legacy originals stay as backups. A completion marker
prevents deleted presets from being reimported on later launches. Migration
failure leaves the marker absent so it can retry; filesystem errors are logged.
The game folder must be writable to save settings, presets and logs.

This local build also includes the refresh-presets button, marker-glance memory
option and Camera and character dismount/bail recentering test changes. These
still require gameplay testing. GitHub release 1.33 is unchanged.
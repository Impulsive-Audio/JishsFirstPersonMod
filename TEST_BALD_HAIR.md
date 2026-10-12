# Temporary bald hairstyle — local 1.27 test 3

This package includes the collision test 2 fix and adds **Bald in first person**
(enabled by default). It uses Skate's smooth bald hairstyle in place of the
scalp-hair item on the live character. Hair colour, facial hair, eyebrows and
other clothing slots are retained. The saved outfit manager/database is not
edited. ReSkate's native cosmetic copy schedules the mesh update, so the swap
and restoration may take a few frames.

Install with Skate closed using the current mod package layout. Keep ReSkate
2.0.7 installed. No GitHub publication has been made.

Test with your long hairstyle:

- Enter first person, look around and bail. Hair should stop entering the view.
- Return to third person. Confirm the same hairstyle and colour return.
- Enable third-person bails and check both directions of the camera switch.
- Turn off **Bald in first person** while active; your hair should return.
- Respawn and switch views several times, including rapid switches.
- Open the first-person menu, customize hair/clothes and return to play.
- Return to third person, restart the game and confirm the saved hairstyle.
- Check your hats, beard and eyebrows remain as selected.

The swap is performed through the existing client update, after ReSkate's own
update. The game function is checked against its complete inspected code before
use. The installed bald asset must match the scalp-hair slot category. Busy
appearance updates are retried on later game updates; the original hair is kept
until restoration succeeds. Replacement actors start from their own outfit.

Automated tests cover the hair state/persistence; native mesh changes and actual
gameplay need this local test. Logs contain hairstyle application/restoration
and reasons an update is waiting.

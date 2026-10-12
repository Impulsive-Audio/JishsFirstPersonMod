# Third-person getting-up test8

Enable **Third person while getting up** with **Third person while bailing**
off. The new setting defaults off and is saved as `third_person_recovery`.
Compatibility mode disables the option without discarding its saved value.

The existing verified movement/get-up handler starts a recovery latch. The
existing comfort queue temporarily selects ReSkate third person and preserves
master first-person intent. Clearing animation get-up flags alone does not
release the latch: verified normal control or ordinary movement does. Changed
characters, unavailable player state, disabling the setting and renewed falls
clear it. The existing camera suppression resets bail and collision history.
Board-only preference, manual camera toggling and game-owned cameras retain
precedence. Camera-cycle override uses the existing saved preference mechanism
for either automatic third-person option.

Gameplay checks:

- Long first-person fall followed by getting up: only getting up is third person.
- Face-first floor contact followed by getting up: no sphere remains on the floor.
- Return to walking/on-board control and verify first person resumes.
- Repeat bails, respawn, change maps, disable the option mid-recovery and manually
  turn first person off during recovery. Manual off must remain off afterward.
- Check both bail options, compatibility mode, board-only mode and save/reload.
- Bald hairstyle should restore in third person and resume when first person
  returns. Other players should continue seeing the original hair.

Includes test7's first-person recovery path for continued testing with this new
option off. ZIP contains installation files only. No push or publication.

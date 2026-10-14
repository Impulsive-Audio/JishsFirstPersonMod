# Character-lock head position correction test4

Test3 extended the shared handoff residual lifetime, including positional residuals. For camera-and-character right-stick mode, transition smoothing now affects direction only. The position supplied by the existing camera/head/offset calculation is preserved exactly during mount and dismount, including when stick look rearms.

No saved offset, base rotation, preset, or bail/dismount recenter values are changed. Test3 Insert/F9 behavior is retained.

Automated regression covers moving camera targets at low/high strengths, mount lock entry/release, board mode, dismount, and delayed character-lock rearming. Each frame asserts that all three camera position coordinates match the supplied configured target while direction still blends.

In-game: compare standing/riding head position against test2, mount and dismount with character lock enabled, then compare low/high smoothing strengths. Check custom configured offsets too.
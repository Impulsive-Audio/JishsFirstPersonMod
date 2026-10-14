# Independent menu and controller controls

Included with on-board free look in one combined contribution against the upstream 1.32 source.

Open Insert once per game session, then use F9 to toggle the companion panel independently. Insert continues to open ReSkate's menu. Keyboard panel bindings remain available in Controls. Controller shortcuts for opening the panel are commented out; controller navigation remains available.

D-pad or left stick navigates, left/right adjusts sliders, A/Cross selects,
B/Circle backs out of editing/popups or closes the panel, and LB/RB switches
General/Controls/Camera. Gameplay input is blocked while the rendered panel owns
input; the menu's private controller reader remains available. Hide the panel
to operate ReSkate's stock menu. Keyboard/mouse preset text entry remains available.

The renderer captures the exact Direct3D 12 creation queue and swapchain, including
ReSkate's public BindDx12 registration. Startup hooks run before the graphics
readiness wait so initial creations/submissions can be learned. Named ReSkate
submissions are a verified fallback and are retained for later independent draws.
No arbitrary same-device graphics queue is selected. Readiness and supported-build
checks still gate camera features. The shared GPU command list is fenced before
reuse. Neither ReSkate itself nor game-specific address tables were changed.

Reference: the supplied Dolly reviewed source's shortcut behavior and MenuFix
handover's independent-queue design. The MenuFix implementation source was not
provided and controller navigation was disabled in the supplied source; those
parts are implemented here in Jish's existing menu/input architecture.

Automated validation covers shortcuts, held-edge suppression, focus, reconnect,
binding capture, persistence, real ImGui navigation/back, and Xbox/Sony/DirectInput
menu isolation. Live startup, cursor behavior, controller feel, swapchain changes
and coexistence with the user's installed overlays still require an in-game check.

Install the complete combined ZIP with the game closed. Both dinput8.dll and the
companion DLL changed; replace both through the ZIP's normal folder layout.
Restart, reach gameplay, press Insert, then record/test your shortcut.

The Camera tab places Off-Board Right Stick Free Look directly above On-board
Right Stick Free Look. Off-board look uses Jish's original effective Spring
settings, including Disable Spring on foot. The added independent off-board
smoothing checkbox and slider have been removed. Older saved override keys are
ignored and are dropped when settings are next saved.
Scrollable menu content shares navigation with its parent, so controllers move
straight to widgets without selecting the content container first.
Existing saved keyboard bindings remain intact; rebind to Insert in Controls.

An optional JishFirstPersonMenuQueueForSwapchain export lets SkateDesigner borrow
only this renderer's verified exact swapchain/queue association. The returned
queue carries an owned COM reference. It does not alter camera or input settings.

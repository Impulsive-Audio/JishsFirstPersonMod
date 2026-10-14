# Menu and controller controls

Press Insert to open ReSkate and the first-person panel together. F9 hides or shows the companion while Insert is open. Closing Insert closes both; reopening Insert restores the companion. Standalone opening and controller menu shortcuts are disabled, and the menu keys are fixed.

The renderer is restored from commit cef1b1b6a557271fe99dd56072592941b3cd8cb7 (1.29.1-reskate-2.0.7). It draws through ReSkate's stock overlay submission path, with GPU fences protecting command-list reuse. It does not draw the panel outside Insert or install cursor clipping/warping hooks.

D-pad or left stick navigates, left/right adjusts sliders, A/Cross selects, B/Circle backs out of editing/popups or hides the panel, and LB/RB switches General/Controls/Camera. Keyboard/mouse preset text entry remains available. Clicks outside the companion reach the ReSkate menu; F9 can also hide the companion to give ReSkate the full screen.

Install the complete package with Skate closed, using its normal folder layout. Existing saved camera/controller preferences are retained. Older saved menu binding values are retained for compatibility but do not change Insert/F9 behavior.

Off-board right-stick look uses the effective camera Spring settings, including Disable Spring on foot. Character-lock bail/dismount recenter faces outward; mount/dismount smoothing in that mode affects direction while preserving the configured camera position. Controller navigation remains available within scrollable menu content.

The automated suite covers controller navigation, input routing, settings persistence, camera transitions and data migration. The user confirmed test7 restores working menu opening in-game. Test documents for earlier numbered builds describe historical behavior; test7 is the current menu baseline.
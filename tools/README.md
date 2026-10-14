# Updating ReSkate hooks

The mod still uses compiled, exact-hash compatibility profiles. These tools only
read release files and print an entry; they never load DLLs, edit the mod or download files.

One-time setup, from the source directory (Python 3.10+):

```powershell
python -m venv .hook-tools
.hook-tools/Scripts/python.exe -m pip install -r tools/requirements.txt
```

For each update:

1. Obtain the official new DLL and its matching tagged source. Keep the previous
   verified DLL and its matching source as the baseline. Source paths point at
   the extracted repository root containing `Engine` and `Extension`.
2. Run the inspector (replace the example paths/version):

```powershell
tools/inspect-hooks.ps1 -Python .hook-tools/Scripts/python.exe `
  -Dll C:/Releases/1.1.8/ReSkate.dll -Version 1.1.8 `
  -Source C:/Releases/1.1.8/source `
  -Baseline C:/Releases/1.1.7/ReSkate.dll `
  -BaselineSource C:/Releases/1.1.7/source
```

3. Review the address report. On success, paste the emitted entry into
   `src/stock_contract.h`. Keep older profiles. If the tool reports a missing or
   ambiguous match or changed source layout, investigate the named contract;
   do not infer addresses from a uniform shift. The tool checks the unchanged
   bridge implementation, not every possible upstream behavioral change.
4. Run `./build.ps1`, then `./package.ps1 -ModVersion <mod-version> -ReSkateVersion <new-version>`.
   Smoke-test activation, on-board glances, on-foot look and mounting in game.

Address-only updates need no other hook edits or version-specific test assertions.
Update package defaults and release notes when preparing the new release.
Source/API changes can require updating the bridge and inspector together.

Tool tests: `.hook-tools/Scripts/python.exe tools/test_inspect_hooks.py`.
Optional round-trip checks use `--fixtures <folder>` containing the official
1.1.7 and 1.1.8 DLL/source layout used in this workspace's `upstream` directory.
Dependencies are maintainer-only; players need no Python installation.

## Additional 1.19 hook verification

With the listed Python dependencies installed, run `python verify_native_interop.py path/to/reskate.dll`. This read-only check validates the exact 2.0.0/2.0.1/2.0.2 release hash, native sampler and console-queue prologues, and two sampler call destinations. It complements `inspect_hooks.py`; it does not prove runtime hook safety or in-game camera behavior.

For controller recording, run `python tools/verify_recording_reader.py path/to/ReSkate.dll`. This also includes the exact 2.0.2 reader and menu-enabled call sites.

## 1.30 spectator verification

Run `python tools/audit_spectator_bridge.py path/to/ReSkate-2.0.7.dll` to verify the exact image, private state getter/data references, remote storage stride, render hook and party camera/FOV call sites. This is read-only and does not load the DLL. Compile `tools/spectator_layout_probe.cpp` against the official 2.0.7 source with MSVC release STL layout to validate the POD offsets. See [spectator test results](../docs/testing/TEST_SPECTATOR.md). Other ReSkate versions are deliberately unsupported by this spectator bridge.


Current development source: private spectator, Sony HID and cosmetic hooks target verified ReSkate 2.0.7. Use audit_spectator_bridge.py and verify_glance_hid.py for this target. See docs/RESKATE_2_0_7_REVERT.md.

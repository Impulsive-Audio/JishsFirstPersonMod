# Native Sony startup correction and 1.28 validation

The native USB launch failures stop after HID hook activation. Source inspection
found a lock cycle: HID diagnostics held native_hid_mutex while acquiring the
logger mutex; startup logging held the logger mutex while hooked CloseHandle
acquired native_hid_mutex. No fresh crash dump establishes the runtime stack.

HID identity/report/consumption diagnostics now accumulate locally and flush
after all input locks release. Diagnostic file I/O has a nested thread-local
scope shared by the production logger and native adapters, bypassing controller
bookkeeping. CloseHandle clears gesture state only for tracked Sony handles.
Pending ReadFileEx callbacks remain deliverable after close/cancellation.
Last-error preservation, batched/oversized report reads, double taps and latched
enter/exit suppression remain covered by existing regressions.

The controller regression uses the production file logger with real Windows
hooks. It verifies at least 300 marked logger closures traverse CloseHandle,
logging completes while both input locks are held, concurrent startup/HID
diagnostic batches complete, and an ordinary file closes while glance_mutex is
held. CTest executes it in a child process with a 15-second timeout. The full
Release suite has 31 tests, including XInput, Sony USB/Bluetooth, DirectInput,
completion ports, callback cancellation, ordinary files and rotating logs.

## User-confirmed live validation

On 2026-10-13, the user confirmed test3 working with DualSense Edge,
standard DualSense and XInput, and authorized the 1.28 release. This clears the
release hold. The report did not give launch counts or a per-scenario checklist;
do not infer those details. The regression suite provides the automated checks
listed above. Bundled ReSkate DLL and settings remain unchanged.

"""Extract only the proven native camera/input code from the supplied source ZIP.
This never edits or builds ReSkate itself.
"""
from pathlib import Path
from zipfile import ZipFile
import hashlib, json

root = Path(__file__).resolve().parents[1]
archive = root.parent / 'ReSkateCustom/ReSkate-1.1.4/release/ReSkateCustom-1.1.4-custom.2-source.zip'
files = [
 'LICENSE', 'Engine/Core/Hooks/hooks.cpp', 'Engine/Core/Hooks/hooks.h',
 'Engine/Core/Platform/memory.h', 'Engine/Game/Build/addresses.h',
 'Engine/Game/Build/supported_build.h', 'Engine/Game/Build/image_identity.h',
 'Engine/Game/Build/image_identity.cpp', 'Engine/Game/Build/20260929/engine.h',
 'Engine/Game/Build/20260929/runtime.h', 'Engine/Game/Build/20260929/camera_observer.h',
 'Engine/Game/Build/20260929/client_source_spawn.h', 'Engine/Game/Build/20260929/native_skater.h',
 'Engine/Game/Input/controller_bindings.h', 'Engine/Game/Input/playstation_report.h',
 'Engine/Game/Skater/first_person_spring.h',
 'Extension/Skater/client_source_spawn_internal.h', 'Extension/Skater/client_first_person.cpp',
 'Extension/Skater/free_flight.h', 'Extension/Skater/source_spawn_camera.cpp',
 'Extension/Multiplayer/Remote/native_pose_layout.h',
 'Extension/UI/Overlay/playstation_input.cpp', 'Extension/UI/Overlay/playstation_input.h',
]
manifest = {}
with ZipFile(archive) as z:
 for n in z.namelist():
  if n.startswith('External/detours/') and not n.endswith('/'): files.append(n)
 for n in files:
  data = z.read(n)
  out = root / 'vendor' / n
  out.parent.mkdir(parents=True, exist_ok=True)
  out.write_bytes(data)
  manifest[n] = hashlib.sha256(data).hexdigest()
 # Extract exact native functions; isolate unrelated freecam/noclip/profile code.
 debug = z.read('Extension/Skater/client_debug.cpp').decode().replace('\r\n', '\n')
 def extract(start, end): return debug[debug.index(start):debug.index(end, debug.index(start))]
 (root/'src/proven_debug.cpp').write_text('''#include "native.h"
#include "Extension/Skater/free_flight.h"
namespace dingosdk::client_source::detail {
''' + extract('double source_flight_now()', 'namespace {') +
 extract('std::array<float, 3> debug_position(', '\n}\nstd::array<float, 16> debug_skater') +
 '\n' + extract('std::array<float, 16> debug_skater(', '\nnamespace {') + '\n}\n')
 camera = z.read('Extension/Skater/source_spawn_camera.cpp').decode().replace('\r\n', '\n')
 camera = camera[:camera.index('\nnamespace dingosdk {')]
 camera = camera.replace('#include "client_source_spawn.h"', '#include "native.h"').replace('#include "Engine/Game/UI/game_view.h"','')
 (root/'src/proven_camera.cpp').write_text(camera)
 fp = z.read('Extension/Skater/client_first_person.cpp').decode().replace('\r\n', '\n')
 fp = fp.replace('#include "client_source_spawn.h"', '#include "native.h"')
 fp = fp.replace('#include "Extension/Multiplayer/Remote/native_skater.h"', '#include "mod.h"')
 fp = fp.replace('multiplayer::set_local_hidden_joint(', 'jishsfirstpersonmod::set_hidden_joint(')
 fp = fp.replace('void first_person_hide_head(std::uintptr_t base, std::uintptr_t component) noexcept {',
 '''void first_person_hide_head(std::uintptr_t base, std::uintptr_t component) {
    if (!jishsfirstpersonmod::begin_head_hide()) return;
    struct VisibilityScope { ~VisibilityScope() { jishsfirstpersonmod::end_head_hide(); } } visibility_scope;
    if (first_person_arm().watched.load(std::memory_order_acquire) != component) return;''')
 fp += '''
namespace jishsfirstpersonmod {
void hide_current_head() noexcept {
    using namespace dingosdk::client_source::detail;
    auto& arm = first_person_arm();
    const auto component = arm.watched.load(std::memory_order_acquire);
    if (component) {
        try { first_person_hide_head(arm.watched_base.load(std::memory_order_acquire),component); }
        catch (...) { /* The next tick reports unavailable head data. */ }
    }
}
}
'''
 # Ensure the render handoff publishes the just-evaluated head before the native render publishes it.
 (root/'src/proven_first_person.cpp').write_text(fp)
 input_source = z.read('Extension/UI/Overlay/overlay_input.cpp').decode().replace('\r\n','\n')
 input_code = input_source[input_source.index('struct ControllerSample {'):input_source.index('dingosdk::overlay::FlightInput read_player_flight_controller()')]
 input_code = input_code.replace('OverlayInputAccess access;', '/* Private reader: the stock capture hook still filters menu input. */')
 (root/'src/proven_input.cpp').write_text('''#include "mod.h"
#include "Extension/UI/Overlay/playstation_input.h"
#include <Xinput.h>
#include <atomic>
#include <mutex>
#include <cstring>
namespace dingosdk::overlay::detail {
namespace {
''' + input_code + '''
}
}
namespace jishsfirstpersonmod {
void prepare_input() {
    using namespace dingosdk::overlay::detail;
    (void)xinput_get_state();
    request_xinput_probe(xinput_slots());
    (void)read_playstation_pads();
}
dingosdk::ControllerInput read_controller() {
    using namespace dingosdk::overlay::detail;
    const auto window = GetForegroundWindow();
    DWORD pid{};
    if (!window || !GetWindowThreadProcessId(window,&pid) || pid != GetCurrentProcessId()) return {};
    bool visible{};
    if (!overlay_available(visible) || visible) return {};
    const auto sample = read_controller_sample();
    dingosdk::ControllerInput out{};
    out.style = sample.style;
    if (!sample.device) return out;
    out.available = true; out.device = sample.device;
    out.buttons = sample.pad.wButtons & dingosdk::controller_button_mask;
    if (sample.pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) out.buttons |= 0x10000;
    if (sample.pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) out.buttons |= 0x20000;
    return out;
}
}
''')
 observer = z.read('Extension/Skater/camera_observer.cpp').decode().replace('\r\n', '\n')
 begin = observer.index('bool camera_range(')
 end = observer.index('bool camera_isolated_callback(')
 helpers = observer[begin:end]
 # Only copy identities and their verified layout checks, not ReSkate-owned counters/hooks.
 keep = helpers[:helpers.index('struct CameraTickFrame')]
 identity = helpers[helpers.index('struct CameraIdentity'):helpers.index('struct CameraSample')]
 functions = helpers[helpers.index('std::uintptr_t camera_entry_identity'):]
 (root/'src/proven_camera_identity.inc').write_text(keep + identity + functions)
 # Preserve the custom build's mathematical and latch regression cases verbatim.
 tests = z.read('Extension/Profile/Test/first_person_look_tests.h').decode().replace('\r\n', '\n')
 cases = tests[tests.index('    auto controls = default_look_controls;'):tests.index('\n}\n\nvoid test_look_persistence')]
 preamble = tests[tests.index('    const auto expect'):tests.index('    profile::Snapshot')]
 (root/'tests/proven_behavior.cpp').write_text('#include "Engine/Game/Input/controller_bindings.h"\n#include "Engine/Game/Skater/first_person_spring.h"\n#include <stdexcept>\nint main() {\nusing namespace dingosdk;\n' + preamble + cases + '\n}\n')

(root/'vendor/source-manifest.json').write_text(json.dumps({'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(), 'files':manifest}, indent=2))
print(f'Extracted {len(files)} native/vendor files from the reference archive')

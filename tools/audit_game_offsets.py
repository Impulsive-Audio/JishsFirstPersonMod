"""Read-only check of the game's static hook contracts and camera vtable slots.
Run with the installed Skate.exe path; never loads or edits the game.
"""
import argparse
import hashlib
import re
import struct
from pathlib import Path
import pefile

def audit(path):
    root = Path(__file__).resolve().parents[1]
    pe = pefile.PE(str(path), fast_load=True)
    base = pe.OPTIONAL_HEADER.ImageBase
    checks = 0
    def check(name, condition):
        nonlocal checks
        if not condition:
            raise ValueError(name + ' differs from the reviewed game contract')
        checks += 1
        print('PASS:', name)
    def pointer(rva):
        return struct.unpack('<Q', pe.get_data(rva, 8))[0] - base
    def source(relative):
        return (root / relative).read_text(encoding='utf-8-sig')
    def constants(text):
        return {name: int(value, 16) for name, value in
                re.findall(r'\b(\w+)\s*=\s*(0x[0-9a-fA-F]+)\b', text)}
    def prefixes(text, mapping):
        values = constants(text)
        for name, rva_name in mapping.items():
            match = re.search(r'\b' + name + r'\s*\{([^}]+)\}', text)
            if not match:
                raise ValueError('Missing prefix ' + name)
            data = bytes(int(token, 0) for token in
                         re.findall(r'0x[0-9a-fA-F]+|\b\d+\b', match.group(1)))
            check(name + ' at ' + hex(values[rva_name]),
                  pe.get_data(values[rva_name], len(data)) == data)
        return values
    player = source('src/player_state_contract.h')
    pc = prefixes(player, {
        'caller_prefix': 'caller_rva', 'animation_request_prefix': 'animation_request_site',
        'animation_bail_prefix': 'animation_bail_site', 'recovery_prefix': 'recovery_site',
        'offboard_owner_prefix': 'offboard_owner_site',
        'offboard_constructor_prefix': 'offboard_constructor_site'})
    call = pc['caller_rva'] + 11
    check('pre-remap selector call destination', pe.get_data(call, 1) == b'\xe8' and
          call + 5 + struct.unpack('<i', pe.get_data(call + 1, 4))[0] == pc['choose_state_rva'])
    for offset, vtable, update in re.findall(r'\{(0x\w+),(0x\w+),(0x\w+)\}', player):
        check('normal foot substate ' + offset, pointer(int(vtable, 16) + 0x18) == int(update, 16))
    spawn = prefixes(source('vendor/Engine/Game/Build/20260929/client_source_spawn.h'), {
        'camera_mode_prefix': 'camera_mode_switch',
        'camera_transform_prefix': 'camera_transform_setter',
        'physics_update_prefix': 'physics_update', 'skater_motion_prefix': 'skater_motion'})
    runtime = prefixes(source('vendor/Engine/Game/Build/20260929/runtime.h'),
                       {'client_tick_prefix': 'client_tick'})
    prefixes(source('src/game_camera_ownership.cpp'), {'spectator_prefix': 'spectator_active'})
    for name, offset, target in [
        ('camera_controller_vtable', 0xb8, 'camera_mode_switch'),
        ('free_camera_vtable', 0xd0, 'camera_transform_setter')]:
        check(name + ' slot ' + hex(offset), pointer(spawn[name] + offset) == spawn[target])
    methods = re.search(r'view_manager_methods\{([^}]+)\}',
                        source('vendor/Engine/Game/Build/20260929/client_source_spawn.h')).group(1)
    for offset, target in zip((0, 8, 0x28, 0x98), re.findall(r'0x\w+', methods)):
        check('view manager slot ' + hex(offset), pointer(spawn['view_manager_vtable'] + offset) == int(target, 16))
    engine = constants(source('vendor/Engine/Game/Build/20260929/engine.h'))
    # client_tick is hooked directly by Runtime/runtime_init.cpp; do not assume
    # the engine client vtable contains that address. Its code prefix is checked above.
    uses = '\n'.join(p.read_text(encoding='utf-8-sig') for p in (root/'src').glob('*.cpp'))
    for name in sorted(set(re.findall(r'addr::engine::(\w+)', uses))):
        rva = engine[name]
        check('engine address in image: '+name, pe.get_section_by_rva(rva) is not None)
    print(f'{checks} game code/vtable checks passed; SHA-256 {hashlib.sha256(Path(path).read_bytes()).hexdigest()}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game', type=Path)
    args = parser.parse_args()
    audit(args.game)

"""Read custom.2's SQLite settings into a standalone INI; never change the DB."""
import argparse
import json
import os
from pathlib import Path
import sqlite3
from contextlib import closing

def import_profile(profile: Path, template: Path, output: Path):
    if not profile.is_file():
        raise FileNotFoundError(profile)
    with closing(sqlite3.connect(profile.resolve().as_uri() + '?mode=ro', uri=True)) as db:
        db.execute('PRAGMA query_only=ON')
        rows = {(scope, name): json.loads(value) for scope, name, value in
                db.execute('SELECT scope,name,value_json FROM player_settings')}
    mapping = {}
    for name in ('first_person', 'look_up', 'look_down', 'look_left', 'look_right',
                 'noclip', 'forward_velocity', 'up_velocity'):
        key = ('/settings/bindings', name)
        if key in rows:
            value = rows[key]
            # ReSkate 2.x keyboard hotkeys are not controller masks. Native
            # ReSkate keeps those bindings; this importer never writes its DB.
            if isinstance(value, int) and value & 0xc0000000: continue
            mapping[name] = value
    for name in ('look_up', 'look_down', 'look_left', 'look_right'):
        key = ('/settings/first_person_look_angles', name)
        if key in rows: mapping[name + '_angle'] = rows[key]
    saved = {'fov':'Fov', 'spring':'Spring', 'offset_x':'OffsetX', 'offset_y':'OffsetY',
             'offset_z':'OffsetZ', 'pitch':'Pitch', 'yaw':'Yaw', 'roll':'Roll',
             'strength_up':'Up', 'strength_down':'Down', 'strength_left':'Left', 'strength_right':'Right'}
    for name, suffix in saved.items():
        option = 'ReSkate.FirstPerson.' + suffix
        for scope in ('/settings/gameplay', 'options'):
            if (scope, option) in rows:
                mapping[name] = rows[(scope, option)]
    lines = []
    for line in template.read_text().splitlines():
        name = line.split('=', 1)[0].strip()
        if name in mapping:
            value = mapping[name]
            if isinstance(value, bool): value = int(value)
            line = f'{name}={value}'
        lines.append(line)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines) + '\n')
    return len(mapping)

if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', type=Path,
        default=Path(os.environ.get('LOCALAPPDATA', '.'))/'ReSkate/profiles/offline/reskate.sqlite3')
    parser.add_argument('--output', type=Path, default=root/'release/JishsFirstPersonMod.ini')
    args = parser.parse_args()
    count = import_profile(args.profile, root/'defaults.ini', args.output)
    print(f'Imported {count} settings into {args.output}; source profile unchanged.')

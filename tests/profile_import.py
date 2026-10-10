from pathlib import Path
import hashlib
import importlib.util
import json
import sqlite3
import tempfile
from contextlib import closing

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('profile_import',root/'tools/import_profile.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
temporary_root = (root/'build').resolve()
temporary_root.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(dir=temporary_root) as directory:
    assert Path(directory).resolve().is_relative_to(temporary_root)
    dbpath = Path(directory)/'profile.sqlite3'
    with closing(sqlite3.connect(dbpath)) as db:
        db.execute('CREATE TABLE player_settings(scope TEXT,name TEXT,value_json TEXT)')
        for scope, name, value in [('/settings/bindings','first_person',0x30),
            ('/settings/bindings','look_left',0x100),('/settings/first_person_look_angles','look_left',120),
            ('options','ReSkate.FirstPerson.Spring',True),
            ('/settings/gameplay','ReSkate.FirstPerson.Fov',95),
            ('/settings/gameplay','ReSkate.FirstPerson.Yaw',7)]:
            db.execute('INSERT INTO player_settings VALUES (?,?,?)',(scope,name,json.dumps(value)))
        db.commit()
    original = hashlib.sha256(dbpath.read_bytes()).digest()
    output = Path(directory)/'JishsFirstPersonMod.ini'
    assert module.import_profile(dbpath,root/'defaults.ini',output)==6
    text = output.read_text()
    for setting in ('first_person=48','look_left=256','look_left_angle=120','spring=1','fov=95','yaw=7'):
        assert setting in text, setting
    assert hashlib.sha256(dbpath.read_bytes()).digest()==original
    with closing(sqlite3.connect(dbpath)) as db:
        db.execute("UPDATE player_settings SET value_json=? WHERE name='first_person'", (json.dumps(0x80000046),))
        db.commit()
    original = hashlib.sha256(dbpath.read_bytes()).digest()
    assert module.import_profile(dbpath,root/'defaults.ini',output)==5
    assert 'first_person=0' in output.read_text()
    assert hashlib.sha256(dbpath.read_bytes()).digest()==original
print('Custom.2 profile settings imported without modifying the SQLite file')

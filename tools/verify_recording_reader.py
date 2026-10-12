"""Verify the exact native Binds reader and allow_menu callers, without loading the DLL."""
import hashlib
import sys
from pathlib import Path
import pefile

PROFILES = {
    '8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427':
        (0x3f87e0, (0x410f08, 0x446898)),
    'dff7b0211996b596611019f44e2c6de7ca8c18a6fc65106c0e32100e06520d0f':
        (0x3e8940, (0x4037b8, 0x439138)),
    'ca9c103f85167ac6957e15ed45b999f59afc387d2722277114047f8e04fe6aaa':
        (0x3d00f0, (0x3eab0d, 0x41fed8)),
    '9259c3de99626d6a9d6083b337927bc948ee144dbf4e698f7733148979aeb528':
        (0x3d00f0, (0x3eab0d, 0x420198)),
    '083bd7c9636d7ff1432cb643842ba58159f26cf8ab3bbb1d2c22df322f865912':
        (0x3c6c80, (0x3e169d, 0x416858)),
    '082de47e7ce36f34ab395feee5a52f77531bdb5f73f0605c5a4f0ad17f94214e':
        (0x3eb480, (0x405c0d, 0x437f68)),
    'ff9b6f63e44fe95c4b902c8b4bb02f2cc8f891d60ccdc15ae486226c7cb1f3a7':
        (0x3e55a0, (0x3ffc7d, 0x431fd8)),
    'abf77e2b21d133a8a7f635d601a0eff60feb2327f2c0e97bd7f38267552f3512':
        (0x3e5f40, (0x40061d, 0x432978)),
}
PROLOGUE = bytes.fromhex('4885c90f84c8010000488bc45641564883ec78488958100fb6f2488968184c8bf148897820')

def verify(path):
    data = Path(path).read_bytes()
    rva, callers = PROFILES[hashlib.sha256(data).hexdigest()]
    pe = pefile.PE(data=data)
    if hashlib.sha256(data).hexdigest() == 'dff7b0211996b596611019f44e2c6de7ca8c18a6fc65106c0e32100e06520d0f':
        assert pe.get_data(0x4042c, 7) == bytes.fromhex('0fb683785f0000'), 'native binding availability offset changed'
        assert pe.get_data(0x40446, 6) == bytes.fromhex('8b837c5f0000'), 'native binding values offset changed'
    if hashlib.sha256(data).hexdigest() == '8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427':
        assert pe.get_data(0x408ac, 7) == bytes.fromhex('0fb683785f0000'), 'native binding availability offset changed'
        assert pe.get_data(0x408c6, 6) == bytes.fromhex('8b837c5f0000'), 'native binding values offset changed'
    assert pe.get_data(rva, len(PROLOGUE)) == PROLOGUE, 'native reader prologue changed'
    for caller in callers:
        call = pe.get_data(caller, 5)
        assert call[0] == 0xe8 and caller + 5 + int.from_bytes(call[1:], 'little', signed=True) == rva
        assert b'\xb2\x01' in pe.get_data(caller - 8, 8), 'caller no longer requests allow_menu=true'
    print(f'{path}: native recording reader and menu-enabled callers verified')

if __name__ == '__main__':
    for path in sys.argv[1:]:
        verify(path)

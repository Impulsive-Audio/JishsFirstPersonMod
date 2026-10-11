#!/usr/bin/env python3
"""Verify the exact released DLL and the two additional 1.19 detour entries.
Run after installing tools/requirements.txt. Read-only; no DLL modification.
"""
import argparse
import hashlib
from pathlib import Path
import pefile

PROFILES = {
    'dff7b0211996b596611019f44e2c6de7ca8c18a6fc65106c0e32100e06520d0f': '2.0.6',
    'ca9c103f85167ac6957e15ed45b999f59afc387d2722277114047f8e04fe6aaa': '2.0.5',
    '9259c3de99626d6a9d6083b337927bc948ee144dbf4e698f7733148979aeb528': '2.0.4',
    '083bd7c9636d7ff1432cb643842ba58159f26cf8ab3bbb1d2c22df322f865912': '2.0.3',
    '082de47e7ce36f34ab395feee5a52f77531bdb5f73f0605c5a4f0ad17f94214e': '2.0.2',
    'ff9b6f63e44fe95c4b902c8b4bb02f2cc8f891d60ccdc15ae486226c7cb1f3a7': '2.0.0',
    'abf77e2b21d133a8a7f635d601a0eff60feb2327f2c0e97bd7f38267552f3512': '2.0.1',
}
ENTRIES = {
    'native first-person sampler': (0x26f810, '488bc4488958085556574154415541564157488da868fdffff4881ec60030000'),
    'native console queue': (0x3ea30, '48895c24085556574154415541564157488d6c24d94881ecd0000000'),
}

def verify(path):
    data = Path(path).read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest not in PROFILES:
        raise ValueError('DLL is not one of the exact reviewed 2.0.0/2.0.1/2.0.2/2.0.3/2.0.4/2.0.5 releases')
    pe = pefile.PE(data=data)
    if pe.FILE_HEADER.Machine != 0x8664:
        raise ValueError('DLL is not x64')
    for name, (rva, hex_bytes) in ENTRIES.items():
        if PROFILES[digest] == "2.0.2":
            rva = 0x273020 if name == "native first-person sampler" else 0x3ec00
        if PROFILES[digest] == "2.0.3":
            rva = 0x210080 if name == "native first-person sampler" else 0x40e30
        if PROFILES[digest] in ("2.0.4", "2.0.5"):
            rva = 0x2196c0 if name == "native first-person sampler" else 0x411b0
        if PROFILES[digest] == "2.0.6":
            rva = 0x21ec50 if name == "native first-person sampler" else 0x41d90
        expected = bytes.fromhex(hex_bytes)
        section = pe.get_section_by_rva(rva)
        if section is None or not section.Characteristics & 0x20000000:
            raise ValueError(f'{name} does not lie in executable code')
        if pe.get_data(rva, len(expected)) != expected:
            raise ValueError(f'{name} prologue differs')
    # Both released DLLs call the same free-function sampler from the render
    # path and FrameSpring::sample. Check exact E8 relative call destinations.
    sampler = 0x21ec50 if PROFILES[digest] == "2.0.6" else 0x2196c0 if PROFILES[digest] in ("2.0.4", "2.0.5") else 0x210080 if PROFILES[digest] == "2.0.3" else 0x273020 if PROFILES[digest] == "2.0.2" else 0x26f810
    calls = (0x21d262, 0x21dd4b) if PROFILES[digest] == "2.0.6" else (0x217cd2, 0x2187bb) if PROFILES[digest] in ("2.0.4", "2.0.5") else (0x20e692, 0x20f17b) if PROFILES[digest] == "2.0.3" else (0x271632, 0x27211b) if PROFILES[digest] == "2.0.2" else (0x26de22, 0x26e90b)
    for call_rva in calls:
        call = pe.get_data(call_rva, 5)
        if len(call) != 5 or call[0] != 0xe8:
            raise ValueError(f'expected sampler call absent at {call_rva:#x}')
        target = call_rva + 5 + int.from_bytes(call[1:], 'little', signed=True)
        if target != sampler:
            raise ValueError(f'sampler call destination differs at {call_rva:#x}')
    print(f'ReSkate {PROFILES[digest]}: exact hash, both hook entries and sampler calls verified')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll')
    args = parser.parse_args()
    try:
        verify(args.dll)
    except (OSError, ValueError, pefile.PEFormatError) as error:
        parser.exit(1, f'Verification failed: {error}\n')

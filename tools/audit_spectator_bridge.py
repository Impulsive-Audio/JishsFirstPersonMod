"""Read-only validation of the ReSkate 2.0.7 spectator bridge; never loads the DLL."""
import argparse
import pathlib
import re
from inspect_hooks import Image, InspectionError
from capstone.x86 import X86_OP_MEM, X86_OP_IMM, X86_REG_RIP

DIGEST = '8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427'

def audit(path, header):
    im = Image(path)
    if im.digest != DIGEST:
        raise InspectionError('Spectator bridge requires the exact official ReSkate 2.0.7 image')
    source = header.read_text()
    addresses = {k: int(v, 16) for k, v in re.findall(r'(\w+)=(0x[0-9a-f]+)', source)}
    signatures = dict(re.findall(r'(\w+)_signature="([0-9a-f]+)"', source))
    for name, address in [('party', addresses['party_getter']), ('remote', addresses['remote_getter']),
                          ('render', addresses['render_hook']), ('transform', addresses['party_transform_return'] - 17)]:
        expected = bytes.fromhex(signatures[name])
        if im.pe.get_data(address, len(expected)) != expected:
            raise InspectionError(f'{name} code contract mismatch')
        if not im.section(address, len(expected), 0x20000000):
            raise InspectionError(f'{name} code is not executable')
    for name in ('party', 'remote'):
        pointer = addresses[name + '_pointer']
        if not im.section(pointer, 8, 0x80000000):
            raise InspectionError(f'{name} pointer is not in writable image data')
        start, end = im.function(addresses[name + '_getter'])
        code = im.code(start, end - start)
        refs = [i.address + i.size + op.mem.disp for i in code for op in i.operands
                if op.type == X86_OP_MEM and op.mem.base == X86_REG_RIP]
        if pointer not in refs:
            raise InspectionError(f'{name} getter does not reference the expected pointer')
    start, end = im.function(0x125c10)
    active = im.code(start, end-start)
    if not any(op.type == X86_OP_MEM and op.mem.disp == addresses['selected_offset'] for i in active for op in i.operands):
        raise InspectionError('Spectator query selected-ID offset mismatch')
    start, end = im.function(addresses['remote_getter'])
    remote = im.code(start, end-start)
    if not any(i.mnemonic == 'imul' and any(op.type == X86_OP_IMM and op.imm == 888 for op in i.operands) for i in remote):
        raise InspectionError('Remote storage stride mismatch')
    start, end = im.function(addresses['party_transform_return']-2)
    helper = im.code(start, end-start)
    call = next((i for i in helper if i.address == addresses['party_transform_return']-2), None)
    if not call or call.mnemonic != 'call' or call.size != 2:
        raise InspectionError('Party transform callback/caller mismatch')
    fov = next((i for i in helper if i.address + i.size == addresses['party_fov_return']), None)
    if not fov or fov.mnemonic != 'call' or fov.operands[0].type != X86_OP_IMM or fov.operands[0].imm != 0x22a390:
        raise InspectionError('Party FOV callback/caller mismatch')
    print('PASS: exact image, code signatures, private state pointers, selected ID, remote stride, render ABI and camera/FOV caller contracts')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=pathlib.Path)
    parser.add_argument('--header', type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1]/'src/spectator_contract.h')
    args = parser.parse_args()
    audit(args.dll, args.header)

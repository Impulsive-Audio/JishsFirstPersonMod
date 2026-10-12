"""Verify the shared ReSkate 2.0.7 HID handler; never loads or changes a DLL."""
import argparse, hashlib, re, struct
from pathlib import Path
import pefile

def verify(dll, source):
 data=dll.read_bytes()
 assert hashlib.sha256(data).hexdigest()=='8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427','Unreviewed DLL'
 text=(Path(__file__).resolve().parents[1]/'src/stock_hid_contract.h').read_text()
 rva=int(re.search(r'stock_hid_rva=(0x[0-9a-f]+)',text)[1],16)
 prefix=bytes(int(x,16) for x in re.search(r'stock_hid_prefix\{([^}]+)',text)[1].split(','))
 callers=[int(x,16) for x in re.search(r'stock_hid_callers\{([^}]+)',text)[1].split(',')]
 pe=pefile.PE(data=data)
 assert pe.FILE_HEADER.Machine==0x8664 and pe.get_section_by_rva(rva).Characteristics&0x20000000
 assert pe.get_data(rva,len(prefix))==prefix,'Shared HID prologue/layout changed'
 for caller in callers:
  call=pe.get_data(caller,5)
  assert call[0]==0xe8 and caller+5+struct.unpack_from('<i',call,1)[0]==rva,'Shared HID call changed'
 code=re.sub(r'\s+','',(source/'Extension/UI/Overlay/input_capture.cpp').read_text())
 for expected in ['structHidRead{HANDLEfile;void*buffer;DWORDcapacity;PlayStationPadkind;};','if(success)neutral_hid(read,bytes);','if(result)neutral_hid(read,bytes?*bytes:request?static_cast<DWORD>(request->InternalHigh):0);']:
  assert expected in code,'HID source/layout no longer reviewed'
 assert b'Native PlayStation HID report neutralized' in data
 print('ReSkate 2.0.7: exact hash, shared HID entry, POD offsets and synchronous/completion callers verified')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('dll',type=Path);p.add_argument('source',type=Path);a=p.parse_args();verify(a.dll,a.source)

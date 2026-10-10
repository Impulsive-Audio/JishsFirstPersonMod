"""Unit checks plus optional real DLL/source round trips; no files are modified."""
import argparse
import contextlib
import io
import pathlib
import sys
import unittest
from unittest.mock import patch
import inspect_hooks as hooks

class InspectorTests(unittest.TestCase):
    def test_missing_and_ambiguous_results_are_rejected(self):
        for values in ([],[0x1000,0x2000]):
            with self.assertRaises(hooks.InspectionError):
                hooks.unique(values,'test candidate')

    def test_duplicate_signature_is_ambiguous(self):
        class Section:
            Characteristics=0x20000000
            VirtualAddress=0x1000
            def get_data(self): return b'\x90'*200
        class Old:
            pe=type('PE',(),{'get_data':lambda self,rva,size:b'\x90'*size})()
        old=Old()
        md=hooks.Cs(hooks.CS_ARCH_X86,hooks.CS_MODE_64);md.detail=True
        old.code=lambda rva,size:list(md.disasm(b'\x90',rva))
        new=type('New',(),{'pe':type('PE',(),{'sections':[Section()]})()})()
        with self.assertRaises(hooks.InspectionError): hooks.signature(old,new,0x1000,1)

    def test_cli_failure_never_prints_an_entry(self):
        output,error=io.StringIO(),io.StringIO()
        with patch.object(sys,'argv',['inspect','--dll','missing.dll','--baseline','missing.dll',
             '--version','1.2.3','--source','.','--baseline-source','.']), contextlib.redirect_stdout(output),contextlib.redirect_stderr(error):
            self.assertEqual(hooks.main(),1)
        self.assertNotIn('NativeContract entry',output.getvalue())
        self.assertIn('No completed profile entry emitted',error.getvalue())

    def test_source_change_blocks_entry(self):
        image=type('Image',(),{'digest':'known','export':lambda self,name:0x1000})()
        with patch.object(sys,'argv',['inspect','--dll','a','--baseline','b','--version','1.2.3',
             '--source','new','--baseline-source','old']),patch.object(hooks,'Image',return_value=image),\
             patch.object(hooks,'profiles',return_value={'known':('1.2.2',{'status_rva':0x1000})}),\
             patch.object(hooks,'source_blocks',side_effect=[{'Settings':'old'},{'Settings':'changed'}]),\
             contextlib.redirect_stdout(io.StringIO()) as output,contextlib.redirect_stderr(io.StringIO()) as error:
            self.assertEqual(hooks.main(),1)
            self.assertNotIn('NativeContract entry',output.getvalue())
            self.assertIn('Source layout/API changed',error.getvalue())

def fixtures(root):
    sources={v:next((root/f'source-{v}').iterdir()) for v in ('1.1.7','1.1.8')}
    sources.update({v:next((root/'history'/f'source-{v}').iterdir()) for v in ('1.1.5','1.1.6')})
    dlls={'1.1.7':root/'ReSkate-1.1.7.dll','1.1.8':root/'release-1.1.8/ReSkate.dll'}
    dlls.update({v:root/'history'/f'ReSkate-{v}.dll' for v in ('1.1.5','1.1.6')})
    table=hooks.profiles(pathlib.Path(__file__).resolve().parents[1]/'src/stock_contract.h')
    for before,after in [('1.1.5','1.1.6'),('1.1.6','1.1.7'),('1.1.7','1.1.8'),('1.1.8','1.1.7')]:
        old,new=hooks.Image(dlls[before]),hooks.Image(dlls[after])
        assert hooks.source_blocks(sources[before])==hooks.source_blocks(sources[after]),'fixture source layouts differ'
        result=hooks.inspect(old,new,table[old.digest][1])
        assert result==table[new.digest][1],'discovered profile differs from verified profile'
        print(f'PASS: {before} -> {after}: all eight addresses reproduced')

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--fixtures',type=pathlib.Path)
    args=parser.parse_args()
    result=unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(InspectorTests))
    if not result.wasSuccessful(): sys.exit(1)
    if args.fixtures: fixtures(args.fixtures)

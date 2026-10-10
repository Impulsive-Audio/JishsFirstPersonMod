#include <Windows.h>
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=2) return 1;
    const auto module=LoadLibraryA(argv[1]);
    if(!module) { std::cerr<<"LoadLibrary error "<<GetLastError(); return 2; }
    const auto entry=reinterpret_cast<int(*)(void*)>(GetProcAddress(module,"luaopen_jishsfirstpersonmod"));
    const auto status=reinterpret_cast<int(*)()>(GetProcAddress(module,"JishsFirstPersonModInitializationStatus"));
    if(!entry || !status) return 3;
    for(unsigned i=0;i<20;++i) if(entry(nullptr)!=0) return 4;
    for(unsigned i=0;i<100 && status()==1;++i) Sleep(10);
    const auto result=status();
    FreeLibrary(module);
    if(result!=3) { std::cerr<<"Unexpected host guard status "<<result; return 5; }
    return 0;
}

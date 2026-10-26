#!/usr/bin/env python3
"""Compile the Windows BIOS exception handler and test real EXE/DLL address provenance."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    out = ROOT / 'build/bios-host-guard'
    out.mkdir(parents=True, exist_ok=True)
    compiler = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang.exe'
    dll = out / 'host.dll'
    (out / 'host.c').write_text('__declspec(dllexport) unsigned char hostRead[]={0x8b,0x01};\n'
                                '__declspec(dllexport) unsigned char hostDiv[]={0xf7,0xf1};\n'
                                '__attribute__((naked,dllexport)) unsigned int HostFault(void) {'
                                '__asm__("movl $0x10180000, %ecx; movl (%ecx), %eax; ret"); }\n')
    subprocess.run([str(compiler), '-shared', str(out / 'host.c'), '-o', str(dll)], check=True)
    unit = '#include "' + (ROOT / 'port/src/port_biosfault.c').as_posix() + '"\n'
    unit += r'''
#include <assert.h>
#include <stdarg.h>
uint8_t gPortBiosArea[0x10000];
void PortStateService(void) {}
void PortLog(const char* format, ...) { (void)format; }
static unsigned char readCode[]={0x8b,0x01}, divCode[]={0xf7,0xf1};
static int chained;
static LONG CALLBACK HostRecovery(EXCEPTION_POINTERS* info) {
    if (info->ExceptionRecord->ExceptionCode!=EXCEPTION_ACCESS_VIOLATION ||
        info->ExceptionRecord->ExceptionInformation[1]!=0x10180000) return EXCEPTION_CONTINUE_SEARCH;
    chained++; info->ContextRecord->Eip+=2; info->ContextRecord->Eax=0x12345678;
    return EXCEPTION_CONTINUE_EXECUTION;
}
__attribute__((noinline)) static unsigned int GameRead(unsigned int address) {
    unsigned int value; __asm__ volatile("movl (%1), %0":"=r"(value):"r"(address)); return value;
}
static LONG Fault(DWORD eip, DWORD fault, DWORD kind, DWORD exception, CONTEXT* c) {
    EXCEPTION_RECORD record={0}; EXCEPTION_POINTERS pointers={&record,c};
    memset(c,0,sizeof(*c)); c->Eip=eip; c->Ecx=fault; c->Eax=123; c->Edx=456;
    record.ExceptionCode=exception; record.NumberParameters=2;
    record.ExceptionInformation[0]=kind; record.ExceptionInformation[1]=fault;
    return BiosFaultHandler(&pointers);
}
int main(int argc,char** argv) {
    CONTEXT c; LONG result; DWORD pointer;
    HMODULE dll=LoadLibraryA(argv[1]); assert(dll);
    PortBiosFaultInstall();
    pointer=(DWORD)(uintptr_t)GetProcAddress(dll,"hostRead"); assert(pointer);
    result=Fault(pointer,0x12181218,0,EXCEPTION_ACCESS_VIOLATION,&c);
    assert(result==EXCEPTION_CONTINUE_SEARCH && c.Ecx==0x12181218 && sPending.reg==-1);
    result=Fault(pointer,0x20,1,EXCEPTION_ACCESS_VIOLATION,&c);
    assert(result==EXCEPTION_CONTINUE_SEARCH && c.Ecx==0x20 && sPending.reg==-1);
    pointer=(DWORD)(uintptr_t)GetProcAddress(dll,"hostDiv"); assert(pointer);
    result=Fault(pointer,0,0,EXCEPTION_INT_DIVIDE_BY_ZERO,&c);
    assert(result==EXCEPTION_CONTINUE_SEARCH && c.Eax==123 && c.Edx==456);
    result=Fault((DWORD)(uintptr_t)readCode,0x12181218,0,EXCEPTION_ACCESS_VIOLATION,&c);
    assert(result==EXCEPTION_CONTINUE_EXECUTION && sPending.reg==1 && (c.EFlags&0x100));
    assert(c.Ecx==BusTarget(0x12181218,0));
    { EXCEPTION_RECORD step={0}; EXCEPTION_POINTERS pointers={&step,&c};
      step.ExceptionCode=EXCEPTION_SINGLE_STEP; c.Eip+=2;
      assert(BiosFaultHandler(&pointers)==EXCEPTION_CONTINUE_EXECUTION);
      assert(c.Ecx==0x12181218 && sPending.reg==-1 && !(c.EFlags&0x100)); }
    result=Fault((DWORD)(uintptr_t)divCode,0,0,EXCEPTION_INT_DIVIDE_BY_ZERO,&c);
    assert(result==EXCEPTION_CONTINUE_EXECUTION && c.Eax==0 && c.Edx==0);
    assert(c.Eip==(DWORD)(uintptr_t)divCode+2);
    /* Exercise real OS exception delivery, not only constructed exception records. */
    { unsigned int (*hostFault)(void)=(void*)GetProcAddress(dll,"HostFault");
      void* handler=AddVectoredExceptionHandler(0,HostRecovery); assert(handler&&hostFault);
      assert(hostFault()==0x12345678 && chained==1);
      RemoveVectoredExceptionHandler(handler); }
    *(uint32_t*)gPortBiosArea=0x89abcdef;
    assert(GameRead(0x10180000)==0x89abcdef && sPending.reg==-1);
    FreeLibrary(dll); puts("DLL AV/division rejected; EXE bus recovery, single-step restoration and GBA division preserved");
    return 0;
}
'''
    (out / 'guard.c').write_text(unit)
    exe = out / 'guard.exe'
    subprocess.run([str(compiler), '-std=gnu11', '-I' + str(ROOT / 'port/include'),
                    '-idirafter', str(ROOT / 'include'), str(out / 'guard.c'), '-o', str(exe)], check=True)
    subprocess.run([str(exe), str(dll)], check=True, timeout=15)


if __name__ == '__main__':
    main()

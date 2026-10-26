/*
 * Stray pointers, as the GBA bus handles them.
 *
 * On the cartridge, a read through a NULL (or nearly NULL) pointer lands in
 * the BIOS and returns the open-bus value; a write there is ignored. The
 * original game relies on this in places (marked "@bug ... (NULL read)" and
 * wrapped in BIOS_PTR() where they are known), sometimes then using the
 * open-bus value itself as a pointer: the bus ignores the top address bits,
 * so 0xE3A02020 is an SRAM mirror. On Windows those accesses fault: the low
 * 64 KiB are never mapped, and nothing is mapped outside the GBA space.
 *
 * This vectored handler makes them behave like the GBA: the faulting
 * instruction's address register is moved so that it points at the GBA
 * location the bus would decode (gPortBiosArea, the stand-in BIOS area
 * refreshed every frame, for the BIOS and open-bus regions), the CPU
 * re-executes the instruction one step, and the register is moved back.
 * Only the ModRM/SIB bytes are decoded, to find that register; the CPU does
 * the rest, flags included. Each site is logged once ("bios: ...") so it can
 * be marked in the sources.
 *
 * Integer division by zero is the other trap: the cartridge's division
 * routines (libgcc's __divsi3 / __modsi3) return 0 for it, quotient and
 * remainder alike, where x86 raises an exception. The handler sets both to 0
 * and skips the instruction.
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
/* Linux i386: the same decoding, driven by SIGSEGV/SIGTRAP/SIGFPE. */
#define _GNU_SOURCE
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <ucontext.h>
typedef uint32_t DWORD;
#endif

#include <stdio.h>
#include <stdlib.h>
#include "port/port.h"
#include "port/port_state.h"

extern uint8_t gPortBiosArea[0x10000];

#define BIOS_AREA_SIZE 0x10000u
#define GBA_SPACE_LOW 0x02000000u
#define GBA_SPACE_HIGH 0x0E010000u
#define LOGGED_MAX 128

typedef struct Pending {
    int reg;          /* CONTEXT register index, -1 when none */
    uint32_t delta;   /* added to it for the step */
    uint32_t target;  /* its value after the move */
#ifdef _WIN32
    DWORD thread;
#endif
} Pending;

#ifdef _WIN32
static Pending sPending = { -1, 0, 0, 0 };
#else
static Pending sPending = { -1, 0, 0 };
#endif
static uint32_t sLogged[LOGGED_MAX];
static int sLoggedCount;

#ifdef _WIN32
static DWORD* Reg(CONTEXT* c, int r) {
    switch (r) {
        case 0: return &c->Eax;
        case 1: return &c->Ecx;
        case 2: return &c->Edx;
        case 3: return &c->Ebx;
        case 4: return &c->Esp;
        case 5: return &c->Ebp;
        case 6: return &c->Esi;
        default: return &c->Edi;
    }
}
#else
static DWORD* Reg(mcontext_t* c, int r) {
    static const int map[8] = { REG_EAX, REG_ECX, REG_EDX, REG_EBX, REG_ESP, REG_EBP, REG_ESI, REG_EDI };
    return (DWORD*)&c->gregs[map[r & 7]];
}
#endif

/* One-byte opcodes followed by a ModRM byte. */
static int HasModRm1(uint8_t op) {
    if (op < 0x40)
        return (op & 7) < 4; /* ALU r/m forms; 0F is handled by the caller */
    switch (op) {
        case 0x62: case 0x63: case 0x69: case 0x6B: case 0xC0: case 0xC1: case 0xC4: case 0xC5:
        case 0xC6: case 0xC7: case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xF6: case 0xF7:
        case 0xFE: case 0xFF:
            return 1;
    }
    return (op >= 0x80 && op <= 0x8F) || (op >= 0xD8 && op <= 0xDF);
}

/* Two-byte (0F xx) opcodes without a ModRM byte. */
static int NoModRm2(uint8_t op) {
    return (op >= 0x05 && op <= 0x0B) || op == 0x0E || (op >= 0x30 && op <= 0x37) || op == 0x77
           || (op >= 0x80 && op <= 0x8F) || (op >= 0xA0 && op <= 0xA2) || (op >= 0xA8 && op <= 0xAA)
           || op >= 0xC8;
}

/*
 * Picks the register to move and by how much so the instruction's effective
 * address shifts by exactly `shift`. Returns 0 when the instruction cannot be
 * handled.
 */
/* Where the GBA bus sends address `a`: its region's mirror, or the BIOS
   stand-in for the BIOS and open-bus regions (and for writes to the ROM,
   which the cartridge ignores). */
static uint32_t BusTarget(uint32_t a, int write) {
    uint32_t bios = (uint32_t)(uintptr_t)gPortBiosArea + ((a & 0xFFFFu) > 0xFFE0u ? (a & 0xFFFFu) - 0x100u : (a & 0xFFFFu));
    uint32_t o;

    switch ((a >> 24) & 0xF) {
        case 0x2:
            return 0x02000000u | (a & 0x3FFFFu);
        case 0x3:
            return 0x03000000u | (a & 0x7FFFu);
        case 0x4:
            return (a & 0xFFFFFFu) < 0x400u ? 0x04000000u | (a & 0x3FFu) : bios;
        case 0x5:
            return 0x05000000u | (a & 0x3FFu);
        case 0x6:
            o = a & 0x1FFFFu;
            return 0x06000000u | (o >= 0x18000u ? o - 0x8000u : o);
        case 0x7:
            return 0x07000000u | (a & 0x3FFu);
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xC: case 0xD:
            return write ? bios : 0x08000000u | (a & 0x1FFFFFFu);
        case 0xE: case 0xF:
            return 0x0E000000u | (a & 0xFFFFu);
        default:
            return bios;
    }
}

static int AddressRegister(const uint8_t* p, uint32_t shift, int* reg, uint32_t* delta) {
    int i;
    uint8_t op;

    for (i = 0; i < 15; i++, p++) {
        if (*p == 0x67)
            return 0; /* 16-bit addressing */
        if (*p != 0x66 && *p != 0xF2 && *p != 0xF3 && *p != 0xF0 && *p != 0x2E && *p != 0x3E && *p != 0x26
            && *p != 0x36 && *p != 0x64 && *p != 0x65)
            break;
    }
    op = *p++;
    /* String instructions: source ESI, destination EDI. */
    if ((op >= 0xA4 && op <= 0xA7) || (op >= 0xAA && op <= 0xAF)) {
        *reg = (op == 0xAA || op == 0xAB || op == 0xAE || op == 0xAF) ? 7 : 6;
        *delta = shift;
        return 1;
    }
    if (op == 0x0F) {
        op = *p++;
        if (op == 0x38 || op == 0x3A)
            p++;
        else if (NoModRm2(op))
            return 0;
    } else if (!HasModRm1(op)) {
        return 0;
    }
    {
        uint8_t modrm = *p++;
        int mod = modrm >> 6, rm = modrm & 7;
        int base = -1, index = -1, scale = 1;

        if (mod == 3)
            return 0;
        if (rm == 4) {
            uint8_t sib = *p++;
            scale = 1 << (sib >> 6);
            index = (sib >> 3) & 7;
            base = sib & 7;
            if (index == 4)
                index = -1;
            if (base == 5 && mod == 0)
                base = -1;
        } else if (!(rm == 5 && mod == 0)) {
            base = rm;
        }
        if (base >= 0 && base != index) {
            *reg = base;
            *delta = shift;
            return 1;
        }
        if (index >= 0 && base < 0 && shift % (uint32_t)scale == 0) {
            *reg = index;
            *delta = shift / (uint32_t)scale;
            return 1;
        }
    }
    return 0;
}

/* Length of a DIV/IDIV (F6/F7 /6, /7) instruction, 0 if it is not one. */
static int DivLength(const uint8_t* p, int* bytes) {
    const uint8_t* start = p;
    int size16 = 0;
    uint8_t modrm;
    int mod, rm;

    while (*p == 0x66 || *p == 0x2E || *p == 0x3E || *p == 0x26 || *p == 0x36 || *p == 0x64 || *p == 0x65) {
        size16 |= *p == 0x66;
        p++;
    }
    if (*p != 0xF6 && *p != 0xF7)
        return 0;
    *bytes = *p == 0xF6 ? 1 : (size16 ? 2 : 4);
    modrm = p[1];
    if (((modrm >> 3) & 7) < 6)
        return 0;
    mod = modrm >> 6;
    rm = modrm & 7;
    p += 2;
    if (mod != 3) {
        if (rm == 4) {
            uint8_t sib = *p++;
            if (mod == 0 && (sib & 7) == 5)
                p += 4;
        } else if (mod == 0 && rm == 5) {
            p += 4;
        }
        p += mod == 1 ? 1 : (mod == 2 ? 4 : 0);
    }
    return (int)(p - start);
}

static int InGameCode(DWORD eip);
static DWORD sFaultEsp; /* stack pointer of the fault being handled */
static int sStackScan = -1;

/* KHCOM_BIOS_STACK=1 (debug kit): the game-code addresses found on the stack
   right after the site, newest first. The release build has no frame pointers,
   so these are candidate callers, not an exact backtrace. */
static void LogStack(uint32_t eip) {
    char line[160];
    int i, n = 0, len;
    const DWORD* sp = (const DWORD*)(uintptr_t)sFaultEsp;

    if (sStackScan < 0) {
        const char* env = getenv("KHCOM_BIOS_STACK");
        sStackScan = env && *env == '1';
    }
    if (!sStackScan || !sp)
        return;
    len = snprintf(line, sizeof(line), "bios-stack: %08lx <-", (unsigned long)eip);
    for (i = 0; i < 96 && n < 8; i++) {
        if (InGameCode(sp[i])) {
            len += snprintf(line + len, sizeof(line) - (size_t)len, " %08lx", (unsigned long)sp[i]);
            n++;
        }
    }
    PortLog("%s", line);
}

/* kind: 0 read, 1 write, 2 division by zero. */
static void LogSite(int kind, uint32_t addr, uint32_t eip) {
    int i;

    for (i = 0; i < sLoggedCount; i++)
        if (sLogged[i] == eip)
            return;
    if (sLoggedCount < LOGGED_MAX)
        sLogged[sLoggedCount++] = eip;
    if (kind == 2)
        PortLog("bios: division by zero at eip %08lx", (unsigned long)eip);
    else
        PortLog("bios: %s %08lx at eip %08lx", kind ? "write" : "read", (unsigned long)addr, (unsigned long)eip);
    LogStack(eip);
}

/* Division by zero: quotient and remainder 0, as libgcc returns on the GBA.
   Returns 0 when the instruction is not a DIV/IDIV. */
static int SkipDivision(DWORD* eip, DWORD* eax, DWORD* edx) {
    int bytes, len = DivLength((const uint8_t*)(uintptr_t)*eip, &bytes);

    if (len == 0)
        return 0;
    LogSite(2, 0, *eip);
    if (bytes == 1) {
        *eax &= ~0xFFFFu; /* AL quotient, AH remainder */
    } else if (bytes == 2) {
        *eax &= ~0xFFFFu;
        *edx &= ~0xFFFFu;
    } else {
        *eax = 0;
        *edx = 0;
    }
    *eip += (DWORD)len;
    return 1;
}

/* A bus access outside the GBA space: picks the register to move for one
   instruction. Returns 0 when it must stay a real fault. */
/* The game's code is linked first, before the platform layer (port_state.c
   hashes the same span for save states). */
static int InGameCode(DWORD eip) {
#ifdef _WIN32
    const uint8_t* start = (const uint8_t*)GetModuleHandleA(NULL) + 0x1000;
#else
    extern const uint8_t __executable_start[];
    const uint8_t* start = __executable_start;
#endif
    return eip >= (DWORD)(uintptr_t)start && eip < (DWORD)(uintptr_t)PortStateService;
}

static int RedirectAccess(uint32_t fault, int write, DWORD eip, int* reg, uint32_t* delta) {
#ifdef PORT_GBA_RELOC
    /* The relocatable build: the bus range itself is not mapped; the game reaching it through a bus address goes to
       the region's array (logged, as the sites to translate). */
    if (fault >= GBA_SPACE_LOW && fault < GBA_SPACE_HIGH) {
        uint32_t target = BusTarget(fault, write);
        if (target >= GBA_SPACE_LOW && target < GBA_SPACE_HIGH) {
            void* p = GbaSpacePtr(target);
            target = p ? (uint32_t)(uintptr_t)p : (uint32_t)(uintptr_t)gPortBiosArea;
        }
        if (!AddressRegister((const uint8_t*)(uintptr_t)eip, target - fault, reg, delta))
            return 0;
        LogSite(write, fault, eip);
        return 1;
    }
#else
    if (fault >= GBA_SPACE_LOW && fault < GBA_SPACE_HIGH)
        return 0; /* only addresses outside the GBA space */
#endif
    /* The system's own address range: a fault there is the port's own bug,
       unless the game's code made it. On the GBA everything below EWRAM past
       the BIOS is open bus, and the game does read it: a stale panelsVisible
       flag animating garbage through a NULL table (issue #26) read
       0x0003FFFC, harmless on the cartridge. */
    if (fault >= BIOS_AREA_SIZE && fault < GBA_SPACE_LOW && !InGameCode(eip))
        return 0;
    {
        uint32_t target = BusTarget(fault, write);
#ifdef PORT_GBA_RELOC
        if (target >= GBA_SPACE_LOW && target < GBA_SPACE_HIGH) {
            void* p = GbaSpacePtr(target);
            target = p ? (uint32_t)(uintptr_t)p : (uint32_t)(uintptr_t)gPortBiosArea;
        }
#endif
        if (!AddressRegister((const uint8_t*)(uintptr_t)eip, target - fault, reg, delta))
            return 0;
    }
    LogSite(write, fault, eip);
    return 1;
}

#ifdef _WIN32
static DWORD sExeBase, sExeSize;
static LONG CALLBACK BiosFaultHandler(EXCEPTION_POINTERS* info) {
    EXCEPTION_RECORD* r = info->ExceptionRecord;
    CONTEXT* c = info->ContextRecord;

    sFaultEsp = c->Esp;

    if (r->ExceptionCode == EXCEPTION_SINGLE_STEP) {
        if (sPending.reg < 0 || sPending.thread != GetCurrentThreadId())
            return EXCEPTION_CONTINUE_SEARCH;
        {
            DWORD* v = Reg(c, sPending.reg);
            /* Move it back unless the instruction loaded something new into it
               (it then no longer points near the redirected location). */
            if (*v - sPending.target + 0x10000u < 0x20000u)
                *v -= sPending.delta;
        }
        sPending.reg = -1;
        c->EFlags &= ~0x100u;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    /* Only this executable emulates the GBA bus. A system/codec DLL may use
       access violations internally; rewriting its registers corrupts its own
       exception recovery. Keep platform code in the EXE eligible, as before. */
    if (c->Eip - sExeBase >= sExeSize)
        return EXCEPTION_CONTINUE_SEARCH;
    if (r->ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO)
        return SkipDivision(&c->Eip, &c->Eax, &c->Edx) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
    if (r->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || r->NumberParameters < 2 || r->ExceptionInformation[0] == 8
        || sPending.reg >= 0)
        return EXCEPTION_CONTINUE_SEARCH;
    {
        int reg;
        uint32_t delta;

        if (!RedirectAccess((uint32_t)r->ExceptionInformation[1], r->ExceptionInformation[0] == 1, c->Eip, &reg, &delta))
            return EXCEPTION_CONTINUE_SEARCH;
        *Reg(c, reg) += delta;
        sPending.reg = reg;
        sPending.delta = delta;
        sPending.target = *Reg(c, reg);
        sPending.thread = GetCurrentThreadId();
        c->EFlags |= 0x100u; /* trap after this one instruction */
        return EXCEPTION_CONTINUE_EXECUTION;
    }
}

void PortBiosFaultInstall(void) {
    const uint8_t* image = (const uint8_t*)GetModuleHandleA(NULL);
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)image;
    const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)(image + dos->e_lfanew);
    sExeBase = (DWORD)(uintptr_t)image;
    sExeSize = nt->OptionalHeader.SizeOfImage;
    AddVectoredExceptionHandler(1, BiosFaultHandler);
}
#else
/* The game runs on one thread; signals are only redirected for it. */
static struct sigaction sPrevSegv, sPrevTrap, sPrevFpe;

static void Chain(int sig, siginfo_t* si, void* uc) {
    struct sigaction* prev = sig == SIGSEGV ? &sPrevSegv : sig == SIGTRAP ? &sPrevTrap : &sPrevFpe;

    if (prev->sa_flags & SA_SIGINFO) {
        prev->sa_sigaction(sig, si, uc);
    } else if (prev->sa_handler != SIG_IGN && prev->sa_handler != SIG_DFL) {
        prev->sa_handler(sig);
    } else {
        signal(sig, SIG_DFL);
        raise(sig);
    }
}

static void BiosSignal(int sig, siginfo_t* si, void* ucv) {
    mcontext_t* c = &((ucontext_t*)ucv)->uc_mcontext;
    DWORD* eip = (DWORD*)&c->gregs[REG_EIP];

    sFaultEsp = (DWORD)c->gregs[REG_ESP];

    if (sig == SIGFPE) {
        if (si->si_code == FPE_INTDIV && SkipDivision(eip, Reg(c, 0), Reg(c, 2)))
            return;
    } else if (sig == SIGTRAP) {
        if (sPending.reg >= 0) {
            DWORD* v = Reg(c, sPending.reg);
            /* Move it back unless the instruction loaded something new into it. */
            if (*v - sPending.target + 0x10000u < 0x20000u)
                *v -= sPending.delta;
            sPending.reg = -1;
            c->gregs[REG_EFL] &= ~0x100;
            return;
        }
    } else if (sPending.reg < 0) {
        int reg;
        uint32_t delta;

        if (RedirectAccess((uint32_t)(uintptr_t)si->si_addr, (c->gregs[REG_ERR] & 2) != 0, *eip, &reg, &delta)) {
            *Reg(c, reg) += delta;
            sPending.reg = reg;
            sPending.delta = delta;
            sPending.target = *Reg(c, reg);
            c->gregs[REG_EFL] |= 0x100; /* trap after this one instruction */
            return;
        }
    }
    Chain(sig, si, ucv);
}

void PortBiosFaultInstall(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = BiosSignal;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &sPrevSegv);
    sigaction(SIGTRAP, &sa, &sPrevTrap);
    sigaction(SIGFPE, &sa, &sPrevFpe);
}
#endif

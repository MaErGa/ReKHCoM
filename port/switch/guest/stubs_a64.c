/* One stub per host function: loads the table index and jumps to the trampoline (crt0_a64.S). The index is the
 * entry's position in bridge_list.h, the same order as the host's table (HF_*). */
#include "bridge.h"

#define STR2(x) #x
#define STR(x) STR2(x)
#define X(name) \
    __asm__(".text\n.p2align 2\n.globl _h_" #name "\n_h_" #name ":\n\tmov w16, #" STR(__COUNTER__) "\n\tb ___host_call\n");
#include "bridge_list.h"
#undef X

uint64_t gHostTable[HF_COUNT];

/* What the host reads to start the image (port/switch/host): its own segment, so it is found by name. */
extern char rk_heap[], rk_stack[];
extern char gPortBiosArea[];
extern int switch_boot(uint32_t stackTop);

__attribute__((section("__RKHDR,__header"), used)) const uint32_t gSwitchGuestHeader[9] = {
    PS5G_MAGIC,
    0, /* imageEnd: the host knows it from the segments */
    (uint32_t)(uintptr_t)switch_boot,
    (uint32_t)(uintptr_t)gHostTable,
    (uint32_t)(uintptr_t)rk_heap,
    (uint32_t)(uintptr_t)(rk_heap + 0x06000000),
    (uint32_t)(uintptr_t)(rk_stack + 0x00800000),
    0, /* regionEnd: idem */
    (uint32_t)(uintptr_t)gPortBiosArea,
};

/* ---- threads: the PPU's other cores (port_ppu.c with PORT_PPU_THREADS, SWITCH_MOTOR.md) -------------------------
 * The host starts a thread of its own (bridge entry thread) that enters the image through switch_thread (crt0_a64.S)
 * on one of the image's stacks rk_tstacks: RK_THREADS of 512 KiB, aligned so a thread's slot (1..RK_THREADS; 0 the
 * game's thread) is the stack sp is on. The slot picks the host stack its host calls switch to (gHostSp) and its
 * block of thread-local variables (gTlsBase, read by __tlv_bootstrap). The game's thread keeps the variables where
 * the linker put them; another thread's block is a fresh copy of their first values. */
#include <SDL3/SDL.h>
#include "bridge_calls.h"
#define RK_THREADS 3
#define RK_TSTACK 0x80000u
extern char rk_tstacks[];
extern int switch_thread(uint32_t fn, uint32_t arg, uint32_t stackTop);
extern void tlv_bootstrap(void) __asm__("__tlv_bootstrap");
extern char tvars_start[] __asm__("section$start$__DATA$__thread_vars");
extern char tvars_end[] __asm__("section$end$__DATA$__thread_vars");
extern char tdata_start[] __asm__("section$start$__DATA$__thread_data");
extern char tdata_end[] __asm__("section$end$__DATA$__thread_data");
extern char tbss_end[] __asm__("section$end$__DATA$__thread_bss");
uint32_t gTlsBase[1 + RK_THREADS];
static uint8_t sTlsFirst[4096]; /* the thread_data section as linked: what a new thread starts with */
static int sThreads;

typedef struct { uint32_t thunk, key, offset; } TlvDescriptor;

/* Called by switch_boot before guest_main: the descriptors point at our thunk (lld leaves them for dyld). */
void rk_threads_boot(void) {
    TlvDescriptor* d;
    uint32_t data = (uint32_t)(tdata_end - tdata_start);
    for (d = (TlvDescriptor*)tvars_start; (char*)d < tvars_end; d++)
        d->thunk = (uint32_t)(uintptr_t)tlv_bootstrap;
    gTlsBase[0] = (uint32_t)(uintptr_t)tdata_start;
    if (data <= sizeof(sTlsFirst))
        memcpy(sTlsFirst, tdata_start, data);
}

int SDL_GetNumLogicalCPUCores(void) {
    int n = h_thread(0, 0, 0, 0, 0);
    return n > 1 + RK_THREADS ? 1 + RK_THREADS : n < 1 ? 1 : n;
}

SDL_Thread* SDL_CreateThread(SDL_ThreadFunction fn, const char* name, void* data) {
    uint32_t size = (uint32_t)(tbss_end - tdata_start), first = (uint32_t)(tdata_end - tdata_start);
    int slot = sThreads + 1;
    uint8_t* block;
    (void)name;
    if (slot > RK_THREADS || first > sizeof(sTlsFirst) || !(block = (uint8_t*)malloc(size + 16)))
        return NULL;
    block = (uint8_t*)(((uintptr_t)block + 15) & ~(uintptr_t)15);
    memcpy(block, sTlsFirst, first);
    memset(block + first, 0, size - first);
    gTlsBase[slot] = (uint32_t)(uintptr_t)block;
    if (!h_thread(1, (uint32_t)(uintptr_t)switch_thread, (uint32_t)(uintptr_t)fn, (uint32_t)(uintptr_t)data,
                  (uint32_t)(uintptr_t)(rk_tstacks + slot * RK_TSTACK)))
        return NULL;
    sThreads = slot;
    return (SDL_Thread*)(uintptr_t)slot;
}

void SDL_DetachThread(SDL_Thread* thread) {
    (void)thread;
}

SDL_Semaphore* SDL_CreateSemaphore(Uint32 initial) {
    int h = h_thread(2, initial, 0, 0, 0);
    return h > 0 ? (SDL_Semaphore*)(uintptr_t)h : NULL;
}

void SDL_WaitSemaphore(SDL_Semaphore* sem) {
    h_thread(3, (uint32_t)(uintptr_t)sem, 0, 0, 0);
}

void SDL_SignalSemaphore(SDL_Semaphore* sem) {
    h_thread(4, (uint32_t)(uintptr_t)sem, 0, 0, 0);
}

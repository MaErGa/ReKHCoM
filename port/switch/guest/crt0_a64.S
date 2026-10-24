/* Entry, host-call trampoline, setjmp and the memory primitives of the Switch game image: AArch64 code with 32-bit
 * pointers (arm64_32, Mach-O objects). The PS5 image's crt0.S does the same in x86-64. The host (port/switch/host)
 * is ordinary 64-bit code: pointers crossing to it are zero-extended here. */
    .text
    .p2align 2

/* int switch_boot(uint32_t stackTop): called by the host on its own stack. Saves the host's callee-saved registers
 * and stack, runs guest_main on the guest stack, returns guest_main's result. */
    .globl _switch_boot
_switch_boot:
    stp x29, x30, [sp, #-16]!
    stp x27, x28, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x19, x20, [sp, #-16]!
    stp d14, d15, [sp, #-16]!
    stp d12, d13, [sp, #-16]!
    stp d10, d11, [sp, #-16]!
    stp d8, d9, [sp, #-16]!
    adrp x9, _gHostSp@PAGE
    add x9, x9, _gHostSp@PAGEOFF
    mov x10, sp
    str x10, [x9]
    mov w0, w0
    mov sp, x0
    mov x29, #0
    bl _rk_threads_boot
    bl _guest_main
    adrp x9, _gHostSp@PAGE
    add x9, x9, _gHostSp@PAGEOFF
    ldr x10, [x9]
    mov sp, x10
    ldp d8, d9, [sp], #16
    ldp d10, d11, [sp], #16
    ldp d12, d13, [sp], #16
    ldp d14, d15, [sp], #16
    ldp x19, x20, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x27, x28, [sp], #16
    ldp x29, x30, [sp], #16
    ret

/* Entered through a stub with the table index in w16. Runs the host function on the host's stack and comes back to
 * the guest stack. Arguments are in registers only (x0-x7, d0-d7); the 32-bit ones are zero-extended, which is what
 * the pointers need and what the host's 32-bit parameters ignore. */
    .globl ___host_call
___host_call:
    adrp x9, _gHostTable@PAGE
    add x9, x9, _gHostTable@PAGEOFF
    ldr x17, [x9, w16, uxtw #3]
    mov x11, sp
    adrp x9, _rk_tstacks@PAGE   /* the slot of this thread: 1.. on a stack of rk_tstacks, else 0 */
    add x9, x9, _rk_tstacks@PAGEOFF
    sub x10, x11, x9
    lsr x10, x10, #19
    cmp x10, #3
    csinc x10, xzr, x10, hs
    adrp x9, _gHostSp@PAGE      /* each thread's host stack */
    add x9, x9, _gHostSp@PAGEOFF
    ldr x10, [x9, x10, lsl #3]
    mov sp, x10
    stp x11, x30, [sp, #-16]!
    mov w0, w0
    mov w1, w1
    mov w2, w2
    mov w3, w3
    mov w4, w4
    mov w5, w5
    mov w6, w6
    mov w7, w7
    blr x17
    ldp x11, x30, [sp], #16
    mov sp, x11
    ret

/* int switch_thread(uint32_t fn, uint32_t arg, uint32_t stackTop): a host thread (bridge entry thread, op 1) entering
 * the image, like switch_boot: fn(arg) on the guest stack stackTop, one of rk_tstacks (stubs_a64.c), whose slot
 * keeps this thread's host stack for its host calls. */
    .globl _switch_thread
_switch_thread:
    stp x29, x30, [sp, #-16]!
    stp x27, x28, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x19, x20, [sp, #-16]!
    stp d14, d15, [sp, #-16]!
    stp d12, d13, [sp, #-16]!
    stp d10, d11, [sp, #-16]!
    stp d8, d9, [sp, #-16]!
    mov w0, w0
    mov w1, w1
    mov w2, w2
    sub x11, x2, #16
    adrp x9, _rk_tstacks@PAGE   /* the slot of this thread: 1.. on a stack of rk_tstacks, else 0 */
    add x9, x9, _rk_tstacks@PAGEOFF
    sub x10, x11, x9
    lsr x10, x10, #19
    cmp x10, #3
    csinc x10, xzr, x10, hs
    mov x19, x10
    adrp x9, _gHostSp@PAGE
    add x9, x9, _gHostSp@PAGEOFF
    mov x11, sp
    str x11, [x9, x19, lsl #3]
    mov sp, x2
    mov x29, #0
    mov x9, x0
    mov w0, w1
    blr x9
    adrp x9, _gHostSp@PAGE
    add x9, x9, _gHostSp@PAGEOFF
    ldr x10, [x9, x19, lsl #3]
    mov sp, x10
    ldp d8, d9, [sp], #16
    ldp d10, d11, [sp], #16
    ldp d12, d13, [sp], #16
    ldp d14, d15, [sp], #16
    ldp x19, x20, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x27, x28, [sp], #16
    ldp x29, x30, [sp], #16
    ret

/* Thread-local variables (_Thread_local, port_ppu.c's PPU_TLS): clang calls each variable's descriptor thunk with the
 * descriptor in x0 and wants its address back, every other register but x16/x17 kept. The descriptors' thunk is this
 * function (rk_threads_boot points them here): the slot of the calling thread's stack picks its block (gTlsBase),
 * plus the descriptor's offset. */
    .globl __tlv_bootstrap
__tlv_bootstrap:
    mov x16, sp
    adrp x17, _rk_tstacks@PAGE
    add x17, x17, _rk_tstacks@PAGEOFF
    sub x16, x16, x17
    lsr x16, x16, #19
    cmp x16, #3
    csinc x16, xzr, x16, hs
    adrp x17, _gTlsBase@PAGE
    add x17, x17, _gTlsBase@PAGEOFF
    ldr w16, [x17, x16, lsl #2]
    ldr w17, [x0, #8]
    add w0, w16, w17
    ret

/* jmp_buf: x19-x30, sp, d8-d15 (22 words, include/setjmp.h) */
    .globl _setjmp
_setjmp:
    mov w0, w0
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    stp x29, x30, [x0, #80]
    mov x9, sp
    str x9, [x0, #96]
    stp d8, d9, [x0, #104]
    stp d10, d11, [x0, #120]
    stp d12, d13, [x0, #136]
    stp d14, d15, [x0, #152]
    mov w0, #0
    ret

    .globl _longjmp
_longjmp:
    mov w0, w0
    ldp x19, x20, [x0, #0]
    ldp x21, x22, [x0, #16]
    ldp x23, x24, [x0, #32]
    ldp x25, x26, [x0, #48]
    ldp x27, x28, [x0, #64]
    ldp x29, x30, [x0, #80]
    ldr x9, [x0, #96]
    mov sp, x9
    ldp d8, d9, [x0, #104]
    ldp d10, d11, [x0, #120]
    ldp d12, d13, [x0, #136]
    ldp d14, d15, [x0, #152]
    cmp w1, #0
    csinc w0, w1, wzr, ne
    ret

    .globl _memcpy
_memcpy:
    mov w0, w0
    mov w1, w1
    mov w2, w2
    mov x3, x0
    orr x5, x0, x1          /* both 8-byte aligned: 16 bytes at a time (forward, so memmove may use it too) */
    tst x5, #7
    b.ne 3f
4:  cmp x2, #16
    b.lo 3f
    ldp x4, x6, [x1], #16
    stp x4, x6, [x3], #16
    sub x2, x2, #16
    b 4b
3:  cbz x2, 2f
1:  ldrb w4, [x1], #1
    strb w4, [x3], #1
    subs x2, x2, #1
    b.ne 1b
2:  ret

    .globl _memset
_memset:
    mov w0, w0
    mov w2, w2
    mov x3, x0
    and w1, w1, #255
    orr w1, w1, w1, lsl #8
    orr w1, w1, w1, lsl #16
    orr x1, x1, x1, lsl #32
    tst x3, #7              /* 8-byte aligned: 16 bytes at a time */
    b.ne 3f
4:  cmp x2, #16
    b.lo 3f
    stp x1, x1, [x3], #16
    sub x2, x2, #16
    b 4b
3:  cbz x2, 2f
1:  strb w1, [x3], #1
    subs x2, x2, #1
    b.ne 1b
2:  ret

    .globl _bzero      /* what clang calls for memset(p, 0, n) on this target */
_bzero:
    mov w2, w1
    mov w1, #0
    b _memset

    .globl _memmove
_memmove:
    mov w0, w0
    mov w1, w1
    mov w2, w2
    cmp x0, x1
    b.ls _memcpy
    add x3, x1, x2
    cmp x0, x3
    b.hs _memcpy
    add x4, x0, x2
    cbz x2, 2f
1:  ldrb w5, [x3, #-1]!
    strb w5, [x4, #-1]!
    subs x2, x2, #1
    b.ne 1b
2:  ret

    .data
    .p2align 3
    .globl _gHostSp
_gHostSp:
    .quad 0, 0, 0, 0  /* the game's thread, then the PPU's (slots 1-3) */

/* The guest's heap and stack: zero-filled segments the host maps with the image. */
    .globl _rk_heap
    .globl _rk_stack
    .zerofill __RKHEAP, __heap, _rk_heap, 0x06000000, 12
    .zerofill __RKSTACK, __stack, _rk_stack, 0x00800000, 12
/* the stacks of the other threads (stubs_a64.c): three of 512 KiB, aligned so the slot is sp's offset >> 19 */
    .globl _rk_tstacks
    .zerofill __RKTSTACK, __stacks, _rk_tstacks, 0x00180000, 19

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68k_native_out() with m68k_bptr_out(), m68k_bptr_in() and
    m68k_to_host_or_shadow(), taken verbatim from rom/m68kemu by
    run_native_returns.py. A native pointer returned to a 68k program must
    become its address when it points into the program's memory, and a handle
    that leads back to it otherwise, at any native address: also below 4 GB,
    where the dispatcher's test of the upper 32 bits cannot tell a pointer
    from a number. No pointer is dereferenced.
*/
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t ULONG;
typedef uint16_t UWORD;
typedef uint8_t UBYTE;
typedef void *APTR;
typedef uintptr_t IPTR;
typedef uintptr_t BPTR;
#define BNULL ((BPTR)0)
#define MKBADDR(a) ((BPTR)(a))
#define MEMF_CLEAR 1

#include "constants.inc"

struct M68KShadowEntry
{
    ULONG m68k_addr;
    void *native_ptr;
};

struct M68KEmuContext
{
    UBYTE *mem;
    ULONG mem_size;
    BPTR *handles;
    ULONG num_handles;
    UWORD num_shadows;
    struct M68KShadowEntry shadow_map[4];
};

static APTR AllocVec(size_t size, ULONG flags)
{
    (void)flags;
    return calloc(1, size);
}

static void bug(const char *fmt, ...)
{
    (void)fmt;
}

#include "handles.inc"

static unsigned checks, failures;

static void check(int ok, const char *what)
{
    checks++;
    if (!ok)
    {
        failures++;
        printf("FAIL: %s\n", what);
    }
}

static int is_handle(ULONG v)
{
    return v >= M68KEMU_HANDLE_BASE && v < M68KEMU_HANDLE_BASE + M68KEMU_MAX_HANDLES * 4 &&
           (v - M68KEMU_HANDLE_BASE) % 4 == 0;
}

int main(void)
{
    static UBYTE arena[0x10000];
    struct M68KEmuContext ctx;
    APTR low = (APTR)(IPTR)0x003FEC70UL, mid = (APTR)(IPTR)0x7FFF0000UL, end = arena + sizeof(arena);
    ULONG h_low, h_mid, h_end;

    memset(&ctx, 0, sizeof(ctx));
    ctx.mem = arena;
    ctx.mem_size = sizeof(arena);

    check(m68k_native_out(&ctx, NULL) == 0, "NULL is 0");
    check(m68k_native_out(&ctx, arena + 0x1234) == 0x1234, "a pointer into the program's memory is its address");
    check(m68k_native_out(&ctx, arena + sizeof(arena) - 1) == sizeof(arena) - 1,
          "the last byte of the program's memory is its address");
    check(ctx.num_handles == 0, "no handle is taken for the program's own memory");

    /* the value an AROS AArch64 guest gave for AllocDosObject(DOS_RDARGS):
       its upper 32 bits are 0, so the dispatcher's test hands it out as it is */
    h_low = m68k_native_out(&ctx, low);
    check(is_handle(h_low), "a native pointer below 32 MB, below 4 GB, becomes a handle");
    check(m68k_to_host_or_shadow(&ctx, h_low) == low, "the handle of a pointer below 4 GB leads back to it");
    check(m68k_native_out(&ctx, low) == h_low, "the same native pointer gives the same handle");

    h_mid = m68k_native_out(&ctx, mid);
    check(is_handle(h_mid) && h_mid != h_low, "a native pointer just below 2 GB becomes another handle");
    check(m68k_to_host_or_shadow(&ctx, h_mid) == mid, "and leads back to it");

    h_end = m68k_native_out(&ctx, end);
    check(is_handle(h_end), "the first byte after the program's memory is native: a handle");
    check(m68k_to_host_or_shadow(&ctx, h_end) == end, "and leads back to it");

    if (sizeof(IPTR) > 4)
    {
        APTR high = (APTR)(IPTR)(((IPTR)0x1234 << 32) | 0x56789000UL);
        ULONG h_high = m68k_native_out(&ctx, high);
        check(is_handle(h_high), "a native pointer above 4 GB becomes a handle");
        check(m68k_to_host_or_shadow(&ctx, h_high) == high, "and leads back to it");
        check(m68k_bptr_in(&ctx, h_high) == (BPTR)high, "m68k_bptr_in() reads the same handle");
    }

    check(ctx.num_handles == (sizeof(IPTR) > 4 ? 4u : 3u), "one handle per distinct native pointer");

    printf("native returns: %u checks, %u failed\n", checks, failures);
    free(ctx.handles);
    return failures != 0;
}

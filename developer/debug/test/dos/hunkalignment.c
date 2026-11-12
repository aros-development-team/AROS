/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/asmcall.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/doshunks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

static UBYTE image[160];
static ULONG length, position, read_limit;
static ULONG allocations, live, fail_at, free_errors;
static unsigned int checks, failures;
static struct { APTR address; ULONG size; } blocks[16];

static void check(int ok, const char *what)
{
    checks++;
    if (!ok)
    {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

static void put16(ULONG n)
{
    image[length++] = n >> 8;
    image[length++] = n;
}

static void put32(ULONG n)
{
    put16(n >> 16);
    put16(n);
}

static ULONG get32(const UBYTE *p)
{
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) |
           ((ULONG)p[2] << 8) | p[3];
}

static BPTR next_segment(BPTR seg)
{
    BPTR next;
    CopyMem(BADDR(seg), &next, sizeof(next));
    return next;
}

static void make_image(ULONG kind)
{
    length = 0;
    put32(HUNK_HEADER); put32(0); put32(3); put32(0); put32(2);
    put32(3); put32(2); put32(2);
    put32(HUNK_CODE); put32(3);
    put32(0xa55a0000); put32(0x00043cc3); put32(0x4e754e71);
    put32(kind);
    if (kind == HUNK_RELOC32)
    {
        put32(1); put32(1); put32(2); put32(0);
    }
    else
    {
        put16(1); put16(1); put16(2); put16(0);
    }
    put32(HUNK_END);
    put32(HUNK_DATA); put32(2); put32(0x11223344); put32(0x55667788);
    put32(HUNK_END);
    put32(HUNK_BSS); put32(2); put32(HUNK_END);
}

static AROS_UFH4(LONG, memory_read,
    AROS_UFHA(BPTR, handle, D1), AROS_UFHA(APTR, buffer, D2),
    AROS_UFHA(LONG, count, D3), AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT
    ULONG n;
    (void)handle; (void)DOSBase;
    if (count < 0)
        return -1;
    n = (ULONG)count < length - position ? (ULONG)count : length - position;
    /* The dispatcher reads the header in one call; fragment later reads. */
    if (position && read_limit && n > read_limit)
        n = read_limit;
    CopyMem(image + position, buffer, n);
    position += n;
    return n;
    AROS_USERFUNC_EXIT
}

static AROS_UFH3(APTR, memory_alloc,
    AROS_UFHA(ULONG, size, D0), AROS_UFHA(ULONG, flags, D1),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    APTR p;
    unsigned int i;
    (void)SysBase;
    allocations++;
    if (fail_at && allocations == fail_at)
        return NULL;
    p = AllocMem(size, flags);
    if (!p)
        return NULL;
    for (i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
        if (!blocks[i].address)
        {
            blocks[i].address = p;
            blocks[i].size = size;
            live++;
            return p;
        }
    FreeMem(p, size);
    return NULL;
    AROS_USERFUNC_EXIT
}

static AROS_UFH3(void, memory_free,
    AROS_UFHA(APTR, address, A1), AROS_UFHA(ULONG, size, D0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    unsigned int i;
    (void)SysBase;
    if (!address)
        return;
    for (i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
        if (blocks[i].address == address)
        {
            if (blocks[i].size != size)
                free_errors++;
            FreeMem(address, blocks[i].size);
            blocks[i].address = NULL;
            live--;
            return;
        }
    free_errors++;
    AROS_USERFUNC_EXIT
}

static BPTR load_image(void)
{
    APTR functions[3] = { (APTR)memory_read, (APTR)memory_alloc, (APTR)memory_free };
    position = allocations = free_errors = 0;
    SetIoErr(0);
    return InternalLoadSeg((BPTR)1, BNULL, (LONG_FUNC *)functions, NULL);
}

static void unload_image(BPTR seg)
{
    check(InternalUnLoadSeg(seg, (VOID_FUNC)memory_free) != DOSFALSE,
          "InternalUnLoadSeg accepts the loaded seglist");
    check(live == 0 && free_errors == 0, "all allocation sizes and frees match");
}

static void test_relocation(ULONG kind, ULONG fragment)
{
    BPTR seg, second, third;
    UBYTE *code, *data, *bss;
    ULONG want, prefix;
    make_image(kind);
    read_limit = fragment;
    fail_at = 0;
    seg = load_image();
    check(seg != BNULL, "load word-offset relocation image");
    if (!seg)
        return;
    second = next_segment(seg);
    third = second ? next_segment(second) : BNULL;
    check(second && third && !next_segment(third), "CODE/DATA/BSS seglist links");
    if (!second || !third)
    {
        unload_image(seg);
        return;
    }
    code = (UBYTE *)BADDR(seg) + sizeof(BPTR);
    data = (UBYTE *)BADDR(second) + sizeof(BPTR);
    bss = (UBYTE *)BADDR(third) + sizeof(BPTR);
    CopyMem((UBYTE *)BADDR(seg) - sizeof(ULONG), &prefix, sizeof(prefix));
    check(prefix == 12 + sizeof(ULONG) + sizeof(BPTR), "ULONG size prefix unchanged");
    if (sizeof(BPTR) > sizeof(ULONG))
        check((IPTR)BADDR(seg) % sizeof(BPTR) == sizeof(ULONG),
              "64-bit BPTR link really has only ULONG alignment");
    check((IPTR)(code + 2) % sizeof(ULONG) == 2, "relocated LONG really has word alignment");
    want = (ULONG)((IPTR)data + 4);
    if (kind == HUNK_RELRELOC32)
        want -= (ULONG)(IPTR)code;
    else if (kind == HUNK_ABSRELOC16)
        want -= (ULONG)((IPTR)code + 2);
    check(get32(code + 2) == want, "word-offset relocation value");
    check(code[0] == 0xa5 && code[1] == 0x5a && code[6] == 0x3c &&
          code[7] == 0xc3 && get32(code + 8) == 0x4e754e71,
          "bytes around relocated LONG unchanged");
    check(get32(data) == 0x11223344 && get32(data + 4) == 0x55667788,
          "DATA bytes unchanged");
    check(get32(bss) == 0 && get32(bss + 4) == 0, "BSS is zeroed");
    unload_image(seg);
}

int main(void)
{
    static const ULONG kinds[] = {
        HUNK_RELOC32, HUNK_RELRELOC32, HUNK_ABSRELOC16,
        HUNK_DREL32, HUNK_RELOC32SHORT
    };
    unsigned int i;
    BPTR seg;
    checks = failures = 0;
    for (i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++)
    {
        test_relocation(kinds[i], 0);
        test_relocation(kinds[i], 1);
    }
    for (i = 1; i <= 5; i++)
    {
        make_image(HUNK_RELOC32);
        fail_at = i;
        read_limit = 0;
        seg = load_image();
        check(seg == BNULL, "allocation failure rejects the load");
        if (seg)
            unload_image(seg);
        check(live == 0 && free_errors == 0, "allocation failure frees every owned block");
    }
    make_image(HUNK_RELOC32);
    length = 42; /* inside the first CODE payload, after allocation */
    fail_at = read_limit = 0;
    seg = load_image();
    check(seg == BNULL, "truncated payload rejects the load");
    if (seg)
        unload_image(seg);
    check(live == 0 && free_errors == 0, "truncated payload frees every owned block");
    check(InternalUnLoadSeg(BNULL, (VOID_FUNC)memory_free) == DOSFALSE,
          "NULL seglist remains rejected");
    printf("HUNKALIGNMENT: %u checks, %u failures\n", checks, failures);
    return failures ? RETURN_ERROR : RETURN_OK;
}

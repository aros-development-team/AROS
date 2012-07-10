
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/dos.h>


extern UBYTE copymem_040, copymeme_040;
extern UBYTE copymemquick_040, copymemquicke_040;
extern UBYTE copymem_060, copymeme_060;
extern UBYTE copymemquick_060, copymemquicke_060;

static void patch_function(UBYTE *start, UBYTE *end, void *library, WORD lvo)
{
    UBYTE *mem;
    ULONG size;

    size = end - start;
    mem = AllocMem(size, MEMF_PUBLIC);
    if (!mem)
        return;
    CopyMem(start, mem, size);
    CacheClearE(mem, size, CACRF_ClearI|CACRF_ClearD);
    SetFunction(library, lvo * -LIB_VECTSIZE, mem);
}

void patches(BOOL quiet, ULONG flags)
{
    if (!(flags & 1))
        return;
    if (SysBase->AttnFlags & AFF_68060) {
        patch_function(&copymem_060, &copymeme_060, SysBase, 104);
        patch_function(&copymemquick_060, &copymemquicke_060, SysBase, 105);
        if (!quiet)
            Printf("Replaced CopyMem() and CopyMemQuick() with 68060 optimized versions.\n");
    } else if (SysBase->AttnFlags & AFF_68040) {
        patch_function(&copymem_040, &copymeme_040, SysBase, 104);
        patch_function(&copymemquick_040, &copymemquicke_040, SysBase, 105);
        if (!quiet)
            Printf("Replaced CopyMem() and CopyMemQuick() with 68040 optimized versions.\n");
    }
}

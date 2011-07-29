#include <aros/asmcall.h>
#include <dos/dosextens.h>
#include <exec/libraries.h>
#include <proto/exec.h>

/* LoadSeg() needs D1-D3 parameters for overlay hunk support */
AROS_UFP4(BPTR, LoadSeg_Overlay,
    AROS_UFPA(UBYTE*, name, D1),
    AROS_UFPA(BPTR, hunktable, D2),
    AROS_UFPA(BPTR, fh, D3),
    AROS_UFPA(struct DosLibrary *, DosBase, A6));

extern void *BCPL_jsr, *BCPL_rts;

static int PatchDOS(struct DosLibrary *dosbase)
{
    UWORD highfunc = 37, lowfunc = 5, skipfuncs = 2;
    UWORD i;
    UWORD *asmcall;
    IPTR func;

    /* patch all 1.x dos functions to return value in both D1 and D0
     * For example most overlayed programs require this */
    asmcall = AllocMem(5 * (highfunc - lowfunc + 1 - skipfuncs) * sizeof(UWORD) + 12 * sizeof(UWORD), MEMF_PUBLIC);

    for (i = lowfunc; i <= highfunc; i++)
    {
    	if (i == 24 || i == 25)
    	    continue;
    	func = (IPTR)__AROS_GETJUMPVEC(dosbase, i)->vec;
    	asmcall[0] = 0x4eb9; // JSR
	asmcall[1] = (UWORD)(func >> 16);
	asmcall[2] = (UWORD)(func >>  0);
	asmcall[3] = 0x2200; // MOVE.L D0,D1
	asmcall[4] = 0x4e75; // RTS
 	__AROS_SETVECADDR(dosbase, i, asmcall);
 	asmcall += 5;
    }

    /* redirect LoadSeg() to LoadSeg_Overlay() if D1 == NULL */
    func = (IPTR)__AROS_GETJUMPVEC(dosbase, 25)->vec;
    asmcall[0] = 0x4a81; // TST.L D1
    asmcall[1] = 0x660a; // BNE.B 7 (D1 not NULL = normal LoadSeg)
    asmcall[2] = 0x4eb9; // JSR LoadSeg_Overlay
    asmcall[3] = (UWORD)((ULONG)LoadSeg_Overlay >> 16);
    asmcall[4] = (UWORD)((ULONG)LoadSeg_Overlay >>  0);
    asmcall[5] = 0x2200; // MOVE.L D0,D1
    asmcall[6] = 0x4e75; // RTS
    asmcall[7] = 0x4eb9; // JSR LoadSeg_Original
    asmcall[8] = (UWORD)(func >> 16);
    asmcall[9] = (UWORD)(func >>  0);
    asmcall[10] = 0x2200; // MOVE.L D0,D1
    asmcall[11] = 0x4e75; // RTS
    __AROS_SETVECADDR(dosbase, 25, asmcall);

    CacheClearU();

    dosbase->dl_A5 = (LONG)&BCPL_jsr;
    dosbase->dl_A6 = (LONG)&BCPL_rts;
    
    return TRUE;
}

ADD2INITLIB(PatchDOS, 0)

#include <aros/asmcall.h>
#include <dos/dosextens.h>
#include <exec/libraries.h>
#include <proto/exec.h>

#include "bcpl.h"

/* CallGlobVec lives in the private function (4) */
AROS_UFP5(LONG, CallGlobVec,
    AROS_UFPA(LONG, function, D0),
    AROS_UFPA(LONG, d1, D1),
    AROS_UFPA(LONG, d2, D2),
    AROS_UFPA(LONG, d3, D3),
    AROS_UFPA(LONG, d4, D4));


/* LoadSeg() needs D1-D3 parameters for overlay hunk support */
AROS_UFP4(BPTR, LoadSeg_Overlay,
    AROS_UFPA(UBYTE*, name, D1),
    AROS_UFPA(BPTR, hunktable, D2),
    AROS_UFPA(BPTR, fh, D3),
    AROS_UFPA(struct DosLibrary *, DosBase, A6));

extern void *BCPL_jsr, *BCPL_rts;
extern const ULONG BCPL_GlobVec[(BCPL_GlobVec_NegSize + BCPL_GlobVec_PosSize) >> 2];
    
const UWORD highfunc = 37, lowfunc = 5, skipfuncs = 2;

#define PATCHMEM_SIZE (10 * (highfunc - lowfunc + 1 - skipfuncs) * sizeof(UWORD) + 16 * sizeof(UWORD))

/* This patches two compatibility problems with badly written programs:
 * 1) Return value in both D0 and D1.
 * 2) 1.x original DOS functions can be called without DOSBase in A6.
 * Both "features" are original BCPL DOS side-effects.
 */

static int PatchDOS(struct DosLibrary *dosbase)
{
    UWORD i;
    UWORD *asmcall, *asmmem;
    IPTR func;
    APTR GlobVec;

    GlobVec = AllocMem(BCPL_GlobVec_NegSize + BCPL_GlobVec_PosSize, MEMF_PUBLIC);
    if (GlobVec == NULL)
        return FALSE;

    CopyMem(BCPL_GlobVec, GlobVec, BCPL_GlobVec_NegSize + BCPL_GlobVec_PosSize);
    GlobVec += BCPL_GlobVec_NegSize;
    *(APTR *)(GlobVec + GV_DOSBase) = dosbase;

    Forbid();

    /* Use this private slot for the C-to-BCPL thunk */
    __AROS_INITVEC(dosbase, 4);
    __AROS_SETVECADDR(dosbase, 4, CallGlobVec);

    asmmem = asmcall = AllocMem(PATCHMEM_SIZE, MEMF_PUBLIC);

    for (i = lowfunc; i <= highfunc; i++)
    {
     	if (i == 24 || i == 25)
    	    continue;
    	func = (IPTR)__AROS_GETJUMPVEC(dosbase, i)->vec;
 	__AROS_SETVECADDR(dosbase, i, asmcall);
 	*asmcall++ = 0x2f0e; // MOVE.L A6,-(SP)
	*asmcall++ = 0x4df9; // LEA dosbase,A6
	*asmcall++ = (UWORD)((ULONG)dosbase >> 16);
	*asmcall++ = (UWORD)((ULONG)dosbase >>  0);
	*asmcall++ = 0x4eb9; // JSR func
	*asmcall++ = (UWORD)(func >> 16);
	*asmcall++ = (UWORD)(func >>  0);
	*asmcall++ = 0x2C5F; // MOVE.L (SP)+,A6
	*asmcall++ = 0x2200; // MOVE.L D0,D1
	*asmcall++ = 0x4e75; // RTS
    }

    /* Redirect LoadSeg() to LoadSeg_Overlay() if D1 == NULL */
    func = (IPTR)__AROS_GETJUMPVEC(dosbase, 25)->vec;
    __AROS_SETVECADDR(dosbase, 25, asmcall);
    *asmcall++ = 0x2f0e; // MOVE.L A6,-(SP)
    *asmcall++ = 0x4df9; // LEA dosbase,A6
    *asmcall++ = (UWORD)((ULONG)dosbase >> 16);
    *asmcall++ = (UWORD)((ULONG)dosbase >>  0);
    *asmcall++ = 0x41f9; // LEA func,A0
    *asmcall++ = (UWORD)(func >> 16);
    *asmcall++ = (UWORD)(func >>  0);
    *asmcall++ = 0x4a81; // TST.L D1
    *asmcall++ = 0x6606; // BNE.B +6 (D1 not NULL = normal LoadSeg)
    *asmcall++ = 0x41f9; // LEA LoadSeg_Overlay,A0
    *asmcall++ = (UWORD)((ULONG)LoadSeg_Overlay >> 16);
    *asmcall++ = (UWORD)((ULONG)LoadSeg_Overlay >>  0);
    *asmcall++ = 0x4e90; // JSR (A0)
    *asmcall++ = 0x2C5F; // MOVE.L (SP)+,A6
    *asmcall++ = 0x2200; // MOVE.L D0,D1
    *asmcall++ = 0x4e75; // RTS

    CacheClearE(asmmem, PATCHMEM_SIZE, CACRF_ClearI|CACRF_ClearD);

    dosbase->dl_A5 = (LONG)&BCPL_jsr;
    dosbase->dl_A6 = (LONG)&BCPL_rts;
    dosbase->dl_GV = (APTR)GlobVec;
    
    Permit();

    return TRUE;
}

ADD2INITLIB(PatchDOS, 0)

static int UnPatchDOS(struct DosLibrary *dosbase)
{
    APTR asmcall;

    asmcall = __AROS_GETJUMPVEC(dosbase, lowfunc)->vec;
    FreeMem(asmcall, PATCHMEM_SIZE);
    FreeMem(dosbase->dl_GV - BCPL_GlobVec_NegSize, BCPL_GlobVec_NegSize + BCPL_GlobVec_PosSize);

    return TRUE;
}

ADD2EXPUNGELIB(UnPatchDOS, 0)

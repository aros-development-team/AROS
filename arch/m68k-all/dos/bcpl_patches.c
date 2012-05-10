
#include <aros/asmcall.h>
#include <string.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <dos/dos.h>
#include <proto/arossupport.h>
#include <proto/exec.h>
#include <proto/dos.h>

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

static AROS_ENTRY(LONG, SetPatch_noop,
	AROS_UFHA(char *, argstr, A0),
	AROS_UFHA(ULONG, argsize, D0),
	struct ExecBase *, SysBase)
{
    AROS_USERFUNC_INIT

    APTR DOSBase = TaggedOpenLibrary(TAGGEDOPEN_DOS);

    if (DOSBase) {
    	struct CommandLineInterface *cli = Cli();
    	if (cli && cli->cli_Interactive) {
    	    Printf("Only AROS m68k SetPatch is supported.\n");
    	}
    	CloseLibrary(DOSBase);
    }

    return RETURN_WARN;

    AROS_USERFUNC_EXIT
}

/* from C:Version */
static CONST_STRPTR FindSegmentVER(BPTR  Segment, LONG *length)
{
    while (Segment)
    {
        void        *MySegment;
        CONST_STRPTR    MyBuffer;
        ULONG       BufferLen;
        CONST_STRPTR    EndBuffer;
        CONST_STRPTR    SegmentEnd;

        MySegment   = BADDR(Segment);
        MyBuffer    = (CONST_STRPTR) (MySegment + sizeof(BPTR));
        BufferLen   = *(ULONG *)(MySegment - sizeof(ULONG));
        SegmentEnd  = (CONST_STRPTR) (MySegment + (BufferLen - sizeof(BPTR)));
        EndBuffer   = SegmentEnd - 5;

        while (MyBuffer < EndBuffer)
        {
            if (MyBuffer[0] == '$' &&
                MyBuffer[1] == 'V' &&
                MyBuffer[2] == 'E' &&
                MyBuffer[3] == 'R' &&
                MyBuffer[4] == ':')
            {
                CONST_STRPTR EndPtr;

                MyBuffer += 5;
                /* Required because some smartass could end his $VER: tag
                 * without '\0' in the segment to save space. - Piru
                 */
                for (EndPtr = MyBuffer; EndPtr < SegmentEnd && *EndPtr; EndPtr++)
                    ;
                if (EndPtr - MyBuffer)
                {
                    *length = EndPtr - MyBuffer;
                    return MyBuffer;
                }
            }

            MyBuffer++;
        }

        Segment =*(BPTR *)MySegment;
    }
    return NULL;
}

/* Check if SetPatch includes magic $VER: string */
static BSTR LoadSeg_Check_SetPatch(BSTR segs, struct DosLibrary *DOSBase)
{
    LONG len;
    CONST_STRPTR ver = FindSegmentVER(segs, &len);
    if (ver && len >= 18 + 1) {
        if (!memcmp (ver + 1, "SetPatch AROS-m68k", 18))
            return segs;
    }
    UnLoadSeg(segs);
    return CreateSegList(SetPatch_noop);
}

static AROS_UFH2(BPTR, NoReqLoadSeg,
    AROS_UFPA(BSTR, name, D1),
    AROS_UFPA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    BPTR ret = BNULL;

    if (name != BNULL) {
        struct Process *me = (struct Process *)FindTask(NULL);
        int len = AROS_BSTR_strlen(name);
        TEXT buff[len+1];
        APTR oldWindowPtr;

        CopyMem(buff, AROS_BSTR_ADDR(name), len);
        buff[len] = 0;

        oldWindowPtr = me->pr_WindowPtr;
        me->pr_WindowPtr = (APTR)-1;
        ret = LoadSeg(buff);
        me->pr_WindowPtr = oldWindowPtr;
    } else {
        SetIoErr(ERROR_OBJECT_NOT_FOUND);
    }

    return ret;

    AROS_USERFUNC_EXIT
}

AROS_UFH5(BPTR, LoadSeg_Check,
    AROS_UFPA(UBYTE*, name, D1),
    AROS_UFPA(BPTR, hunktable, D2),
    AROS_UFPA(BPTR, fh, D3),
    AROS_UFPA(APTR, LoadSeg_Original, A0),
    AROS_UFPA(struct DosLibrary *, DOSBase, A6))
{ 
    AROS_USERFUNC_INIT

    UBYTE *filename;

    /* name == NULL: Overlay LoadSeg */
    if (name == NULL)
        return AROS_UFC4(BPTR, LoadSeg_Overlay,
            AROS_UFCA(UBYTE*, name, D1),
            AROS_UFCA(BPTR, hunktable, D2),
            AROS_UFCA(BPTR, fh, D3),
            AROS_UFCA(struct DosLibrary *, DOSBase, A6));

    filename = FilePart(name);
    /* On m68k, we map AOS SetPatch to a no-op seglist,
     * due to the fact that OS 3.x and higher's SetPatch
     * blindly patches without checking OS versions.
     */
    if (stricmp(filename,"setpatch") == 0) {
        BSTR segs =  AROS_UFC2(BPTR, LoadSeg_Original,
        AROS_UFCA(UBYTE*, name, D1),
        AROS_UFCA(struct DosLibrary *, DOSBase, A6));
        if (segs == BNULL)
            return BNULL;
        return LoadSeg_Check_SetPatch (segs, DOSBase);
    }
    /* Do not allow Picasso96 to load, it is not
     * compatible with built-in AROS RTG system */
    if (stricmp(filename,"rtg.library") == 0)
    	return BNULL;

    /* Call original LoadSeg function */  
    return AROS_UFC2(BPTR, LoadSeg_Original,
        AROS_UFCA(UBYTE*, name, D1),
        AROS_UFCA(struct DosLibrary *, DOSBase, A6));

    AROS_USERFUNC_EXIT
}

extern void *BCPL_jsr, *BCPL_rts;
extern const ULONG BCPL_GlobVec[(BCPL_GlobVec_NegSize + BCPL_GlobVec_PosSize) >> 2];
    
const UWORD highfunc = 37, lowfunc = 5, skipfuncs = 2;

#define PATCHMEM_SIZE (10 * (highfunc - lowfunc + 1 - skipfuncs) * sizeof(UWORD) + 13 * sizeof(UWORD))

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

    /* NoReqLoadSeg() patch */
    SetFunction((struct Library *)dosbase, 28, NoReqLoadSeg);

    /* LoadSeg() patch */
    func = (IPTR)__AROS_GETJUMPVEC(dosbase, 25)->vec;
    __AROS_SETVECADDR(dosbase, 25, asmcall);
    *asmcall++ = 0x2f0e; // MOVE.L A6,-(SP)
    *asmcall++ = 0x4df9; // LEA dosbase,A6
    *asmcall++ = (UWORD)((ULONG)dosbase >> 16);
    *asmcall++ = (UWORD)((ULONG)dosbase >>  0);
    *asmcall++ = 0x41f9; // LEA func,A0
    *asmcall++ = (UWORD)(func >> 16);
    *asmcall++ = (UWORD)(func >>  0);
    *asmcall++ = 0x4eb9; // jsr LoadSeg_Check
    *asmcall++ = (UWORD)((ULONG)LoadSeg_Check >> 16);
    *asmcall++ = (UWORD)((ULONG)LoadSeg_Check >>  0);
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

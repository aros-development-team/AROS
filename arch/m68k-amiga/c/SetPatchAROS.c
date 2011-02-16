/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code to dynamically load ELF executables under AOS.
    Lang: english
*/

/* Compile for AROS:
 *
 * $ bin/linux-x86_64/tools/m68k-amiga-aros-gcc  -Os -I /path/to/src/AROS \
 *      SetPatchAROS.c -o SetPatchAROS.elf
 * $ bin/linux-x86_64/tools/elf2hunk SetPatchAROS.elf SetPatchAROS
 * 
 * Copy SetPatchAROS to your AOS installation, and add the following
 * to your startup script:
 *
 * Run SetPatchAROS
 *
 * Send a ^C to the SetPatchAROS to unload it.
 */

#define MINSTACK	8192

#include <aros/asmcall.h>
#include <dos/stdio.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <exec/rawfmt.h>

/*************  ExecBase Patches ********************/

static APTR oldRawDoFmt;
static AROS_UFH5(APTR, myRawDoFmt,
	AROS_UFHA(CONST_STRPTR, fmt, A0),
	AROS_UFHA(APTR,        args, A1),
	AROS_UFHA(VOID_FUNC,  putch, A2),
	AROS_UFHA(APTR,      putptr, A3),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    /* moveb %d0, %a3@+
     * rts
     */
    const ULONG m68k_string = 0x16c04e75;
    /* addql #1, %a3@
     * rts
     */
    const ULONG m68k_count  = 0x52934e75;
    /* jmp %a6@(-86 * 6)
     */
    const ULONG m68k_serial = 0x4eeefdfc;

    switch ((IPTR)putch) {
    case (IPTR)RAWFMTFUNC_STRING:
    	putch = (VOID_FUNC)&m68k_string;
    	break;
    case (IPTR)RAWFMTFUNC_COUNT:
    	putch = (VOID_FUNC)&m68k_count;
    	break;
    case (IPTR)RAWFMTFUNC_SERIAL:
    	putch = (VOID_FUNC)&m68k_serial;
    	break;
    default:
    	break;
    }

    return AROS_UFC5(APTR, oldRawDoFmt,
	AROS_UFCA(CONST_STRPTR, fmt, A0),
	AROS_UFCA(APTR,        args, A1),
	AROS_UFCA(VOID_FUNC,  putch, A2),
	AROS_UFCA(APTR,      putptr, A3),
	AROS_UFCA(struct ExecBase *, SysBase, A6));

    AROS_USERFUNC_EXIT
}

/*************  DosLibrary Patches ******************/

#include <rom/dos/internalloadseg_elf.c>

static AROS_UFH4(LONG, ReadFunc,
	AROS_UFHA(BPTR, file,   D1),
	AROS_UFHA(APTR, buffer, D2),
	AROS_UFHA(LONG, length, D3),
        AROS_UFHA(struct DosLibrary *, DOSBase, A6)
)
{
    AROS_USERFUNC_INIT

    return FRead(file, buffer, 1, length);

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(APTR, AllocFunc,
	AROS_UFHA(ULONG, length, D0),
	AROS_UFHA(ULONG, flags,  D1),
        AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    return AllocMem(length, flags);

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(void, FreeFunc,
	AROS_UFHA(APTR, buffer, A1),
	AROS_UFHA(ULONG, length, D0),
        AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    FreeMem(buffer, length);

    AROS_USERFUNC_EXIT
}

static APTR oldLoadSeg;
extern void myNewStackSwap(void);

static AROS_UFH2(BPTR, myLoadSeg,
	AROS_UFHA(CONST_STRPTR, name, D1),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
   AROS_USERFUNC_INIT

   SIPTR FunctionArray[3];
   BPTR file, segs;
   ULONG magic;

   FunctionArray[0] = (SIPTR)ReadFunc;
   FunctionArray[1] = (SIPTR)AllocFunc;
   FunctionArray[2] = (SIPTR)FreeFunc;

   file = Open (name, MODE_OLDFILE);
   if (file) {
       LONG len;

       SetVBuf(file, NULL, BUF_FULL, 4096);
       SetIoErr(0);

       /* Is it the ELF magic? */
       len = FRead(file, &magic, 1, sizeof(magic));
       if (len == sizeof(magic) && magic == 0x7f454c46) { /* ELF magic */
           struct StackSwapStruct sss;
           struct StackSwapArgs ssa;
           
           sss.stk_Lower = AllocMem(8192, MEMF_ANY);
           if (sss.stk_Lower == NULL) {
               Close(file);
               return NULL;
           }
           sss.stk_Upper = sss.stk_Lower + 8192;
           sss.stk_Pointer = sss.stk_Upper;
           ssa.Args[0] = (IPTR)file;
           ssa.Args[1] = (IPTR)BNULL;
           ssa.Args[2] = (IPTR)FunctionArray;
           ssa.Args[3] = (IPTR)NULL;
           ssa.Args[4] = (IPTR)DOSBase;

           segs = (BPTR)AROS_UFC4(IPTR, myNewStackSwap,
           	   	AROS_UFHA(struct StackSwapStruct *, &sss, A0),
           	   	AROS_UFHA(LONG_FUNC, InternalLoadSeg_ELF, A1),
           	   	AROS_UFHA(struct StackSwapArgs *, &ssa, A2),
           	   	AROS_UFHA(struct ExecBase *, SysBase, A6));
           FreeMem(sss.stk_Lower, 8192);

           if (segs) {
               if ((LONG)segs > 0) {
                   Close(file);
               } else {
                   segs = (BPTR)-((LONG)segs);
               }
               SetIoErr(0);
               return segs;
           }

           Close(file);
           return NULL;
       }
       Close(file);
   }

   return AROS_UFC2(BPTR, oldLoadSeg,
              AROS_UFCA(CONST_STRPTR, name, D1),
              AROS_UFCA(struct DosLibrary *, DOSBase, A6));

   AROS_USERFUNC_EXIT
}

static APTR oldCreateProc;
static AROS_UFH5(APTR, myCreateProc,
	AROS_UFHA(STRPTR *, name, D1),
	AROS_UFHA(LONG, priority, D2),
	AROS_UFHA(BPTR, seglist,  D3),
	AROS_UFHA(LONG, stacksize,D4),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    if (stacksize < MINSTACK)
    	stacksize = MINSTACK;
    else
    	stacksize += 1024;

    return AROS_UFC5(APTR, oldCreateProc,
	AROS_UFCA(STRPTR *, name, D1),
	AROS_UFCA(LONG, priority, D2),
	AROS_UFCA(BPTR, seglist,  D3),
	AROS_UFCA(LONG, stacksize,D4),
	AROS_UFCA(struct DosLibrary *, DOSBase, A6));

   AROS_USERFUNC_EXIT
}

static APTR oldCreateNewProc;
static AROS_UFH2(APTR, myCreateNewProc,
	AROS_UFHA(struct TagItem *, tag, D1),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    struct TagItem *tstate, *tmp, *found, fake[2];
    int tags;

    tstate = tag;
    found = NULL;
    tags = 0;

    while ((tmp = NextTagItem(&tstate)) != NULL) {
    	if (tmp->ti_Tag == NP_StackSize)
    		found = tmp;
    	tags++;
    }

    Printf("Tags:  %ld\n", (LONG)tags);
    if (found == NULL) {
    	Printf("Fake: (0x%lx %ld)\n", (LONG)NP_StackSize, MINSTACK);
	fake[0].ti_Tag = NP_StackSize;
	fake[0].ti_Data = MINSTACK;
	fake[1].ti_Tag = TAG_MORE;
	fake[1].ti_Data = (IPTR)tag;
	tag = &fake[0];
    } else {
    	Printf("Found: 0x%lx (%ld)\n", (LONG)found, found->ti_Data);
    	if (found->ti_Data <= MINSTACK)
    	    found->ti_Data = MINSTACK;
    	else
    	    found->ti_Data += 1024;
    }
    return AROS_UFC2(APTR, oldCreateNewProc,
    	    AROS_UFCA(struct TagItem *, tag, D1),
    	    AROS_UFCA(struct DosLibrary *, DOSBase, A6));

    AROS_USERFUNC_EXIT
}

/* Really stupid way to deal with this,
 * but what we're going to do is make our
 * patches, and wait for a ^C.
 *
 * When we get a ^C, unpatch ourselves from
 * Dos/LoadSeg(), and exit.
 *
 * To use:
 *
 * > Run SetPatchAROS
 */
int main(int argc, char **argv)
{
   APTR DOSBase;

   DOSBase = OpenLibrary("dos.library", 0);
   if (DOSBase != NULL) {
       struct Library *sbl = (APTR)SysBase;
       if (sbl->lib_Version > 40) {
       	   FPrintf(Error(), "SetPatchAROS: Unsupported exec.library %ld.%ld\n",
       	   	   sbl->lib_Version, sbl->lib_Revision);
       	   CloseLibrary((APTR)DOSBase);
       	   return RETURN_ERROR;
       }

       Disable();

       oldRawDoFmt      = SetFunction(SysBase, -87 * LIB_VECTSIZE, myRawDoFmt);
       oldCreateProc    = SetFunction(DOSBase, -23 * LIB_VECTSIZE, myCreateProc);
       oldCreateNewProc = SetFunction(DOSBase, -83 * LIB_VECTSIZE, myCreateNewProc);
       oldLoadSeg       = SetFunction(DOSBase, -25 * LIB_VECTSIZE, myLoadSeg);
       Enable();
       PutStr("AROS Support active. Press ^C to unload.\n");
       Wait(SIGBREAKF_CTRL_C);
       Disable();
       SetFunction(DOSBase, -25 * LIB_VECTSIZE, oldLoadSeg);
       SetFunction(DOSBase, -83 * LIB_VECTSIZE, oldCreateNewProc);
       SetFunction(DOSBase, -23 * LIB_VECTSIZE, oldCreateProc);
       SetFunction(SysBase, -87 * LIB_VECTSIZE, oldRawDoFmt);
       Enable();
       PutStr("AROS Support unloaded.\n");
       CloseLibrary(DOSBase);
   }

   return RETURN_OK;
}

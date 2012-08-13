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

#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <exec/rawfmt.h>

#include <loadseg.h>

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

#define PROTO_KERNEL_H      /* Don't pick up AROS kernel hooks */

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
       	   FPrintf(Output(), "SetPatchAROS: Unsupported exec.library %ld.%ld\n",
       	   	   sbl->lib_Version, sbl->lib_Revision);
       	   CloseLibrary((APTR)DOSBase);
       	   return RETURN_ERROR;
       }

       Disable();
       oldRawDoFmt      = SetFunction((struct Library *)SysBase, -87 * LIB_VECTSIZE, myRawDoFmt);
       Enable();

       PutStr("AROS Support active. Press ^C to unload.\n");
       Wait(SIGBREAKF_CTRL_C);

       Disable();
       SetFunction((struct Library *)SysBase, -87 * LIB_VECTSIZE, oldRawDoFmt);
       Enable();

       PutStr("AROS Support unloaded.\n");
       CloseLibrary(DOSBase);
   }

   return RETURN_OK;
}

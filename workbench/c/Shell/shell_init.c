/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Shell Resource
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/symbolsets.h>
#include <aros/shcommands.h>

THIS_PROGRAM_HANDLES_SYMBOLSET(SHCOMMANDS)
DEFINESET(SHCOMMANDS)

#include LC_LIBDEFS_FILE

#undef DOSBase

extern void ShellStart(void);

#ifdef __mc68000
/* Under AOS, the "CLI" segment has a BCPL entry,
 * while the "Shell" segment has a C entry point.
 *
 * This small stub 'thunks' from BCPL to C,
 * and back again.
 */
extern void Shell_CLISeg(void);
asm (
        "       .balign 4\n"
        "       .global Shell_CLISeg\n"
        "       .long   ((Shell_CLI_end - Shell_CLI_start) / 4)+1\n"
        "Shell_CLISeg:\n"
        "       .long   0\n"    /* No next BCPL segment */
        "       .long   ((Shell_CLI_end - Shell_CLI_start) / 4)+1\n"
        "Shell_CLI_start:\n"
        "       movem.l %a0-%a1/%a6, %sp@-\n"
        "       lsl.l   #2, %d1\n"
        "       move.l  %d1, %a0\n"
        "       jsr     ShellStart\n"
        "       movem.l %sp@+,%a0-%a1/%a6\n"
        "       move.l  %d0, %d1\n"
        "       jmp     %a6@\n"
        "       .balign 4\n"
        "       .long   0\n"    /* End of setup data */
        "       .long   1\n"    /* Update BCPL Entry (GV #1) */
        "       .long   4\n"    /* Offset of CLI start */
        "       .long   2\n"    /* Need a minimum of 2 GV slots */
        "Shell_CLI_end:\n"
    );
#endif

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    APTR DOSBase;
    BPTR seg;

    D(bug("[Shell] Init\n"));

    DOSBase = OpenLibrary("dos.library", 0);
    if ( DOSBase == NULL ) {
    	D(bug("[Shell] What? No dos.library?\n"));
    	return FALSE;
    }

    seg = CreateSegList(ShellStart);
    if (seg != BNULL) {
	AddSegment("shell", seg, CMD_SYSTEM);
#ifdef __mc68000
	AddSegment("CLI", MKBADDR(Shell_CLISeg), CMD_SYSTEM);
#else
	AddSegment("CLI", seg, CMD_SYSTEM);
#endif
    }
 
    CloseLibrary(DOSBase);

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0);

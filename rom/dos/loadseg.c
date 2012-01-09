/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DOS function LoadSeg()
    Lang: english
*/

#include <aros/asmcall.h>
#include <aros/config.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <aros/debug.h>
#include "dos_intern.h"

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

static AROS_UFH4(LONG, SeekFunc,
	AROS_UFHA(BPTR, file,  D1),
	AROS_UFHA(LONG,   pos, D2),
	AROS_UFHA(LONG,  mode, D3),
        AROS_UFHA(struct DosLibrary *, DOSBase, A6)
)
{
    AROS_USERFUNC_INIT

    return Seek(file, pos, mode);

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

#ifdef __mc68000
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
    	    Printf("SetPatch is a reserved program name.\n");
    	}
    	CloseLibrary(DOSBase);
    }

    return RETURN_WARN;

    AROS_USERFUNC_EXIT
}
#endif

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(BPTR, LoadSeg,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 25, Dos)

/*  FUNCTION
        Loads an executable file into memory. Each hunk of the loadfile
        is loaded into its own memory section and a handle on all of them
        is returned. The segments can be freed with UnLoadSeg().

    INPUTS
        name - NUL terminated name of the file.

    RESULT
        Handle to the loaded executable or NULL if the load failed.
        IoErr() gives additional information in that case.

    NOTES
        This function is built on top of InternalLoadSeg()

    EXAMPLE

    BUGS

    SEE ALSO
        UnLoadSeg()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR file, segs=0;
    SIPTR err;
    LONG_FUNC FunctionArray[] = {
    	(LONG_FUNC)ReadFunc,
    	(LONG_FUNC)AllocFunc,
    	(LONG_FUNC)FreeFunc,
    	(LONG_FUNC)SeekFunc,	/* Only needed for ELF */
    };

#ifdef __mc68000
    /* On m68k, we map SetPatch to a no-op seglist,
     * due to the fact that OS 3.x and higher's SetPatch
     * blindly patches without checking OS versions.
     */
    if (Stricmp(FilePart(name),"SetPatch") == 0)
    	return CreateSegList(SetPatch_noop);
    /* Do not allow Picasso96 to load, it is not
     * compatible with built-in AROS RTG system */
    if (Stricmp(FilePart(name),"rtg.library") == 0)
    	return BNULL;
#endif

    /* Open the file */
    D(bug("[LoadSeg] Opening '%s'...\n", name));
    file = Open (name, MODE_OLDFILE);

    if (file)
    {
	D(bug("[LoadSeg] Loading '%s'...\n", name));

	SetVBuf(file, NULL, BUF_FULL, 4096);
	segs = InternalLoadSeg(file, BNULL, FunctionArray, NULL);
	/* We cache the IoErr(), since Close() will alter it */
	err = IoErr();

	D(if (segs == BNULL)
 	    bug("[LoadSeg] Failed to load '%s'\n", name));
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
 	/* overlayed executables return -segs and handle must not be closed */
 	if ((LONG)segs > 0)
 	    Close(file);
 	else
 	    segs = (BPTR)-((LONG)segs);
#else
        Close(file);
#endif
        SetIoErr(err);
    }
  D(else
        bug("[LoadSeg] Failed to open '%s'\n", name));
  

    /* And return */
    return segs;

    AROS_LIBFUNC_EXIT
} /* LoadSeg */

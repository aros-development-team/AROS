/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: DOS function LoadSeg()
    Lang: english

    Revision 1.14
      LoadSeg now simply calls InternalLoadSeg() with the
      array of functions
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
/* iaint: Sigh, I'm getting sick of this...
#undef DEBUG
#define DEBUG 1
*/
#	include <aros/debug.h>
#include "dos_intern.h"

extern LONG Dos_Read();
extern void * Exec_AllocMem();
extern void Exec_FreeMem();

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(BPTR, LoadSeg,

/*  SYNOPSIS */
        AROS_LHA(STRPTR, name, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 25, Dos)

/*  FUNCTION
        Loads an executable file into memory. Each hunk of the loadfile
        is loaded into his own memory section and a handle on all of them
        is returned. The segments can be freed with UnLoadSeg().

    INPUTS
        name - NUL terminated name of the file.

    RESULT
        Handle to the loaded executable or 0 if the load failed.
        IoErr() gives additional information in that case.

    NOTES
        This function is built on top of InternalLoadSeg()

    EXAMPLE

    BUGS

    SEE ALSO
        UnLoadSeg()

    INTERNALS

    HISTORY
        29-10-95    digulla automatically created from
                            dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

  void (* FunctionArray[])() = { (void *)&Dos_Read,
                                 (void *)&Exec_AllocMem,
                                 (void *)&Exec_FreeMem  };

  BPTR file, segs=0;

  /* Open the file */
  file = Open (name,MODE_OLDFILE);

  if (file)
  {
D(bug("Loading \"%s\"...\n", name));

    segs = InternalLoadSeg (file, NULL, (void *)FunctionArray, NULL);

    if (segs)
      SetIoErr (0);
#if DEBUG > 1
    else
      bug ("Loading failed\n");
#endif
    Close(file);
  }

  /* And return */
  return segs;

  AROS_LIBFUNC_EXIT
} /* LoadSeg */

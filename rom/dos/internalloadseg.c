/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DOS function InternalLoadSeg()
    Lang: english
*/


#define DEBUG 0

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <aros/debug.h>
#include "dos_intern.h"
#include "internalloadseg.h"

#if AROS_MODULES_DEBUG
#include <exec/nodes.h>
#include <exec/lists.h>
#include <string.h>

struct MinList debug_seglist, free_debug_segnodes;

#endif

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH4(BPTR, InternalLoadSeg,

/*  SYNOPSIS */
        AROS_LHA(BPTR     , fh           , D0),
        AROS_LHA(BPTR     , table        , A0),
        AROS_LHA(LONG_FUNC, functionarray, A1),
        AROS_LHA(LONG *   , stack        , A2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 126, Dos)

/*  FUNCTION
        Loads from fh.
        Functionarray is a pointer to an array of functions. See below.

        This function really only tries to load the different file
        formats aos, elf and aout.

    INPUTS
        fh            : Filehandle to load from
        table         : ignored
        functionarray : array of functions to be used for read, alloc and free
           FuncTable[0] -> bytes  = ReadFunc(readhandle, buffer, length), DOSBase
                           D0                D1          A0      D0       A6
           FuncTable[1] -> Memory = AllocFunc(size,flags), ExecBase
                           D0                 D0   D1      A6
           FuncTable[2] -> FreeFunc(memory, size), ExecBase
                                    A1       D0    A6
        stack         : pointer to storage (ULONG) for stacksize.
                        (currently ignored)

    RESULT
        seglist  - pointer to loaded Seglist or NULL in case of failure.

    NOTES

    EXAMPLE

    BUGS
       Use of table and stack are not implemented, yet!

    SEE ALSO
        UnLoadSeg()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    typedef struct _segfunc_t
    {
        BPTR (*func)(BPTR, BPTR, SIPTR *, SIPTR *,
		     struct MinList *, struct DosLibrary *);
        D(CONST_STRPTR format;)
    } segfunc_t;

    #define SEGFUNC(format) { InternalLoadSeg_##format D(, (STRPTR)#format)}
    
    static const segfunc_t funcs[] = 
    {
        SEGFUNC(ELF),
        SEGFUNC(ELF64),
    #if !defined(__mc68000__) && !defined(__arm__)
        SEGFUNC(ELF_AROS),
    #endif      
        SEGFUNC(AOS),
        SEGFUNC(AOUT)
    };
  
    BPTR segs = 0;

    if (fh)
    {
        int i = 0;
	const int num_funcs = sizeof(funcs)/sizeof(funcs[0]);
	struct MinList *pseginfos;
#if AROS_MODULES_DEBUG
	struct MinList seginfos;

	NEWLIST(&seginfos);
	pseginfos = &seginfos;
#else
	pseginfos = NULL;
#endif
      
	do
	{
	    SetIoErr(0);
	   
	    segs = (*funcs[i].func)(fh, MKBADDR(NULL), (SIPTR *)functionarray,
				    NULL, pseginfos, DOSBase);
            
	    D(bug("[InternalLoadSeg] %s loading %p as an %s object.\n",
	          segs ? "Succeeded" : "FAILED", fh, funcs[i].format));
 	     
	} while	(!segs && (IoErr() == ERROR_NOT_EXECUTABLE) && (++i < num_funcs));

#if AROS_MODULES_DEBUG
	if(segs)
	{
	    struct debug_segnode *segnode;

	    Forbid();
	    segnode = (struct debug_segnode *)REMHEAD(&free_debug_segnodes);
	    Permit();

	    if (segnode)
	    {
		struct seginfo *si;

		NameFromFH(fh, segnode->name, sizeof(segnode->name));
		D(bug("[InternalLoadSeg] loaded: %s\n", segnode->name));

		segnode->seglist = segs;

		/* copy the segments info list */
		NEWLIST(&segnode->seginfos);
		while ((si = (struct seginfo *)REMHEAD(&seginfos)))
		    ADDTAIL(&segnode->seginfos, si);

#if defined(__AROS_SET_START_ADDR)
		__AROS_SET_START_ADDR(segnode);
#else
#warning "if you want gdb debugging of loaded executables implement __AROS_GET_START_ADDR in machine.h"
#endif

		Forbid();
		ADDTAIL(&debug_seglist, segnode);
		Permit();
	    }
	}
#endif
    }

    return segs;
  
    AROS_LIBFUNC_EXIT
} /* InternalLoadSeg */

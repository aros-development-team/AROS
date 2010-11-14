/*
**	$VER: examplebase.h 37.15 (14.8.97)
**
**	definition of ExampleBase
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/

#ifndef EXAMPLE_EXAMPLEBASE_H
#define EXAMPLE_EXAMPLEBASE_H

#ifdef	 __MAXON__
#   ifndef  EXEC_LIBRARIES
#	include <exec/libraries.h>
#   endif
#   ifndef  DOS_DOS
#	include <dos/dos.h>
#   endif
#else
#   ifndef  EXEC_LIBRARIES_H
#	include <exec/libraries.h>
#   endif /* EXEC_LIBRARIES_H */
#   ifndef  DOS_DOS_H
#	include <dos/dos.h>
#   endif /* DOS_DOS_H */
#endif

#ifndef LIBCORE_BASE_H
#   include <libcore/base.h>
#endif

struct ExampleBase
{
    /* This is the header which is the same for all libraries */
    struct LibHeader	   exb_LibHeader;

    /* This is the library specific data. */
    struct IntuitionBase  *exb_IntuitionBase;
    struct GfxBase	  *exb_GfxBase;
};

#endif /* EXAMPLE_EXAMPLEBASE_H */

/*
**	$VER: examplebase.h 37.1 (4.12.96)
**
**	definition of ExampleBase
**
**	(C) Copyright 1996 Andreas R. Kleinert
**	All Rights Reserved.
*/

#ifndef EXAMPLE_EXAMPLEBASE_H
#define EXAMPLE_EXAMPLEBASE_H

#ifdef	 __MAXON__
#ifndef  EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#else
#ifndef  EXEC_LIBRARIES
#include <exec/libraries.h>
#endif /* EXEC_LIBRARIES_H */
#endif

struct ExampleBase
{
    struct Library	   exb_LibNode;
    APTR		   exb_SegList;
    struct ExecBase	  *exb_SysBase;
    struct IntuitionBase  *exb_IntuitionBase;
    struct GfxBase	  *exb_GfxBase;
};

#endif /* EXAMPLE_EXAMPLEBASE_H */

#ifndef LIBCORE_BASE_H
#define LIBCORE_BASE_H

/*
**	$VER: base.h 37.15 (14.8.97)
**
**	Common structure header for all libraries.
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/

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

struct LibHeader
{
    struct Library	   lh_LibNode;
    BPTR		   lh_SegList;
    struct ExecBase	  *lh_SysBase;
};

#endif /* LIBCORE_BASE_H */

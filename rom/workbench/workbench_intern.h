/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.1  1999/08/14 04:52:35  bergers
    Empty but compilable libarary. None of the functions have been implemented.


    Desc: Internal header file for workbench library
    Lang: english
*/
#ifndef __WORKBENCH_INTERN_H__
#define __WORKBENCH_INTERN_H__

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef DOS_DOS_H
#	include <dos/dos.h>
#endif
#ifndef INTUITION_INTUITION_H
#	include <intuition/intuition.h>
#endif

#include <aros/debug.h>

/*
    This is the WorkbenchBase structure. It is documented here because 
    it is completely private. Applications should treat it as a struct 
    Library, and use the workbench.library functions to get information.
*/

extern struct ExecBase * SysBase;

struct WorkbenchBase
{
    struct Library   	LibNode;
    BPTR	     	wb_SegList;
    struct ExecBase  *	wb_SysBase;
};

/*
 * Defintion of internal structures.
 */

struct AppWindow 
{
  struct Window * aw_window; /* just to have something here... change!! */
};

struct AppIcon
{
};

struct AppMenuItem
{
};

#endif /* __WORKBENCH_INTERN_H__  */


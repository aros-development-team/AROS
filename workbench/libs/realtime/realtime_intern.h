/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.1  2000/03/18 18:27:47  bergers
    Empty libraries. None of the functions have been implemented, yet, but I hope that someone will do it. :-)



    Desc: Internal header file for realtime library
    Lang: english
*/
#ifndef __REALTIME_INTERN_H__
#define __REALTIME_INTERN_H__

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif

#include <aros/debug.h>

extern struct ExecBase * SysBase;

struct RealtimeBase
{
    struct Library   	LibNode;
    BPTR	     	wb_SegList;
    struct ExecBase  *	wb_SysBase;
};

/*
 * Defintion of internal structures.
 */

#endif /* __REALTIME_INTERN_H__  */


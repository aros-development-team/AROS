/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/10/24 15:51:39  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.5  1996/09/13 17:10:55  aros
    Don't use the TOLOWER() and TOUPPER() macros directly.

    Revision 1.4  1996/09/12 14:52:47  digulla
    Better way to separate public and private parts of the library base

    Revision 1.3  1996/08/31 12:58:14  aros
    Merged in/modified for FreeBSD.

    Revision 1.2  1996/08/01 17:41:42  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef UTILITY_INTERN_H
#define UTILITY_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif
#ifndef EXEC_IO_H
#include <exec/io.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif
#ifndef UTILITY_NAME_H
#include <utility/name.h>
#endif
#ifndef UTILITY_DATE_H
#include <utility/date.h>
#endif
#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif
#ifndef CLIB_EXEC_PROTOS_H
#include <clib/exec_protos.h>
#endif
#ifndef CLIB_UTILITY_PROTOS_H
#include <clib/utility_protos.h>
#endif

/*
    This is the internal version of the UtilityBase structure
*/

struct IntUtilityBase
{
    struct UtilityBase UBase;

    /*
       This is where the private data starts.
    */
    struct ExecBase	*ub_SysBase;
    ULONG		 ub_LastID;

    struct NamedObject	*ub_GlobalNameSpace;

    /*
	This should always be at the end, and it is only valid when the
	library is loaded from disk, eg testing...

	The reference is still in here of course, it just means that
	when I change the library base I will have to recompile most of
	the files, oh well...
    */
    BPTR		 ub_SegList;
};

/* digulla again... Needed for close() */
#define expunge() \
 AROS_LC0(BPTR, expunge, struct UtilityBase *, UtilityBase, 3, Utility)

#define GetIntUtilityBase(ub)   ((struct IntUtilityBase *)(ub))
#define GetUtilityBase(ub)      (GetIntUtilityBase(ub)->UBase)

#define SysBase 	(GetIntUtilityBase(UtilityBase)->ub_SysBase)

#endif

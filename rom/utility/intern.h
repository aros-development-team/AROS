/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/31 12:58:14  aros
    Merged in/modified for FreeBSD.

    Revision 1.2  1996/08/01 17:41:42  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef UTILITY_INTERN_H
#define UTILITY_INTERN_H

#define UTILITY_UTILITY_H

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
#ifndef CLIB_EXEC_PROTOS_H
#include <clib/exec_protos.h>
#endif
#ifndef CLIB_UTILITY_PROTOS_H
#include <clib/utility_protos.h>
#endif

/* digulla
        I could use a table for this conversion, but the utility.library
        functions are patched by locale anyway, so this would be some
        kind of overkill.
*/

#define TOLOWER(a) \
(((a)>='A'&&(a)<='Z') || ((a)>=0xc0 && (a)<=0xde && (a)!=0xd7) ? (a)+0x20 : (a) )

#define TOUPPER(a) \
(((a)>='a'&&(a)<='z') || ((a)>=0xe0 && (a)<=0xfe && (a)!=0xf7) ? (a)+0x20 : (a) )

/* iaint from here... */

/*
    This is the internal version of the UtilityBase structure
    It overrides the definition in utility/utility.h
*/


#define UTILITYNAME "utility.library"

struct UtilityBase
{
    struct Library       ub_LibNode;
    UBYTE                ub_Language;
    UBYTE                ub_Reserved;

    /*
       This is where the private data starts.
    */
    struct ExecBase     *ub_SysBase;
    ULONG                ub_LastID;

    struct NamedObject  *ub_GlobalNameSpace;

    /*
        This should always be at the end, and it is only valid when the
        library is loaded from disk, eg testing...

        The reference is still in here of course, it just means that
        when I change the library base I will have to recompile most of
        the files, oh well...
    */
    ULONG                ub_SegList;
};


/* digulla again... Needed for close() */
#define expunge() \
 __AROS_LC0(BPTR, expunge, struct UtilityBase *, UtilityBase, 3, Utility)

#endif

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1997/01/08 03:36:14  iaint
    A few more utility.lib functions

    Revision 1.7  1996/12/18 01:27:36  iaint
    NamedObjects

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
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef UTILITY_NAME_H
#include <utility/name.h>
#endif
#ifndef UTILITY_DATE_H
#include <utility/date.h>
#endif
#ifndef UTILITY_PACK_H
#include <utility/pack.h>
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
#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
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
    BPTR                 ub_SegList;
};

/* digulla again... Needed for close() */
#define expunge() \
 AROS_LC0(BPTR, expunge, struct UtilityBase *, UtilityBase, 3, Utility)

#define GetIntUtilityBase(ub)   ((struct IntUtilityBase *)(ub))
#define GetUtilityBase(ub)      (&GetIntUtilityBase(ub)->UBase)

#define SysBase         (GetIntUtilityBase(UtilityBase)->ub_SysBase)

/*
    Internal versions of the NamedObject structures.
    Access using GetIntNamedObject()

    I have a problem here of sorts. The autodocs do not give enough
    information really. They say it is possible to nest NamedObjects
    with namespaces, however the implementation didn't allow NamedObjects
    with their own namespaces to be members of a namespace.

    However, the autodocs are right, so you can nest like that.
*/

struct NameSpace
{
    struct MinList          ns_List;
    struct SignalSemaphore  ns_Lock;
    ULONG                   ns_Flags;
};

struct IntNamedObject
{
    struct NamedObject  no;

    struct Node         no_Node;
    struct NameSpace   *no_ParentSpace;     /* The NameSpace I am in */
    struct NameSpace   *no_NameSpace;       /* My NameSpace */
    struct Message     *no_FreeMessage;
    UWORD               no_UseCount;
    BOOL                no_FreeObject;
};

#define GetIntNamedObject(no)   ((struct IntNamedObject *)(no))
#define GetNamedObject(no)      (&GetIntNamedObject(no)->no)


/* Internal function prototypes */
struct NameSpace *GetNameSpace(struct NamedObject *, struct UtilityBase *);
struct IntNamedObject *IntFindNamedObj(struct NameSpace *, struct Node *, STRPTR, struct UtilityBase *);

#endif

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal information for utility.library.
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
#ifndef AROS_DEBUG_H
#include <aros/debug.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif
#ifndef PROTO_ALIB_H
#include <proto/alib.h>
#endif
#ifndef PROTO_EXEC_H
#include <proto/exec.h>
#endif
#ifndef PROTO_UTILITY_H
#include <proto/utility.h>
#endif

/* Definition of a utility namespace, needed here for the library base. */

struct NameSpace
{
    struct MinList          ns_List;
    struct SignalSemaphore  ns_Lock;
    ULONG                   ns_Flags;
};

/*
    This is the internal version of the UtilityBase structure
*/

struct IntUtilityBase
{
    struct UtilityBase UBase;

    /* This is where the private data starts. */
    ULONG                ub_LastID;
    struct NameSpace     ub_NameSpace;
};

#define GetIntUtilityBase(ub)   ((struct IntUtilityBase *)(ub))
#define GetUtilityBase(ub)      (&GetIntUtilityBase(ub)->UBase)

/*
    Internal versions of the NamedObject structures.
    Access using GetIntNamedObject()

    I have a problem here of sorts. The autodocs do not give enough
    information really. They say it is possible to nest NamedObjects
    with namespaces, however the implementation didn't allow NamedObjects
    with their own namespaces to be members of a namespace.

    However, the autodocs are right, so you can nest like that.
*/

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

/*
    Access union to get a some memory in Pack|UnpackStructureTags(). I use
    this because
    a) it's neater.
    b) it avoids any bizarre aliasing problems in the compiler.
    c) did I say it makes stuff easier to read? :-)
*/
union memaccess
{
    UBYTE   ub;
    UWORD   uw;
    ULONG   ul;
    BYTE    sb;
    WORD    sw;
    LONG    sl;
};

#endif

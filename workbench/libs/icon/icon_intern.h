#ifndef ICON_INTERN_H
#define ICON_INTERN_H

/* Include files */
#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif
#ifndef CLIB_ALIB_PROTOS_H
#   include <clib/alib_protos.h>
#endif
#ifndef PROTO_ICON_H
#   include <proto/icon.h>
#endif
#ifndef WORKBENCH_WORKBENCH_H
#   include <workbench/workbench.h>
#endif
#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#include <string.h>

/* Internal prototypes */
VOID	GetDefIconName (LONG, UBYTE *);
UBYTE * WriteValue     (LONG, UBYTE *);

/* Constants */
#define MAX_DEFICON_FILEPATH	256

/* Number of entries in the mementrys in the freelists */
#define FREELIST_MEMLISTENTRIES 10

/* To get right alignment we make our very own memlist structur
Look at the original struct MemList in exec/memory.h to see why */

struct IconInternalMemList
{
    struct Node iiml_Node;
    UWORD   iiml_NumEntries;
    struct MemEntry iiml_ME[FREELIST_MEMLISTENTRIES];
};

struct IconBase
{
    struct Library    library;
    struct ExecBase * sysbase;
    BPTR	      seglist;
    struct Library  * dosbase;
    struct Library  * utilitybase;
};

#undef SysBase
#define SysBase     (((struct IconBase *)IconBase)->sysbase)
#undef DOSBase
#define DOSBase     (((struct IconBase *)IconBase)->dosbase)
#undef UtilityBase
#define UtilityBase	(((struct IconBase *)IconBase)->utilitybase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct IconBase *, IconBase, 3, Icon)

#endif /* ICON_INTERN_H */

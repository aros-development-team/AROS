#ifndef ICON_INTERN_H
#define ICON_INTERN_H

/* Include files */
#ifndef CLIB_ALIB_PROTOS_H
#   include <proto/alib.h>
#endif
#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
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
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif
#ifndef LIBCORE_BASE_H
#   include <libcore/base.h>
#endif
#include <string.h>

/* Internal prototypes */
AROS_UFP3(LONG, dosstreamhook,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(BPTR,            file, A2),
    AROS_UFPA(ULONG *,         msg, A1)
);
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

extern struct ExecBase * SysBase;

struct IconBase
{
    struct LibHeader  libheader;

    /* Private parts */
#if 0
    struct Library  * dosbase;
#endif
    struct Library  * utilitybase;
    struct Hook       dsh;
};

#define LB(icon)        ((struct IconBase *)icon)
#if 0
#undef DOSBase
#define DOSBase     (((struct IconBase *)IconBase)->dosbase)
#else
extern struct DosLibrary * DOSBase;
#endif
#undef UtilityBase
#define UtilityBase	(((struct IconBase *)IconBase)->utilitybase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct IconBase *, IconBase, 3, Icon)

#endif /* ICON_INTERN_H */

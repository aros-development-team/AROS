/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSLIST_INTERN_H
#define AROSLIST_INTERN_H


#undef  AROS_ALMOST_COMPATIBLE 
#define AROS_ALMOST_COMPATIBLE 1

#ifndef EXEC_TYPES_H
#    include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#    include <exec/libraries.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef UTILTY_HOOKS_H
#   include <utility/hooks.h>
#endif

/* Predeclaration */
struct ListBase_intern;

struct ListEntry
{
    struct ListEntry *le_Next;
    UBYTE	      le_Flags;
    
    APTR	     le_Item; /* The inserted item */
};

/* Flag bits for le_Flags */
#define LEFLG_SELECTED (1 << 0)


struct ListData
{
    LONG		ld_NumEntries;
    /* How any entries are pssible to put into currently  allocated buffer */
    LONG 		ld_NumAllocated; 
    struct ListEntry 	*ld_EntryTables;
    struct ListEntry 	*ld_UnusedEntries;
    struct ListEntry 	**ld_PointerArray;
    struct Hook		*ld_ConstructHook;
    struct Hook		*ld_DestructHook;
    APTR		ld_Pool;
    LONG		ld_Active;
};


#define LD(x) ((struct ListData *)x)

#undef UB
#define UB(x) ((UBYTE *)x)

#define NUMENTRIES_TO_ADD 100


/* Macro to add a filentry at the head of a filentry list */
#define AddLEHead(l, le)	\
    le->le_Next = l;		\
    l = le;

#define NewLEList(l) \
l = NULL;

#define IsLEListEmpty(l) \
(l != NULL)

#define RemLEHead(l) \
l = l->le_Next;


/**************/
/* Prototypes */
/**************/

struct ListEntry **AllocEntries(ULONG, struct ListData *, struct ListBase_intern *);
ULONG InsertItems(APTR *, struct ListEntry **, LONG, struct ListData *, struct ListBase_intern *);
ULONG CountItems(APTR *);

struct ListBase_intern
{
    struct Library 	library;
    struct ExecBase	*sysbase;
    BPTR		seglist;	

    struct Library	*utilitybase;
    struct IntuitionBase *intuitionbase;
    struct IClass	*classptr;

};

typedef struct IntuitionBase IntuiBase;

#undef LB
#define LB(b) ((struct ListBase_intern *)b)
#undef UtilityBase
#define UtilityBase 	LB(AROSListBase)->utilitybase

/* On Linux-M68k SysBase has to be global! */
extern struct ExecBase * SysBase;

/*
#undef SysBase
#define SysBase		LB(AROSListBase)->sysbase
*/

#undef IntuitionBase
#define IntuitionBase	LB(AROSListBase)->intuitionbase

#define expunge() \
AROS_LC0(BPTR, expunge, struct ListBase_intern *, AROSListBase, 3, AROSList)

#endif /* AROSLIST_INTERN_H */

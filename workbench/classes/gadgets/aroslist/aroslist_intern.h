/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSLIST_INTERN_H
#define AROSLIST_INTERN_H



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

struct ListEntry **AllocEntries(ULONG, struct ListData *);
ULONG InsertItems(APTR *, struct ListEntry **, LONG, struct ListData *);
ULONG CountItems(APTR *);

#endif /* AROSLIST_INTERN_H */

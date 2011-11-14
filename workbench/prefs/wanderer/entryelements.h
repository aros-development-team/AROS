/*
    Copyright  2004-2007, The AROS Development Team. All rights reserved.
    This file is part of the Wanderer Preferences program, which is distributed
    under the terms of version 2 of the GNU General Public License.

    $Id$
*/

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>

struct EntryElement__Entry
{
    struct      Node    EE_E_Node;
    IPTR                EE_E_ID;
    char                *EE_E_Name;
};

#if !defined(HAVE_ELEMENTFUNCS)
extern BOOL EntryElementRegister(struct List *entry_List, ULONG entry_ID, CONST_STRPTR entry_Name);
extern ULONG EntryElementCount(struct List *entry_List);
extern void EntryElementRemove(struct List *entry_List, ULONG entry_ID);
extern IPTR EntryElementFindNode(struct List *entry_List, ULONG entry_ID);
extern IPTR EntryElementFindNamedNode(struct List *entry_List, char * entry_Name);
extern IPTR GetEntryElementName(IPTR entry);
extern IPTR GetEntryElementID(IPTR entry);
#endif

/*
    Copyright  2004-2019, The AROS Development Team. All rights reserved.
    This file is part of the Wanderer Preferences program, which is distributed
    under the terms of version 2 of the GNU General Public License.

    $Id$
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>

#define HAVE_ELEMENTFUNCS
#include "entryelements.h"

ULONG EntryElementCount(struct List *entry_List)
{
    ULONG count = 0;
    struct EntryElement__Entry    *current_Node = NULL;

    ForeachNode(entry_List, current_Node)
    {
        count += 1;
    }
    return count;
}

BOOL EntryElementRegister(struct List *entry_List, ULONG entry_ID, CONST_STRPTR entry_Name)
{
    struct EntryElement__Entry    *current_Node = NULL;
    IPTR                        element_NameLen = strlen(entry_Name) ;

    //If the element already exists fail ..
    ForeachNode(entry_List, current_Node)
    {
        if ((current_Node->EE_E_ID == (IPTR)entry_ID) ||
            (strcmp(current_Node->EE_E_Name, entry_Name) == 0))
            return FALSE;
    }
    
    current_Node = AllocVec(sizeof(struct EntryElement__Entry), MEMF_PUBLIC|MEMF_CLEAR);
    current_Node->EE_E_Name = AllocVec(element_NameLen + 1, MEMF_PUBLIC|MEMF_CLEAR);
    strcpy(current_Node->EE_E_Name, entry_Name);

    current_Node->EE_E_ID = (IPTR)entry_ID;
    AddTail(entry_List, (struct Node *)current_Node);

    return TRUE;
}

void EntryElementRemove(struct List *entry_List, ULONG entry_ID)
{
    struct EntryElement__Entry    *current_Node = NULL;

    ForeachNode(entry_List, current_Node)
    {
        if (current_Node->EE_E_ID == (IPTR)entry_ID)
        {
/* TODO: remove the elements node and free its storage */
        }
    }
}

IPTR EntryElementFindNode(struct List *entry_List, ULONG entry_ID)
{
    struct EntryElement__Entry    *current_Node = NULL;

    ForeachNode(entry_List, current_Node)
    {
        if (current_Node->EE_E_ID == (IPTR)entry_ID)
            return (IPTR)current_Node;
    }
    return 0;
}

IPTR EntryElementFindNamedNode(struct List *entry_List, char * entry_Name)
{
    struct EntryElement__Entry    *current_Node = NULL;

    ForeachNode(entry_List, current_Node)
    {
        if (strcmp(current_Node->EE_E_Name, entry_Name) == 0)
            return (IPTR)current_Node;
    }
    return 0;
}

IPTR GetEntryElementName(IPTR entry)
{
    return (IPTR)(((struct EntryElement__Entry *)entry)->EE_E_Name);
}

IPTR GetEntryElementID(IPTR  entry)
{
    return ((struct EntryElement__Entry *)entry)->EE_E_ID;
}

//
// pmdlist.c
//
// PopupMenu Library - Linked Lists
//
// Copyright (C)2000 Henrik Isaksson <henrik@boing.nu>
// All Rights Reserved.
//

#include "pmdlist.h"

#include <exec/memory.h>

#include <proto/exec.h>
#include <clib/alib_protos.h>

//
// PM_InitList - allocate and initialize an Exec MinList structure.
//
PMDList *PM_InitList(void)
{
    PMDList *newlist=NULL;

    newlist=(PMDList *)AllocVec(sizeof(struct MinList), MEMF_ANY);
    if(newlist) {
        NewList((struct List *)newlist);
    }

    return newlist;
}

//
// PM_FreeList - free all nodes in a list and the list structure itself.
//
void PM_FreeList(PMDList *list)
{
    PMGLN *worknode;
    PMGLN *nextnode;

    worknode = (PMGLN *)(list->mlh_Head); /* First node */
    while((nextnode = (PMGLN *)(worknode->n.mln_Succ))) {
        PM_FreeNode((PMNode *)worknode);
        worknode = nextnode;
    }

    FreeVec(list);
}

//
// PM_CopyList - copy a list.
//
PMDList *PM_CopyList(PMDList *list)
{
    PMDList *newlist=NULL;
    PMGLN *worknode;
    PMGLN *nextnode;

    newlist=PM_InitList();
    if(newlist) {
        if(list) {
            worknode = (PMGLN *)(list->mlh_Head); /* First node */
            while((nextnode = (PMGLN *)(worknode->n.mln_Succ))) {
                PMGLN *copy=(PMGLN *)PM_CopyNode((PMNode *)worknode);
                if(copy) PM_AddToList(newlist, (PMNode *)copy);
                worknode = nextnode;
            }
        }
    }

    return newlist;
}

//
// PM_AddToList - add A to list l.
//
void PM_AddToList(PMDList *l, PMNode *A)
{
    AddHead((struct List *)l, (struct Node *)A);
}

//
// PM_Unlink - remove A from list l.
//
void PM_Unlink(PMDList *l, PMNode *A)
{
    Remove((struct Node *)A);
}

//
// PM_FreeNode - free a PMNode.
//
void PM_FreeNode(PMNode *A)
{
    FreeVec(A);
}

//
// PM_CopyNode - copy a PMNode.
//
PMNode *PM_CopyNode(PMNode *A)
{
    PMNode *newnode=NULL;

    if(A) {
        newnode=(PMNode *)AllocVec(((PMGLN *)A)->Length, MEMF_ANY);
        if(newnode) {
            CopyMem(A, newnode, ((PMGLN *)A)->Length);
        }
    }

    return newnode;
}

/*
     Copyright 2010, The AROS Development Team. All rights reserved.
     $Id$
 */

/*
 * audio.device
 *
 * by Nexus Development 2003
 * coded by Emanuele Cesaroni
 *
 * $Id: ObjPool_lite_1.c,v 1.4 2003/10/30 10:57:41 henes Exp $
 */

#include	"audio_intern.h"

/*
 *
 *	enode_AllocNode
 * -----------------
 *
 *	Allocs an ENODE node. The node returned has size equal to sizeof(ENODE). Is possible also to alloc more bytes after the struct
 *	enode equal to the nodesize for personal uses. If it has to be simply as the ENODE you can ask as nodesize ENODE_NOBODY. Is also
 *	possible to name this node at making using the field name. The name can be changed after or can be ENODE_NONAME.
 *	Returns the node or NULL if there is not memory. If you ask as size for example 10 bytes the whole struct size is sizeof(ENODE) + 10.
 *
 */

ENODE *enode_AllocNode(unsigned long int nodesize, unsigned long int name)
{
    ENODE *thenode;

    if (nodesize == ENODE_NOBODY)
        nodesize = sizeof(ENODE);
    else
        nodesize += sizeof(ENODE);
    if ((thenode = (ENODE*) AllocMem(nodesize, MEM_TYPE)))
    {
        thenode->prec = NULL;
        thenode->next = NULL; // if i add this the whole guru
        thenode->size = nodesize;
        thenode->name = name;
    }
    return (thenode);
}

/*
 *
 *	enode_FreeNode
 * ----------------
 *
 *	Frees an enode allocated by enode_AllocNode(). Pay attention because the node is not removed from the list it may still be.
 *	So before free a simple enode remove it from the list, or better allocs all the nodes you want but always asking for
 *	ELIST_FREE elist types which free all the nodes they have in their list automatically. The best would be to ask for that type
 *	of list (ELIST_FREE) adding a new node each time to an existing list or making more than one list; one could manage a stack of
 *	available nodes and another one when needs of a node could pick the one from the stack and after having used it could send
 *	it back to the stack list.
 *
 */

void enode_FreeNode(ENODE *thenode)
{
    if (thenode)
        FreeMem(thenode, thenode->size);
}

/*
 *	enode_InitList
 * ----------------
 *
 *	Inits a just allocated list.
 *
 */

VOID enode_InitList(ELIST *thelist)
{
    thelist->firstnode = &thelist->first_node;
    thelist->lastnode = &thelist->last_node;
    thelist->firstnode->prec = NULL; // 0->F
    thelist->firstnode->next = thelist->lastnode; // 0->F->L
    thelist->lastnode->prec = thelist->firstnode; // 0->F<->L
    thelist->lastnode->next = NULL; // 0->F<->L->0
    thelist->firstnode->name = ELIST_SIMPLE; // The action to perform when freed by enode_FreeList().
}

/*
 *
 *	enode_AllocList
 * -----------------
 *
 *	Allocs the functional body of an enode list of nodes. Return a ready ELIST or NULL if there is not memory.
 *	You can ask a particular quitting mode when this list is freed by enode_FreeList(). By ELIST_FREE when the
 *	enode_FreeList() is called the function frees by enode_FreeNode() all the nodes into the elist. By ELIST_SIMPLE
 *	the function does nothing and you have to free manually all the enodes. The function saves the type into the name of the
 *	head node (elist->firstnode->name).
 *	The input value 'toalloc' is usefull to ask enode_AllocList() to allocate by enode_AllocNode() a certain number of pre
 *	allocated nodes own by this list as default. If you want an empty list please ask for a ELIST_EMPTY otherwise for the
 *	number of enodes. The enodes allocated here are with noname and has the minimal size possible equal to sizeof(ENODE).
 *	Remember to free manually all the enodes here allocated (using enode_FreeNode()) if you don't ask the ELIST_FREE mode
 *	here. If there isn't enought mem to alloc all the requested enodes the function frees all the resources and returns NULL.
 *
 *	>Syn
 *	*list	=	enode_AllocList(nodes,exit_mode)
 *
 *	>Inputs
 *	nodes			=	Number of nodes to alloc at startup, if none use ELIST_EMPTY.
 *	exit_mode	=	If ELIST_SIMPLE no action is taken at freeing otherwise by ELIST_FREE each node in the list is freed.
 *
 */

ELIST *enode_AllocList(unsigned long int toalloc, unsigned long int exitmode)
{
    ELIST *thelist;
    ENODE *thenode;

    if ((thelist = (ELIST*) AllocMem(sizeof(ELIST), MEM_TYPE)))
    {
        thelist->firstnode = &thelist->first_node;
        thelist->lastnode = &thelist->last_node;
        thelist->firstnode->prec = NULL; // 0->F
        thelist->firstnode->next = thelist->lastnode; // 0->F->L
        thelist->lastnode->prec = thelist->firstnode; // 0->F<->L
        thelist->lastnode->next = NULL; // 0->F<->L->0
        thelist->firstnode->name = exitmode; // The action to perform when freed by enode_FreeList().
        if (toalloc) // Have i to pre-alloc some enodes as default for this list?????
        {
            do
            {
                if ((thenode = enode_AllocNode(ENODE_NOBODY, ENODE_NONAME)))
                    enode_AddHead(thelist, thenode);
                else
                {
                    thelist->firstnode->name = ELIST_FREE; // Force the free mode so enode_FreeList() will free all the list's nodes.
                    enode_FreeList(thelist);
                    return (NULL);
                }
            } while (--toalloc);
        }
    }
    return (thelist);
}

/*
 *
 *	enode_FreeList
 * ----------------
 *
 *	Frees the head body ELIST of a list of enodes. It considers the initial exitmode asked in enode_AllocList(). The value is
 *	saved under the elist->firstnode->name field. If it is ELIST_FREE before to free the entire body list frees by
 *	enode_FreeNode() all the nodes in the list. By ELIST_SIMPLE it does nothing more than to free the list's body ELIST.
 *
 */

void enode_FreeList(ELIST *thelist)
{
    ENODE *thenode;

    if (thelist)
    {
        if (thelist->firstnode->name == ELIST_FREE)
        {
            while ((thenode = enode_RemHead(thelist)))
                enode_FreeNode(thenode);
        }
        FreeMem(thelist, sizeof(ELIST));
    }
}

/*
 *
 *	enode_PrintList
 * -----------------
 *
 *	Prints an elist info.
 *
 */

void enode_PrintList(ELIST *thelist)
{
    /*
     ENODE	*actnode;

     actnode = thelist->firstnode;
     do
     {
     if(actnode == thelist->firstnode)		printf("List is: [F](%d)",thelist);
     else	if(actnode == thelist->lastnode)	printf("-[L](%d)\n",thelist->lastnode);
     else												printf("-[%d](%d){%d}",actnode->name,actnode,actnode->pri);
     }while(actnode = actnode->next);
     */
}

/*
 *
 *	enode_AddHead
 * ---------------
 *
 *	Add an enode to the head of an elist.
 *
 *
 */

void enode_AddHead(ELIST *thelist, ENODE *thenode)
{
    thenode->prec = thelist->firstnode; // F<-N
    thenode->next = thelist->firstnode->next; // F<-N->L
    thelist->firstnode->next->prec = thenode; // F<-N<->L
    thelist->firstnode->next = thenode; // F<->N<->L
}

/*
 *
 *	enode_RemHead
 * ---------------
 *
 *	Removes the head enode from an elist. Returns the pointer to the enode or NULL if the list is empty.
 *
 *
 */

ENODE *enode_RemHead(ELIST *thelist)
{
    ENODE *thenode;

    if ((thenode = thelist->firstnode->next) != thelist->lastnode)
    {
        thenode->prec->next = thenode->next;
        thenode->next->prec = thenode->prec;
        return (thenode);
    }
    return (NULL);
}

/*
 *
 *	enode_AddTail
 * ---------------
 *
 *	Add an enode to the tail of an elist.
 *
 *
 */

void enode_AddTail(ELIST *thelist, ENODE *thenode)
{
    thenode->prec = thelist->lastnode->prec; // F<-N
    thenode->next = thelist->lastnode; // F<-N->L
    thelist->lastnode->prec->next = thenode; // F<->N<-L
    thelist->lastnode->prec = thenode; // F<-N<->L
}

/*
 *
 *	enode_RemTail
 * ---------------
 *
 *	Removes the tail enode from an elist. Returns the pointer to the enode or NULL if the list is empty.
 *
 *
 */

ENODE *enode_RemTail(ELIST *thelist)
{
    ENODE *thenode;

    if ((thenode = thelist->lastnode->prec) != thelist->firstnode)
    {
        thenode->prec->next = thenode->next;
        thenode->next->prec = thenode->prec;
        return (thenode);
    }
    return (NULL);
}

/*
 *
 *	enode_Remove
 * --------------
 *
 *	Removes an enode from the elist where it is.
 *
 */

void enode_Remove(ENODE *thenode)
{
    thenode->prec->next = thenode->next;
    thenode->next->prec = thenode->prec;
}

/*
 *
 *	enode_FindNode
 * ----------------
 *
 *	Finds in an elist the first enode from the bottom (head) of the list which has the field enode->name equal
 *	to the requested name. If doesn't find any return NULL. The name is a 32 bit integer value.
 *
 */

ENODE *enode_FindNode(ELIST *thelist, unsigned long int name)
{
    ENODE *founded;

    founded = thelist->firstnode;
    while ((founded = founded->next) != thelist->lastnode)
    {
        if (founded->name == name)
            return (founded);
    }
    return (NULL);
}

/*
 *
 *	enode_Insert
 * --------------
 *
 *	Puts the enode 'insert' in the same list 'thelist' of enode 'before' after the 'before' enode. If 'before' is NULL
 *	the enode 'insert' is added at the head of the elist 'thelist'. In that case is the same as calling enode_AddHead(thelist,insert).
 *
 */

void enode_Insert(ELIST *thelist, ENODE *insert, ENODE *before)
{
    if (before == NULL)
        enode_AddHead(thelist, insert);
    else
    {
        insert->prec = before;
        insert->next = before->next;
        before->next->prec = insert;
        before->next = insert;
    }
}

/*
 *
 *	enode_SwapNodes
 * -----------------
 *
 *	Swaps two nodes.
 *
 */

VOID enode_SwapNodes(ENODE *node_a, ENODE *node_b)
{
    ENODE *a_prec, *a_next, *b_prec;

    a_prec = node_a->prec;
    a_next = node_a->next;
    b_prec = node_b->prec;

    if (node_a->next == node_b)
    {
        node_a->next = node_b->next;
        node_a->prec = node_b;
        if (node_b->next)
            node_b->next->prec = node_a;
        node_b->next = node_a;
        node_b->prec = a_prec;
        if (a_prec)
            a_prec->next = node_b;
    }
    else if (node_b->next == node_a)
    {
        node_b->next = node_a->next;
        node_b->prec = node_a;
        if (node_a->next)
            node_a->next->prec = node_b;
        node_a->next = node_b;
        node_a->prec = b_prec;
        if (b_prec)
            b_prec->next = node_a;
    }
    else
    {
        node_a->next = node_b->next;
        node_a->prec = node_b->prec;
        if (node_b->next)
            node_b->next->prec = node_a;
        if (node_b->prec)
            node_b->prec->next = node_a;
        node_b->next = a_next;
        node_b->prec = a_prec;
        if (a_next)
            a_next->prec = node_b;
        if (a_prec)
            a_prec->next = node_b;
    }
}

/*
 *
 *	enode_FindListNode
 * --------------------
 *
 *	Given an enode finds the elist which contains that enode. Could be a bit expansive in time.
 *
 *
 */

ELIST *enode_FindListNode(ENODE *thenode)
{
    while (thenode->prec != NULL)
        thenode = thenode->prec;
    return ((ELIST*) thenode); //the first enode is has the same struct address of the elist.
}

/*
 *
 *	enode_Enqueue
 * ---------------
 *
 *	Appends an enode into an elist considering the node pri. You are allowed to modify the enode->pri field before using this
 *	function. The pri is a signed char so can have a range from -127 to +127. More high it is more pri the node has when added
 *	into the list. The highest pri is +127 the lowest is -127. The order is..
 *	list_head->127->126->125->....->list_tail
 *	If a new node has the same pri of an existing one the last is inserted with a FIFO (first in, first out) order so is placed
 *	in the last position before the next node with lower pri and after all the other with same pri.
 *
 */

void enode_Enqueue(ELIST *thelist, ENODE *thenode)
{
    ENODE *actnode;

    actnode = thelist->firstnode;
    while ((actnode = actnode->next) != thelist->lastnode)
    {
        if (thenode->pri > actnode->pri)
        {
            enode_Insert(thelist, thenode, actnode->prec);
            return;
        }
    }
    enode_AddTail(thelist, thenode);
}

/*
 *	enode_GetHeadNode
 *  -------------------
 *
 *	Returns the pointer to the head enode of the elist without removing it. If the elist is empty returns NULL.
 *
 */

ENODE *enode_GetHeadNode(ELIST *thelist)
{
    if (thelist->firstnode->next != thelist->lastnode)
        return (thelist->firstnode->next);
    else
        return (NULL);
}

/*
 *	enode_GetTailNode
 *  -------------------
 *
 *	Returns the pointer to the tail enode of the elist without removing it. If the elist is empty returns NULL.
 *
 */

ENODE *enode_GetTailNode(ELIST *thelist)
{
    if (thelist->lastnode->prec != thelist->firstnode)
        return (thelist->lastnode->prec);
    else
        return (NULL);
}

/*
 *
 *	enode_GetNextNode
 *  -------------------
 *
 *	Returns,without removing, the next node after the given 'thenode' or NULL if 'thenode' is the last one of the list.
 *
 */

ENODE *enode_GetNextNode(ELIST *thelist, ENODE *thenode)
{
    if (thenode->next != thelist->lastnode)
        return (thenode->next);
    else
        return (NULL);

}

/*
 *
 *	enode_GetPrecNode
 *  -------------------
 *
 *	Returns, without removing, the previous enode after the given 'thenode' or NULL if 'thenode' is the first one of the list.
 *
 */

ENODE *enode_GetPrecNode(ELIST *thelist, ENODE *thenode)
{
    if (thenode->prec != thelist->firstnode)
        return (thenode->prec);
    else
        return (NULL);

}

/*
 *
 *	enode_GetNodeName
 *  -------------------
 *
 *	Returns the name of an enode.
 *
 */

unsigned long int enode_GetNodeName(ENODE *thenode)
{
    return (thenode->name);
}

/*
 *
 *	enode_SetNodeName
 *  -------------------
 *
 *	Changes te node's own name.
 *
 */

VOID enode_SetNodeName(ENODE *thenode, unsigned long int name)
{
    thenode->name = name;
}

/*
 *
 *	enode_CountNodes
 * ------------------
 *
 *	Returns the number (unsigned long int) of nodes present into an ELIST.
 *
 *
 */

unsigned long int enode_CountNodes(ELIST *thelist)
{
    unsigned long int count = 0;
    ENODE *actnode;

    actnode = thelist->firstnode;
    while ((actnode = actnode->next) != thelist->lastnode)
    {
        count++;
    }
    return (count);
}

/*
 *
 *	enode_GetNNode
 * ----------------
 *
 *	Returns the node in n position from the bottom. If the pos value exits from the list returns NULL. The same if the list
 *	is empty. The first node has position 0.
 *
 *
 *	>Syn
 *	*nnode	=	enode_GetNNode(*list,pos)
 *
 */

ENODE *enode_GetNNode(ELIST *thelist, signed long int pos)
{
    ENODE *actnode;

    if ((actnode = enode_GetHeadNode(thelist)))
    {
        while ((pos-- != 0) && (actnode))
            actnode = enode_GetNextNode(thelist, actnode);
    }
    return (actnode);
}


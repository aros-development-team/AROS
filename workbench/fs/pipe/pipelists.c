/****************************************************************************
**  File:       pipelists.c
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
*/

#include   <exec/types.h>

#include   "pipelists.h"



/*---------------------------------------------------------------------------
** pipelists.c
** -----------
** This module contains functions and macros for list manipulation.
** To use its functions, a PIPELISTNODE must be part of the structure to be
** inserted in a list.  A list is identified by a PIPELISTHEADER (not a
** pointer to, but an actual PIPELISTHEADER structure).
**      These routines, as implemented, use the fact that a PIPELISTHEADER
** and a PIPELISTNODE have the same struture.  Loops are started with the
** scanning pointer referencing the header.  This makes processing uniform,
** even when  the list is empty.
**
** Visible Functions
** -----------------
**	void  InsertHead (headerp, nodep)
**	void  InsertTail (headerp, nodep)
**	void  Delete     (headerp, nodep)
**
** Macros (in pipelists.h)
** -----------------------
**	InitList  (headerp)
**	FirstItem (headerp)
**	NextItem  (nodep)
**
** Local Functions
** ---------------
**	- none -
*/



/*---------------------------------------------------------------------------
** Insert the node pointed to by "nodep" at the head (front) of the list
** identified by "headerp".
*/

void  InsertHead (headerp, nodep)

PIPELISTHEADER  *headerp;
PIPELISTNODE    *nodep;

{ nodep->next= headerp->head;
  headerp->head= nodep;
}



/*---------------------------------------------------------------------------
** Insert the node pointed to by "nodep" at the tail (end) of the list
** identified by "headerp".
*/

void  InsertTail (headerp, nodep)

PIPELISTHEADER  *headerp;
PIPELISTNODE    *nodep;

{ register PIPELISTNODE  *l;


  for (l= (PIPELISTNODE *) headerp; l->next != NULL; l= l->next)
    ;

  l->next= nodep;
  nodep->next= NULL;
}



/*---------------------------------------------------------------------------
** Delete the node pointed to by "nodep" from the list identified by
** "headerp".  If the node is not found in the list, nothing is done.
*/

void  Delete (headerp, nodep)

PIPELISTHEADER  *headerp;
PIPELISTNODE    *nodep;

{ PIPELISTNODE  *l;


  for (l= (PIPELISTNODE *) headerp; l->next != NULL; l= l->next)
    if (l->next == nodep)
      { l->next= l->next->next;
        break;
      }
}

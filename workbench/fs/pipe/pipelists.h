/****************************************************************************
**  File:       pipelists.h
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
*/



typedef struct pipelistnode       /* must be first member of list items */
  { struct pipelistnode  *next;
  }
PIPELISTNODE;

typedef struct pipelistheader
  { struct pipelistnode  *head;
  }
PIPELISTHEADER;



#define   InitList(headerp)    ((void) ((headerp)->head= NULL))
#define   FirstItem(headerp)   ((headerp)->head)
#define   NextItem(nodep)      (((PIPELISTNODE *) (nodep))->next)



extern void  InsertHead ( /* headerp, nodep */ );
extern void  InsertTail ( /* headerp, nodep */ );
extern void  Delete     ( /* headerp, nodep */ );

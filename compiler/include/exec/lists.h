#ifndef EXEC_LISTS_H
#define EXEC_LISTS_H
/* (C) 1995 AROS - The Amiga Replacement OS */

/******************************************************************************

    FILE
	$Id$

    DESCRIPTION
	Prototypes and macros for exec-lists.

******************************************************************************/

/**************************************
		Includes
**************************************/
#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif


/**************************************
	       Structures
**************************************/
/* Normal list */
struct List
{
    struct Node * lh_Head,
		* lh_Tail,
		* lh_TailPred;
    UBYTE	  lh_Type;
    BYTE	  l_pad;
};

/* Minimal list */
struct MinList
{
    struct MinNode * mlh_Head,
		   * mlh_Tail,
		   * mlh_TailPred;
};


/**************************************
	       Makros
**************************************/
#define IsListEmpty(l) \
	( (((struct List *)l)->lh_TailPred) == (struct Node *)(l) )

#define IsMsgPortEmpty(mp) \
      ( (((struct MsgPort *)mp)->mp_MsgList.lh_TailPred) \
	    == (struct Node *)(&(((struct MsgPort *)mp)->mp_MsgList)) )

#ifdef AROS_ALMOST_COMPATIBLE
#   define NEWLIST(l)       (((struct List *)l)->lh_TailPred \
				= (struct Node *)(l), \
			    ((struct List *)l)->lh_Tail = 0, \
			    ((struct List *)l)->lh_Head \
				= (struct Node *)\
				    &(((struct List *)l)->lh_Tail))

#   define GetHead(l)       (void *)(((struct List *)l)->lh_Head->ln_Succ \
				? ((struct List *)l)->lh_Head \
				: (struct Node *)0)
#   define GetTail(l)       (void *)(((struct List *)l)->lh_TailPred->ln_Pred \
				? ((struct List *)l)->lh_TailPred \
				: (struct Node *)0)
#   define GetSucc(n)       (void *)(((struct Node *)n)->ln_Succ->ln_Succ \
				? ((struct Node *)n)->ln_Succ \
				: (struct Node *)0)
#   define GetPred(n)       (void *)(((struct Node *)n)->ln_Pred->ln_Pred \
				? ((struct Node *)n)->ln_Pred \
				: (struct Node *)0)
#endif


/******************************************************************************
*****  ENDE exec/lists.h
******************************************************************************/

#endif /* EXEC_LISTS_H */

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_NODES_H
#define EXEC_NODES_H

/******************************************************************************

    MODUL
	$Id$

    DESCRIPTION
	Header-file for nodes.

******************************************************************************/

/**************************************
		Includes
**************************************/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif


/**************************************
	       Structures
**************************************/

struct Node __mayalias;
struct Node
{
    struct Node * ln_Succ,
		* ln_Pred;
    UBYTE	  ln_Type;
    BYTE	  ln_Pri;
    char	* ln_Name;
};

struct MinNode __mayalias;
struct MinNode
{
    struct MinNode * mln_Succ,
		   * mln_Pred;
};


/**************************************
		 Defines
**************************************/
/* Values for ln_Type */
#define NT_UNKNOWN	0	/* Unknown node 			    */
#define NT_TASK 	1	/* Exec task				    */
#define NT_INTERRUPT	2	/* Interrupt				    */
#define NT_DEVICE	3	/* Device				    */
#define NT_MSGPORT	4	/* Message-Port 			    */
#define NT_MESSAGE	5	/* Indicates message currently pending	    */
#define NT_FREEMSG	6
#define NT_REPLYMSG	7	/* Message has been replied		    */
#define NT_RESOURCE	8
#define NT_LIBRARY	9
#define NT_MEMORY	10
#define NT_SOFTINT	11	/* Internal flag used by SoftInits	    */
#define NT_FONT 	12
#define NT_PROCESS	13	/* AmigaDOS Process			    */
#define NT_SEMAPHORE	14
#define NT_SIGNALSEM	15	/* signal semaphores			    */
#define NT_BOOTNODE	16
#define NT_KICKMEM	17
#define NT_GRAPHICS	18
#define NT_DEATHMESSAGE 19
#define NT_HIDD		20	/* AROS specific			    */

#define NT_USER 	254	/* User node types work down from here	    */
#define NT_EXTENDED	255

/***************************************
    Macros
****************************************/

#define SetNodeName(node,name)   \
    (((struct Node *)(node))->ln_Name = (char *)(name))
#define GetNodeName(node)        \
    (((struct Node *)(node))->ln_Name)

/******************************************************************************
*****  ENDE exec/nodes.h
******************************************************************************/

#endif /* EXEC_NODES_H */

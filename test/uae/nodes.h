#ifndef EXEC_NODES_H
#define EXEC_NODES_H
/* Copyright © 1995, The AROS Development Team. All rights reserved. */

/******************************************************************************

    MODUL
	$Id$

    DESCRIPTION
	Header-file for nodes.

******************************************************************************/

/**************************************
		Includes
**************************************/
/* #ifndef AROS_CONFIG_H
#   include <aros/config.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif */
#include "types.h"

class NodePtr : APTR
{
public:
    /* Here I overload the -> (dereference) operator. When you dereferenece
       a NodePtr, then you get a struct Node *. Here is how it works:

	    NodePtr nptr;

	    ... give nptr a valid value ...

	    name = nptr->ln_Name;

	The C++ compiler creates this code for the last line:

	    struct Node * temp = nptr.operator-> ();
	    name = temp->ln_Name;

	This means that you can use any field name which is valid after
	"struct Node *" after an NodePtr.
    */
    inline struct Node * operator -> ()
    {
	return (struct Node *) ntohl (data);
    }

    /* Convert a NodePtr to a Node */
    inline operator struct Node * ()
    {
	return (struct Node *) ntohl (data);
    }

    /* Convert it to void * */
    inline operator void * ()
    {
	return (struct Node *) ntohl (data);
    }

    /* Create a NodePtr from a struct Node pointer. */
    inline NodePtr (struct Node * v)
    {
	data = htonl ((long)v);
    }

    inline NodePtr ()
    {
	return;
    }
};

/* Pretty much the same but this time for MinNodePtr */
class MinNodePtr : APTR
{
public:
    inline struct MinNode * operator -> ()
    {
	return (struct MinNode *) ntohl (data);
    }

    inline MinNodePtr (struct MinNode * v)
    {
	data = htonl ((long)v);
    }

    inline MinNodePtr ()
    {
	return;
    }
};

/**************************************
	       Structures
**************************************/

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
struct Node
{
    NodePtr ln_Succ,
	    ln_Pred;
    UBYTE   ln_Type;
    BYTE    ln_Pri;
    /* AROS: pointer should be 32bit aligned, but we can not do this on
       the native machine because of binary compatibility.
    */
    STRPTR  ln_Name;

public:
    inline Node () { return; }
};

#else
struct Node
{
    NodePtr ln_Succ,
	    ln_Pred;
    /* AROS: pointer should be 32bit aligned */
    STRPTR  ln_Name;
    UBYTE   ln_Type;
    BYTE    ln_Pri;

public:
    inline Node () { return; }
};
#endif /* AROS_FLAVOUR */

struct MinNode
{
    MinNodePtr mln_Succ,
	       mln_Pred;

public:
    inline MinNode () { return; }
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

#define NT_USER 	254	/* User node types work down from here	    */
#define NT_EXTENDED	255


/******************************************************************************
*****  ENDE exec/nodes.h
******************************************************************************/

#endif /* EXEC_NODES_H */

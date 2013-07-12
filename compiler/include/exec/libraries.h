#ifndef EXEC_LIBRARIES_H
#define EXEC_LIBRARIES_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amiga header file exec/libraries.h
    Lang: english
*/

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

/* Library constants */
/* LIB_VECTSIZE is in aros/machine.h */
#define LIB_RESERVED	4	/* Exec reserves the first 4 vectors */
#define LIB_BASE	(-LIB_VECTSIZE)
#define LIB_USERDEF	(LIB_BASE-(LIB_RESERVED*LIB_VECTSIZE))
#define LIB_NONSTD	(LIB_USERDEF)

/* Standard vectors */
#define LIB_OPEN	(LIB_BASE*1)
#define LIB_CLOSE	(LIB_BASE*2)
#define LIB_EXPUNGE	(LIB_BASE*3)
#define LIB_EXTFUNC	(LIB_BASE*4)  /* for future expansion */


/* Library structure. Also used by Devices and some Resources. */
struct Library {
    struct  Node lib_Node;
    UBYTE   lib_Flags;
    UBYTE   lib_pad;
    UWORD   lib_NegSize;	    /* number of bytes before library */
    UWORD   lib_PosSize;	    /* number of bytes after library */
    UWORD   lib_Version;	    /* major */
    UWORD   lib_Revision;	    /* minor */
#ifdef AROS_NEED_LONG_ALIGN
    UWORD   lib_pad1;		    /* make sure it is longword aligned */
#endif
    APTR    lib_IdString;	    /* ASCII identification */
    ULONG   lib_Sum;		    /* the checksum */
    UWORD   lib_OpenCnt;	    /* how many people use us right now? */
#ifdef AROS_NEED_LONG_ALIGN
    UWORD   lib_pad2;		    /* make sure it is longword aligned */
#endif
};

/* lib_Flags bits (all others are reserved by the system) */
#define LIBF_SUMMING	(1<<0)      /* lib is currently being checksummed */
#define LIBF_CHANGED	(1<<1)      /* lib has changed */
#define LIBF_SUMUSED	(1<<2)      /* sum should be checked */
#define LIBF_DELEXP	(1<<3)      /* delayed expunge */

#ifdef AROS_LIB_OBSOLETE
/* Temporary Compatibility */
#define lh_Node 	lib_Node
#define lh_Flags	lib_Flags
#define lh_pad		lib_pad
#define lh_NegSize	lib_NegSize
#define lh_PosSize	lib_PosSize
#define lh_Version	lib_Version
#define lh_Revision	lib_Revision
#define lh_IdString	lib_IdString
#define lh_Sum		lib_Sum
#define lh_OpenCnt	lib_OpenCnt
#endif

/* ID numbers for Exec/TaggedOpenLibrary
 */
#define TAGGEDOPEN_GRAPHICS     1
#define TAGGEDOPEN_LAYERS       2
#define TAGGEDOPEN_INTUITION    3
#define TAGGEDOPEN_DOS          4
#define TAGGEDOPEN_ICON         5
#define TAGGEDOPEN_EXPANSION    6
#define TAGGEDOPEN_UTILITY      7
#define TAGGEDOPEN_KEYMAP       8
#define TAGGEDOPEN_GADTOOLS     9
#define TAGGEDOPEN_WORKBENCH    10


#endif	/* EXEC_LIBRARIES_H */

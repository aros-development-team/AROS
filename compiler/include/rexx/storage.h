#ifndef REXX_STORAGE_H
#define REXX_STORAGE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ARexx data structures
    Lang: English
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef DOS_DOSEXTENS_H
#   include <dos/dosextens.h>
#endif

#define RXFF_NOIO	(1<<RXFB_NOIO)
#define RXFF_RESULT	(1<<RXFB_RESULT)
#define RXFF_STRING	(1<<RXFB_STRING)
#define RXFF_TOKEN	(1<<RXFB_TOKEN)
#define RXFF_NONRET	(1<<RXFB_NONRET)
#define RXFF_FUNCLIST   (1<<RXFB_FUNCLIST)

struct RexxArg
{
	LONG  ra_Size;
	UWORD ra_Length;
	UBYTE ra_Depricated1; /* Was ra_Flags but not used anymore */
	UBYTE ra_Depricated2; /* Was ra_Hash but not used anymore */
	BYTE  ra_Buff[8];
};

struct RexxRsrc
{
	struct Node rr_Node;
	WORD        rr_Unused1; /* rr_Func */
	APTR        rr_Unused2; /* rr_Base */
	LONG        rr_Size; /* Total size of structure */
	LONG        rr_Args1; /* Meaning depends on type of Resource */
	LONG        rr_Args2; /* Meaning depedns on type of Resource */
};

/* Types for the resource nodes */
#define RRT_ANY   0
#define RRT_LIB   1  /* A function library */
/*#define RRT_PORT  2  Not used */
/*#define RRT_FILE  3  Not used */
#define RRT_HOST  4  /* A function host */
#define RRT_CLIP  5  /* A clip on the clip list */

#endif /* REXX_STORAGE_H */

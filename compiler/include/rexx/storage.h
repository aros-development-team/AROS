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

struct RexxMsg
{
	struct Message	rm_Node;
	IPTR		rm_Private1; /* Was rm_TaskBlock */
	IPTR		rm_Private2; /* Was rm_LibBase */
	LONG		rm_Action; /* What to do ? */
	LONG		rm_Result1; /* The first result as a number */
	IPTR		rm_Result2; /* The second result, most of the time an argstring */
	IPTR		rm_Args[16]; /* 16 possible arguments for function calls */
	struct MsgPort *rm_PassPort;
	STRPTR		rm_CommAddr; /* The starting host environment */
	STRPTR		rm_FileExt; /* The file extension for macro files */
	struct FileHandle *rm_Stdin; /* Input filehandle to use */
	struct FileHandle *rm_Stdout; /* Output filehandle to use */
	LONG		rm_Unused1; /* Was rm_avail */
};

/* Shortcuts for the arguments */
#define ARG0(msg) (msg->rm_Args[0])
#define ARG1(msg) (msg->rm_Args[1])
#define ARG2(msg) (msg->rm_Args[2])

/* The command for in rm_Action */
#define RXCOMM   0x01000000
#define RXFUNC   0x02000000
#define RXCLOSE  0x03000000
#define RXQUERY  0x04000000
#define RXADDFH  0x07000000
#define RXADDLIB 0x08000000
#define RXREMLIB 0x09000000
#define RXADDCON 0x0A000000
#define RXREMCON 0x0B000000
#define RXTCOPN  0x0C000000
#define RXTCCLS  0x0D000000

#define RXCODEMASK 0xFF000000
#define RXARGMASK  0x0000000F

/* Flags that can be combined with the commands */
#define RXFB_NOIO     16
#define RXFB_RESULT   17
#define RXFB_STRING   18
#define RXFB_TOKEN    19
#define RXFB_NONRET   20
#define RXFB_FUNCLIST 5

/* Convert from bit number to number */
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
	LONG        rr_Args2; /* Meaning depends on type of Resource */
};

/* Types for the resource nodes */
#define RRT_ANY   0
#define RRT_LIB   1  /* A function library */
/*#define RRT_PORT  2  Not used */
/*#define RRT_FILE  3  Not used */
#define RRT_HOST  4  /* A function host */
#define RRT_CLIP  5  /* A clip on the clip list */

#endif /* REXX_STORAGE_H */

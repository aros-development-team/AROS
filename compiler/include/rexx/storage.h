#ifndef REXX_STORAGE_H
#define REXX_STORAGE_H
/*
    (C) 2000 AROS - The Amiga Research OS

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

struct RexxMsg
{
	struct Message	rm_Node;	/* EXEC message structure */
	APTR		rm_TaskBlock;	/* global structure (private) */
	APTR		rm_LibBase;	/* library base (private) */
	LONG		rm_Action;	/* command (action) code */
	LONG		rm_Result1;	/* primary result (return code) */
	LONG		rm_Result2;	/* secondary result */
	STRPTR		rm_Args[16];	/* argument block (ARG0-ARG15) */
	struct MsgPort *rm_PassPort;	/* forwarding port */
	STRPTR		rm_CommAddr;	/* host address (port name) */
	STRPTR		rm_FileExt;	/* file extension */
	LONG		rm_Stdin;	/* input stream (filehandle) */
	LONG		rm_Stdout;	/* output stream (filehandle) */
	LONG		rm_avail;	/* future expansion */
};

/* rm_Action definitions */
#define RXCODEMASK	0xFF000000
#define RXARGMASK	0x0000000F

/* Commands */
#define RXCOMM		0x01000000	/* a command-level invocation */
#define RXFUNC		0x02000000	/* a function call */
#define RXCLOSE		0x03000000	/* close the REXX server */
#define RXQUERY		0x04000000	/* query for information */
#define RXADDFH		0x07000000	/* add a function host */
#define RXADDLIB	0x08000000	/* add a function library */
#define RXREMLIB	0x09000000	/* remove a function library */
#define RXADDCON	0x0A000000	/* add/update a ClipList string */
#define RXREMCON	0x0B000000	/* remove a ClipList string */
#define RXTCOPN		0x0C000000	/* open the trace console */
#define RXTCCLS		0x0D000000	/* close the trace console */

/* Command modifier flag bits */
#define RXFB_NOIO	16		/* suppress I/O inheritance? */
#define RXFB_RESULT	17		/* result string expected? */
#define RXFB_STRING	18		/* program is a "string file"? */
#define RXFB_TOKEN	19		/* tokenize the command line? */
#define RXFB_NONRET	20		/* a "no-return" message? */

/* The flag form of the command modifiers */
#define RXFF_NOIO	(1<<RXFB_NOIO)
#define RXFF_RESULT	(1<<RXFB_RESULT)
#define RXFF_STRING	(1<<RXFB_STRING)
#define RXFF_TOKEN	(1<<RXFB_TOKEN)
#define RXFF_NONRET	(1<<RXFB_NONRET)

#endif /* REXX_STORAGE_H */

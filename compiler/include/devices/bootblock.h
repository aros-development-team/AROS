#ifndef DEVICES_BOOTBLOCK_H
#define DEVICES_BOOTBLOCK_H

/*
    Copyright (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: Floppy BootBlock definition
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct BootBlock
{
    UBYTE   bb_id[4];
    LONG    bb_chksum;
    LONG    bb_dosblock;
};

#define	BOOTSECTS	    	2

#define BBID_DOS	    	{'D', 'O', 'S', '\0'}
#define BBID_KICK	    	{'K', 'I', 'C', 'K'}

#define BBNAME_DOS	    	0x444F5300 /* "DOS0" */
#define BBNAME_KICK	    	0x4B49434B /* "KICK" */

#endif /* DEVICES_BOOTBLOCK_H */

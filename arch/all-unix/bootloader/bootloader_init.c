/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Bootloader information initialisation.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/bootloader.h>
#include <proto/utility.h>

#include <aros/symbolsets.h>
#include <aros/bootloader.h>
#include "bootloader_intern.h"
#include LC_LIBDEFS_FILE

#include <ctype.h>
#include <string.h>

/* These come from exec/init.c */
extern char *Kernel_Args;
extern char *BootLoader_Name;

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR BootLoaderBase)
{
    D(bug("[Bootldr] Init\n"));
    BootLoaderBase->Flags = 0;
    
    NEWLIST(&(BootLoaderBase->Args));

    D(if (BootLoader_Name) bug("[BootLdr] Init: Loadername = %s\n",BootLoader_Name);)
    BootLoaderBase->LdrName = BootLoader_Name;
    if (Kernel_Args) {
	    STRPTR cmd,buff;
	    ULONG temp;
	    struct Node *node;
	    ULONG len = strlen(Kernel_Args);	    

	    D(bug("[BootLdr] Kernel arguments: %s\n", Kernel_Args));
	    /* First make a working copy of the command line */
	    if ((buff = AllocMem(len, MEMF_ANY|MEMF_CLEAR)))
	    {
		strcpy(buff,Kernel_Args);
		/* remove any leading spaces */
		cmd = stpblk(buff);
		while(cmd[0])
		{
		    /* Split the command line */
		    temp = strcspn(cmd," ");
		    cmd[temp++] = 0x00;
		    D(bug("[BootLdr] Init: Argument %s\n",cmd));
		    /* Allocate node and insert into list */
		    node = AllocMem(sizeof(struct Node),MEMF_ANY|MEMF_CLEAR);
		    node->ln_Name = cmd;
		    AddTail(&(BootLoaderBase->Args),node);
		    /* Skip to next part */
		    cmd = stpblk(cmd+temp);
		}
		
		BootLoaderBase->Flags |= BL_FLAGS_CMDLINE;
	    }
	}
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)

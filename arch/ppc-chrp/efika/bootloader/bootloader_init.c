/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Bootloader information initialisation.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/bootloader.h>
#include <proto/utility.h>
#include <proto/kernel.h>

#include <aros/symbolsets.h>
#include <aros/bootloader.h>
#include <aros/kernel.h>
#include "bootloader_intern.h"
#include LC_LIBDEFS_FILE

#include <string.h>

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR BootLoaderBase)
{
    void *KernelBase = OpenResource("kernel.resource");
    struct TagItem *msg = KrnGetBootInfo();
    IPTR tmp;

    BootLoaderBase->Flags = 0;
    
    NEWLIST(&(BootLoaderBase->Args));
    NEWLIST(&(BootLoaderBase->DriveInfo));

    D(bug("[BootLdr] Init. msg=%p\n", msg));
    
    /* Right. Now we extract the data currently placed in 0x1000 by exec */
    if (msg)
    {
        BootLoaderBase->LdrName = "UBootSecondLevelBootloader";
        BootLoaderBase->Flags |= MB_FLAGS_LDRNAME;
        
        tmp = GetTagData(KRN_CmdLine, 0, msg);
        D(bug("[BootLdr] KRN_CmdLine=%p\n", tmp));
	if (tmp)
	{
	    STRPTR cmd,buff;
	    ULONG temp;
	    struct Node *node;
	    
	    D(bug("[BootLdr] CmdLine=\"%s\"\n", (STRPTR)tmp));
	    
	    /* First make a working copy of the command line */
	    if ((buff = AllocMem(200,MEMF_ANY|MEMF_CLEAR)))
	    {
		strncpy(buff, (STRPTR)tmp, 200);
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
		
		BootLoaderBase->Flags |= MB_FLAGS_CMDLINE;
	    }
	}
    }
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)

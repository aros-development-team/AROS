/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot AROS
    Lang: english
*/

#include <string.h>
#include <stdlib.h>

#include <aros/debug.h>
#include <exec/alerts.h>
#include <aros/asmcall.h>
#include <aros/bootloader.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/types.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <libraries/partition.h>
#include <utility/tagitem.h>
#include <devices/bootblock.h>
#include <devices/timer.h>
#include <dos/dosextens.h>
#include <resources/filesysres.h>

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/partition.h>
#include <proto/bootloader.h>

#include LC_LIBDEFS_FILE

#include "dosboot_intern.h"
#include "menu.h"

/* Delay just like Dos/Delay(), ticks are
 * in 1/50th of a second.
 */
static void bootDelay(ULONG timeout)
{
    struct timerequest  timerio;
    struct MsgPort 	timermp;
    
    memset(&timermp, 0, sizeof(timermp));
    
    timermp.mp_Node.ln_Type = NT_MSGPORT;
    timermp.mp_Flags 	    = PA_SIGNAL;
    timermp.mp_SigBit	    = SIGB_SINGLE;
    timermp.mp_SigTask	    = FindTask(NULL);    
    NEWLIST(&timermp.mp_MsgList);
  
    timerio.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    timerio.tr_node.io_Message.mn_ReplyPort    = &timermp;
    timerio.tr_node.io_Message.mn_Length       = sizeof(timermp);

    if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)&timerio, 0) != 0) {
        D(bug("dosboot: Can't open timer.device unit 0\n"));
        return;
    }

    timerio.tr_node.io_Command 		       = TR_ADDREQUEST;
    timerio.tr_time.tv_secs                    = timeout / TICKS_PER_SECOND;
    timerio.tr_time.tv_micro  		       = 1000000UL / TICKS_PER_SECOND * (timeout % TICKS_PER_SECOND);

    SetSignal(0, SIGF_SINGLE);
	
    DoIO(&timerio.tr_node);

    CloseDevice((struct IORequest *)&timerio);
}

int dosboot_Init(LIBBASETYPEPTR LIBBASE)
{
    struct ExpansionBase *ExpansionBase;
    void *BootLoaderBase;
    BOOL WantBootMenu = FALSE;
    ULONG t;

    LIBBASE->delayTicks = 50;

    D(bug("dosboot_Init: GO GO GO!\n"));

    ExpansionBase = (APTR)OpenLibrary("expansion.library", 0);

    D(bug("[Strap] ExpansionBase 0x%p\n", ExpansionBase));
    if( ExpansionBase == NULL )
    {
        D(bug( "Could not open expansion.library, something's wrong!\n"));
        Alert(AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_ExpansionLib);
    }

    ExpansionBase->Flags |= EBF_SILENTSTART;

    LIBBASE->bm_ExpansionBase = ExpansionBase;

    /*
     * Search the kernel parameters for the bootdelay=%d string. It determines the
     * delay in seconds.
     */
    if ((BootLoaderBase = OpenResource("bootloader.resource")) != NULL)
    {
    	struct List *args = GetBootInfo(BL_Args);

    	if (args)
    	{
    	    struct Node *node;

    	    ForeachNode(args, node)
    	    {
    		if (0 == strncmp(node->ln_Name, "bootdelay=", 10))
    		{
    		    ULONG delay = atoi(&node->ln_Name[10]);

		    D(bug("[Boot] delay of %d seconds requested.", delay));
		    if (delay)
		    	bootDelay(delay * 50);
    		}
    		else if (0 == stricmp(node->ln_Name, "bootmenu"))
                {
                    D(bug("[BootMenu] bootmenu_Init: Forced with bootloader argument\n"));
                    WantBootMenu = TRUE;
                }
                /*
                 * TODO: The following two flags should have corresponding switches
                 * in 'display options' page.
                 */
		else if (0 == stricmp(node->ln_Name, "nomonitors"))
		{
		    LIBBASE->db_BootFlags |= BF_NO_DISPLAY_DRIVERS;
		}
		else if (0 == stricmp(node->ln_Name, "nocomposition"))
		{
		    LIBBASE->db_BootFlags |= BF_NO_COMPOSITION;
		}
		else if (0 == strnicmp(node->ln_Name, "bootdevice=", 11))
		{
		    LIBBASE->db_BootDevice = &node->ln_Name[11];
		    selectBootDevice(LIBBASE);
		}
    	    }
    	}
    }

    /* Scan for any additional partition volumes */
    dosboot_BootScan(LIBBASE);

    /* Show the boot menu if needed */
    bootmenu_Init(LIBBASE, WantBootMenu);

    /* We want to be able to find ourselves in RTF_AFTERDOS */
    LIBBASE->bm_Screen = NULL;
    AddResource(&LIBBASE->db_Node);

    /* Attempt to boot until we succeed */
    for (;;)
    {
        dosboot_BootStrap(LIBBASE);

        if (!LIBBASE->bm_Screen)
            LIBBASE->bm_Screen = NoBootMediaScreen(LIBBASE);

        D(bug("No bootable disk was found.\n"));
        D(bug("Please insert a bootable disk in any drive.\n"));
        D(bug("Retrying in 3 seconds...\n"));

        for (t = 0; t < 150; t += LIBBASE->delayTicks)
        {
            bootDelay(LIBBASE->delayTicks);

            if (LIBBASE->bm_Screen)
                anim_Animate(LIBBASE->bm_Screen, LIBBASE);
        }
    }

    /* We never get here */
    return FALSE;
}

ADD2INITLIB(dosboot_Init, 1)

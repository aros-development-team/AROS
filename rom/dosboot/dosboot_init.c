/*
    Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.

    Desc: Boot AROS
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
#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/partition.h>
#include <proto/bootloader.h>

#include LC_LIBDEFS_FILE

#include "dosboot_intern.h"
#include "../expansion/expansion_intern.h"
#include "menu.h"

/* Delay just like Dos/Delay(), ticks are
 * in 1/50th of a second.
 */
static void bootDelay(ULONG timeout)
{
    struct timerequest  timerio;
    struct MsgPort     timermp;
    
    memset(&timermp, 0, sizeof(timermp));
    
    timermp.mp_Node.ln_Type = NT_MSGPORT;
    timermp.mp_Flags        = PA_SIGNAL;
    timermp.mp_SigBit       = SIGB_SINGLE;
    timermp.mp_SigTask      = FindTask(NULL);
    NEWLIST(&timermp.mp_MsgList);
  
    timerio.tr_node.io_Message.mn_Node.ln_Type  = NT_REPLYMSG;
    timerio.tr_node.io_Message.mn_ReplyPort     = &timermp;
    timerio.tr_node.io_Message.mn_Length        = sizeof(timermp);

    if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)&timerio, 0) != 0) {
        D(bug("[DOSBoot] %s: Failed to open timer.device unit 0\n", __func__));
        return;
    }

    timerio.tr_node.io_Command  = TR_ADDREQUEST;
    timerio.tr_time.tv_secs     = timeout / TICKS_PER_SECOND;
    timerio.tr_time.tv_micro    = 1000000UL / TICKS_PER_SECOND * (timeout % TICKS_PER_SECOND);

    SetSignal(0, SIGF_SINGLE);

    DoIO(&timerio.tr_node);

    CloseDevice((struct IORequest *)&timerio);
}

static BOOL bstreqcstr(BSTR bstr, CONST_STRPTR cstr)
{
    int clen;
    int blen;

    clen = strlen(cstr);
    blen = AROS_BSTR_strlen(bstr);
    if (clen != blen)
        return FALSE;

    return (memcmp(AROS_BSTR_ADDR(bstr),cstr,clen) == 0);
}

static void selectBootDevice(LIBBASETYPEPTR LIBBASE, STRPTR bootDeviceName)
{
    struct BootNode *bn = NULL;

    if (bootDeviceName == NULL &&
        LIBBASE->db_BootNode != NULL)
        return;

    Forbid(); /* .. access to ExpansionBase->MountList */

    if (bootDeviceName != NULL)
    {
        struct BootNode *i;
        ForeachNode(&LIBBASE->bm_ExpansionBase->MountList, i)
        {
            struct DeviceNode *dn;

            dn = i->bn_DeviceNode;
            if (dn == NULL || dn->dn_Name == BNULL)
                continue;

            if (bstreqcstr(dn->dn_Name, bootDeviceName))
            {
                bn = i;
                break;
            }
        }
    }

    /* Default */
    if (bn == NULL)
        bn = (APTR)GetHead(&LIBBASE->bm_ExpansionBase->MountList);

    Permit();

    LIBBASE->db_BootNode = bn;
}


int dosboot_Init(LIBBASETYPEPTR LIBBASE)
{
    struct ExpansionBase *ExpansionBase;
    void *BootLoaderBase;
    BOOL WantBootMenu = FALSE;
    ULONG t;
    STRPTR bootDeviceName = NULL;

    LIBBASE->delayTicks = 50;

    D(bug("[DOSBoot] %s: GO GO GO!\n", __func__));

    ExpansionBase = (APTR)TaggedOpenLibrary(TAGGEDOPEN_EXPANSION);
    D(bug("[DOSBoot] %s: ExpansionBase 0x%p\n", __func__, ExpansionBase));
    if( ExpansionBase == NULL )
    {
        D(bug( "[DOSBoot] %s: Failed to open expansion.library\n", __func__));
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

                    D(bug("[DOSBoot] %s: delay of %d seconds requested.", __func__, delay));
                    if (delay)
                        bootDelay(delay * 50);
                }
                else if (0 == stricmp(node->ln_Name, "bootmenu"))
                {
                    D(bug("[DOSBoot] %s: bootmenu Forced with bootloader argument\n", __func__));
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
                    bootDeviceName = &node->ln_Name[11];
                }
                else if (0 == stricmp(node->ln_Name, "econsole"))
                {
                    LIBBASE->db_BootFlags |= BF_EMERGENCY_CONSOLE;
                    D(bug("[DOSBoot] %s: Emergency console selected\n", __func__));
                }
            }
            IntExpBase(ExpansionBase)->BootFlags = LIBBASE->db_BootFlags;
        }
    }

    D(bug("[DOSBoot] Scanning for additional partitions/volumes...\n"));

    /* Scan for any additional partition volumes */
    dosboot_BootScan(LIBBASE);

    D(bug("[DOSBoot] Prepairing initial device...\n"));

    /* Select the initial boot device, so that the choice is available in the menu */
    selectBootDevice(LIBBASE, bootDeviceName);

    D(bug("[DOSBoot] Enabling devices\n"));
    // Set all devices ENALBED by default
    {
        ListLength(&ExpansionBase->MountList, LIBBASE->devicesCount);
        D(bug("[DOSBoot] %d devices in mountlist (want %d bytes)\n", LIBBASE->devicesCount, LIBBASE->devicesCount * sizeof(BOOL) * 2));

        if ((LIBBASE->devicesCount > 0) &&
            ((LIBBASE->devicesEnabled = AllocVec (LIBBASE->devicesCount * sizeof(BOOL) * 2, MEMF_ANY|MEMF_CLEAR)) != NULL))
        {
            D(bug("[DOSBoot] devices enabled list storage @ 0x%p\n", LIBBASE->devicesEnabled));

            for(int i=0; i<LIBBASE->devicesCount; i++)
            {
                LIBBASE->devicesEnabled[i] = TRUE;
            }
        }
    }

    D(bug("[DOSBoot] ** Displaying bootmenu (if wanted) ...\n"));
    /* Show the boot menu if needed */
    bootmenu_Init(LIBBASE, WantBootMenu);

    D(bug("[DOSBoot] ** Prepairing to boot ...\n"));

    // Disable selected Devices
    // and
    // Set final boot device
    if (LIBBASE->devicesCount > 0)
    {
        struct List tempList;
        struct BootNode *bn, *temp;
        int index = 0;

        NewList(&tempList);

        ObtainSemaphore(&IntExpBase(ExpansionBase)->BootSemaphore);

        Forbid(); /* .. access to ExpansionBase->MountList */

        while ((temp = (struct BootNode *)RemHead(&ExpansionBase->MountList)) != NULL)
        {
            AddTail(&tempList, (struct Node *) temp);
        }

        ForeachNodeSafe (&tempList, bn, temp)
        {
            if ((!LIBBASE->devicesEnabled) || (LIBBASE->devicesEnabled[index++] == TRUE))
            {
                if (bn == LIBBASE->db_BootNode)
                {
                    bn->bn_Node.ln_Type = NT_BOOTNODE;
                    bn->bn_Node.ln_Pri = 127;
                    AddHead(&ExpansionBase->MountList, &bn->bn_Node);
                }
                else
                {
                    AddTail(&ExpansionBase->MountList, &bn->bn_Node);
                }
            }
        }

        Permit();

        ReleaseSemaphore(&IntExpBase(ExpansionBase)->BootSemaphore);


        if (LIBBASE->devicesEnabled != NULL)
        {
            FreeVec(LIBBASE->devicesEnabled);
        }
    }

    /* updates the boot flags */
    IntExpBase(ExpansionBase)->BootFlags = LIBBASE->db_BootFlags;

    /* We want to be able to find ourselves in RTF_AFTERDOS */
    LIBBASE->bm_Screen = NULL;
    AddResource(&LIBBASE->db_Node);

    D(bug("[DOSBoot] ** Attempting to boot ...\n"));

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

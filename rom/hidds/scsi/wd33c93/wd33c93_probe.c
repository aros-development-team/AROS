/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hardware detection routine
    Lang: English
*/

#include <aros/debug.h>
#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/expansion.h>
#include <proto/bootloader.h>
#include <proto/oop.h>

#include <aros/asmcall.h>
#include <aros/bootloader.h>
#include <aros/symbolsets.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <exec/rawfmt.h>
#include <hidd/bus.h>
#include <hidd/scsi.h>
#include <hidd/storage.h>
#include <hidd/hidd.h>
#include <oop/oop.h>

#include <interface/Hidd.h>

#include <string.h>

#include "wd33c93_intern.h"


#define NAME_BUFFER 128

#define RANGESIZE0 8
#define RANGESIZE1 4
#define DMASIZE    16

CONST_STRPTR scsiWD33c93Name = "scsi_wd33c93.hidd";
CONST_STRPTR WD33c93ControllerName = "WD33C93 SCSI Controller";
CONST_STRPTR A2091ControllerName = "A2091 WD33C93 SCSI Controller";

static int wd33c93Probe(struct scsiwd33c93Base *base)
{
    APTR BootLoaderBase;
    struct wd33c93ProbedBus *probedbus;

    D(bug("[SCSI:WD33C93] %s()\n", __PRETTY_FUNCTION__));

    /* Prepare lists for probed/found ide buses */
    NEWLIST(&base->probedbuses);
    base->buscount = 0;

    /* Obtain command line parameters */
    BootLoaderBase = OpenResource("bootloader.resource");
    D(bug("[SCSI:WD33C93] BootloaderBase = %p\n", BootLoaderBase));
    if (BootLoaderBase != NULL)
    {
        struct List *list;
        struct Node *node;

        list = (struct List *)GetBootInfo(BL_Args);
        if (list)
        {
            ForeachNode(list, node)
            {
                if (strncmp(node->ln_Name, "SCSI=", 4) == 0)
                {
                    const char *cmdline = &node->ln_Name[4];

                    if (strstr(cmdline, "disable"))
                    {
                        D(bug("[SCSI:WD33C93] %s: Disabling all SCSI devices\n", __func__));
                    }
                }
            }
        }
    }

    D(bug("[SCSI:WD33C93] %s: Enumerating devices\n", __func__));

#if (1)
    struct Library *ExpansionBase = (APTR)OpenLibrary("expansion.library", 0);
    if (ExpansionBase)
    {
        struct ConfigDev *cd = NULL;

        /* Look for Commodore A2091 SCSI controllers */
        while((cd = FindConfigDev(cd, 514, -1))) {
            switch (cd->cd_Rom.er_Product)
            {
                case 2:
                case 3:
                case 10:
                {
                    OOP_Object *scsiA2091 = NULL;
                    struct TagItem scsi_tags[] =
                    {
                        {aHidd_Name             , (IPTR)scsiWD33c93Name         },
                        {aHidd_HardwareName     , (IPTR)A2091ControllerName 	},
                        {aHidd_Producer         , 514				            },
                        {aHidd_Product          , cd->cd_Rom.er_Product         },
                        {TAG_DONE               , 0				                }
                    };
                    scsiA2091 = HW_AddDriver(base->storageRoot, base->scsiClass, scsi_tags);
                    if (scsiA2091)
                    {
                        D(bug("[SCSI:WD33C93] %s: A2091 (%04x:%04x) controller @ 0x%p\n", __func__, 514, cd->cd_Rom.er_Product, scsiA2091);)
                    }
                }
            }
        }
        CloseLibrary(ExpansionBase);
    }
#endif
    D(bug("[SCSI:WD33C93] %s: Finished..\n", __func__);)

    return TRUE;
}

ADD2INITLIB(wd33c93Probe, 30)

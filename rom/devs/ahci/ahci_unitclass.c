/*
    Copyright © 2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/utility.h>

#include <hidd/storage.h>
#include <hidd/ahci.h>
#include <oop/oop.h>

#include "ahci.h"


static void ahci_strcpy(const UBYTE *str1, UBYTE *str2, ULONG size)
{
    register int i = size;

    while (i--)
    {
        if (str1[i] < ' ')
            str2[i] = '\0';
        else
            str2[i] = str1[i];
    }
}

/*****************************************************************************************

    NAME
        --background_unitclass--

    LOCATION
        IID_Hidd_AHCIUnit

    NOTES
        Unit class is private to ahci.device. Instances of this class represent
        devices connected to AHCI buses, and can be used to obtain information
        about these devices.

*****************************************************************************************/

OOP_Object *AHCIUnit__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct AHCIBase *AHCIBase = cl->UserData;
    struct ahci_Bus *uBus = (struct ahci_Bus *)GetTagData(aHidd_DriverData, 0, msg->attrList);

    D(bug ("[AHCI:Unit] Root__New()\n");)

    if (!uBus)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct ahci_Unit *unit = OOP_INST_DATA(cl, o);
        struct ata_port  *at = uBus->ab_Port->ap_ata[0];

        D(bug ("[AHCI:Unit] Root__New: Unit Obj @ %p\n", o);)

        unit->au_Bus = uBus;
        uBus->ab_Unit = o;

        ahci_strcpy(at->at_identify.model, unit->au_Model, 40);
        D(bug ("[AHCI:Unit] Root__New: Model    %s\n", unit->au_Model);)
        ahci_strcpy(at->at_identify.serial, unit->au_SerialNumber, 20);
        D(bug ("[AHCI:Unit] Root__New: Serial   %s\n", unit->au_SerialNumber);)
        ahci_strcpy(at->at_identify.firmware, unit->au_FirmwareRev, 8);
        D(bug ("[AHCI:Unit] Root__New: FW Revis %s\n", unit->au_FirmwareRev);)
    }
    return o;
}

void AHCIUnit__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
#if (0)
    struct AHCIBase *AHCIBase = cl->UserData;
    struct ahci_Unit *unit = OOP_INST_DATA(cl, o);
#endif
    D(bug ("[AHCI:Unit] Root__Dispose(%p)\n", o);)

    OOP_DoSuperMethod(cl, o, msg);
}

void AHCIUnit__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct AHCIBase *AHCIBase = cl->UserData;
    struct ahci_Unit *unit = OOP_INST_DATA(cl, o);
    struct ata_port  *at = unit->au_Bus->ab_Port->ap_ata[0];
    ULONG idx;

    Hidd_StorageUnit_Switch (msg->attrID, idx)
    {
    case aoHidd_StorageUnit_Device:
        *msg->storage = (IPTR)"ahci.device";
        return;

    case aoHidd_StorageUnit_Number:
        *msg->storage = unit->au_UnitNum;
        return;

    case aoHidd_StorageUnit_Type:
        {
            switch (unit->au_Bus->ab_Port->ap_type)
            {
                case ATA_PORT_T_DISK:
                    {
                        if (at->at_identify.support_dsm & ATA_SUPPORT_DSM_TRIM)
                            *msg->storage = vHidd_StorageUnit_Type_SolidStateDisk;
                        else
                            *msg->storage = vHidd_StorageUnit_Type_FixedDisk;
                        break;
                    }
                case ATA_PORT_T_ATAPI:
                    *msg->storage = vHidd_StorageUnit_Type_OpticalDisc;
                    break;

                default:
                    *msg->storage = vHidd_StorageUnit_Type_Unknown;
                    break;
            }
            return;
        }

    case aoHidd_StorageUnit_Model:
        *msg->storage = (IPTR)unit->au_Model;
        return;

    case aoHidd_StorageUnit_Revision:
        *msg->storage = (IPTR)unit->au_FirmwareRev;
        return;

    case aoHidd_StorageUnit_Serial:
        *msg->storage = (IPTR)unit->au_SerialNumber;
        return;

    case aoHidd_StorageUnit_Removable:
        {
            if (at->at_identify.config & (1 << 7))
                *msg->storage = (IPTR)TRUE;
            else
                *msg->storage = (IPTR)FALSE;
            return;
        }
    }

    Hidd_AHCIUnit_Switch (msg->attrID, idx)
    {
    case aoHidd_AHCIUnit_Features:
        *msg->storage = (IPTR)at->at_features;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

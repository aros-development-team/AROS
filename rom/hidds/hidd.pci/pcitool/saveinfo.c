/*
    Copyright (C) 2003-2013, The AROS Development Team.
    $Id$
*/


#include "saveinfo.h"

BPTR DeviceInfoFile;

/*Save only the displayed PCI Information to RAM Disk*/
void SaveToDisk(struct PCIInfo *DeviceInfo)
{
    DeviceInfoFile = Open( "RAM:PCIToolInfo.txt", MODE_NEWFILE );
    if (DeviceInfoFile)
    {
        FPuts( DeviceInfoFile, "Driver Name: ");
        FPuts( DeviceInfoFile, DeviceInfo->Driver_name );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Direct bus: ");
        FPuts( DeviceInfoFile, DeviceInfo->Direct_bus );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Hardware info: ");
        FPuts( DeviceInfoFile, DeviceInfo->Hardware_info );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Description: ");
        FPuts( DeviceInfoFile, DeviceInfo->Description );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Vendor name: ");
        FPuts( DeviceInfoFile, DeviceInfo->Vendor_name );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Product name: ");
        FPuts( DeviceInfoFile, DeviceInfo->Product_name );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Subsystem: ");
        FPuts( DeviceInfoFile, DeviceInfo->Subsystem );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "VendorID: ");
        FPuts( DeviceInfoFile, DeviceInfo->VendorID );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "ProductID: ");
        FPuts( DeviceInfoFile, DeviceInfo->ProductID );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RevisionID: ");
        FPuts( DeviceInfoFile, DeviceInfo->RevisionID );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Class: ");
        FPuts( DeviceInfoFile, DeviceInfo->Class );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Subclass: ");
        FPuts( DeviceInfoFile, DeviceInfo->Subclass );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Interface: ");
        FPuts( DeviceInfoFile, DeviceInfo->Interface );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "ROM Base: ");
        FPuts( DeviceInfoFile, DeviceInfo->ROM_Base );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "ROM Size: ");
        FPuts( DeviceInfoFile, DeviceInfo->ROM_Size );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "IRQ: ");
        FPuts( DeviceInfoFile, DeviceInfo->IRQ );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Status: ");
        FPuts( DeviceInfoFile, DeviceInfo->Status );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_0: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_0 );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_1: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_1 );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_2: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_2 );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_3: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_3 );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_4: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_4 );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_5: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_5 );
        FPuts( DeviceInfoFile, "\n" );

        /*Clean up the Device Values*/
        CleanUpPCIToolInfo(DeviceInfo);

        Close( DeviceInfoFile );
    }
    else
    {
        // TODO: Requester
        PutStr("Can't open file for output\n");
    }
}

/*Just Open a new Textfile in RAM Disk. No writing yet*/
BOOL OpenPCIInfoFile(void)
{
    DeviceInfoFile = Open( "RAM:PCIToolInfo.txt", MODE_NEWFILE );

    if(DeviceInfoFile)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*Closes the file opened by OpenPCIInfoFile*/
void ClosePCIInfoFile(void)
{
    if(DeviceInfoFile)
    {
        Close(DeviceInfoFile);
    }
}

/*Writes a set of PCI Information to the Opened file*/
void WriteToPCIInfoFile(struct PCIInfo *DeviceInfo)
{
    if(DeviceInfoFile)
    {
        FPuts( DeviceInfoFile, "\n" );
        FPuts( DeviceInfoFile, "Driver Name: ");
        FPuts( DeviceInfoFile, DeviceInfo->Driver_name );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Direct bus: ");
        FPuts( DeviceInfoFile, DeviceInfo->Direct_bus );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Hardware info: ");
        FPuts( DeviceInfoFile, DeviceInfo->Hardware_info );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Description: ");
        FPuts( DeviceInfoFile, DeviceInfo->Description );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Vendor name: ");
        FPuts( DeviceInfoFile, DeviceInfo->Vendor_name );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Product name: ");
        FPuts( DeviceInfoFile, DeviceInfo->Product_name );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Subsystem: ");
        FPuts( DeviceInfoFile, DeviceInfo->Subsystem );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "VendorID: ");
        FPuts( DeviceInfoFile, DeviceInfo->VendorID );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "ProductID: ");
        FPuts( DeviceInfoFile, DeviceInfo->ProductID );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RevisionID: ");
        FPuts( DeviceInfoFile, DeviceInfo->RevisionID );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Class: ");
        FPuts( DeviceInfoFile, DeviceInfo->Class );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Subclass: ");
        FPuts( DeviceInfoFile, DeviceInfo->Subclass );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Interface: ");
        FPuts( DeviceInfoFile, DeviceInfo->Interface );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "ROM Base: ");
        FPuts( DeviceInfoFile, DeviceInfo->ROM_Base );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "ROM Size: ");
        FPuts( DeviceInfoFile, DeviceInfo->ROM_Size );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "IRQ: ");
        FPuts( DeviceInfoFile, DeviceInfo->IRQ );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "Status: ");
        FPuts( DeviceInfoFile, DeviceInfo->Status );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_0: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_0 );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_1: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_1 );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_2: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_2 );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_3: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_3 );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_4: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_4 );
        FPuts( DeviceInfoFile, "\n" );

        FPuts( DeviceInfoFile, "RangeList_5: ");
        FPuts( DeviceInfoFile, DeviceInfo->Rangelist_5 );
        FPuts( DeviceInfoFile, "\n" );
        FPuts( DeviceInfoFile, "\n" );
        /*Clean up the Device Values*/
        CleanUpPCIToolInfo(DeviceInfo);
    }
}

void CleanUpPCIToolInfo(struct PCIInfo *DeviceInfo)
{
    DeviceInfo->Driver_name[0] = '\0';
    DeviceInfo->Direct_bus[0] = '\0';
    DeviceInfo->Hardware_info[0] = '\0';
    DeviceInfo->Description[0] = '\0';
    DeviceInfo->Vendor_name[0] = '\0';
    DeviceInfo->Product_name[0] = '\0';
    DeviceInfo->Subsystem[0] = '\0';
    DeviceInfo->VendorID[0] = '\0';
    DeviceInfo->ProductID[0] = '\0';
    DeviceInfo->RevisionID[0] = '\0';
    DeviceInfo->Class[0] = '\0';
    DeviceInfo->Subclass[0] = '\0';
    DeviceInfo->Interface[0] = '\0';
    DeviceInfo->ROM_Base[0] = '\0';
    DeviceInfo->ROM_Size[0] = '\0';
    DeviceInfo->IRQ[0] = '0';
    DeviceInfo->Status[0] = '\0';
    DeviceInfo->Rangelist_0[0] = '\0';
    DeviceInfo->Rangelist_1[0] = '\0';
    DeviceInfo->Rangelist_2[0] = '\0';
    DeviceInfo->Rangelist_3[0] = '\0';
    DeviceInfo->Rangelist_4[0] = '\0';
    DeviceInfo->Rangelist_5[0] = '\0';
}

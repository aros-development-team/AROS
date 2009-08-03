/*
    Copyright (C) 2003-2009, The AROS Development Team.
    $Id$
*/


#include "saveinfo.h"

/*Save only the displayed PCI Information to RAM Disk*/
void SaveToDisk(struct PCIInfo *DeviceInfo)
{
	DeviceInfoFile = Open( "RAM:PCIToolInfo.txt", MODE_NEWFILE );
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
	

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Driver_name[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Direct_bus[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Hardware_info[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Description[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Vendor_name[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Product_name[CleanIndex] = ' ';
	}
	
	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Subsystem[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 10)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->VendorID[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 10)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->ProductID[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 10)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->RevisionID[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Class[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Subclass[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Interface[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 10)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->ROM_Base[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 10)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->ROM_Size[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 10)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->IRQ[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Status[CleanIndex] = ' ';
	}

	
CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Rangelist_0[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Rangelist_1[CleanIndex] = ' ';
	}

	CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Rangelist_2[CleanIndex] = ' ';
	}

CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Rangelist_3[CleanIndex] = ' ';
	}


CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Rangelist_4[CleanIndex] = ' ';
	}

CleanIndex = 0;
	while(CleanIndex < 200)
	{	
		CleanIndex = CleanIndex + 1;
		DeviceInfo->Rangelist_5[CleanIndex] = ' ';
	}



	
	
	
}


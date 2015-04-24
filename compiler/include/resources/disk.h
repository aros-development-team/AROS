/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef RESOURCES_DISK_H
#define RESOURCES_DISK_H

#include <exec/interrupts.h>
#include <exec/libraries.h>
#include <exec/ports.h>

struct DiscResourceUnit
{
	struct Message   dru_Message;
	struct Interrupt dru_DiscBlock;
	struct Interrupt dru_DiscSync;
	struct Interrupt dru_Index;
};

struct DiscResource
{
	struct Library           dr_Library;
	struct DiscResourceUnit *dr_Current;
	UBYTE                    dr_Flags;
	UBYTE                    dr_pad;
	struct Library          *dr_SysLib;
	struct Library          *dr_CiaResource;
	ULONG                    dr_UnitID[4];
	struct List              dr_Waiting;
	struct Interrupt         dr_DiscBlock;
	struct Interrupt         dr_DiscSync;
	struct Interrupt         dr_Index;
	struct Task             *dr_CurrTask;
};


#define DRB_ALLOC0  0
#define DRB_ALLOC1  1
#define DRB_ALLOC2  2
#define DRB_ALLOC3  3
#define DRB_ACTIVE  7

#define DRF_ALLOC0  (1<<DRB_ALLOC0)
#define DRF_ALLOC1  (1<<DRB_ALLOC1)
#define DRF_ALLOC2  (1<<DRB_ALLOC2)
#define DRF_ALLOC3  (1<<DRB_ALLOC3)
#define DRF_ACTIVE  (1<<DRB_ACTIVE)

#define DSKDMAOFF  0x4000

#define DISKNAME  "disk.resource"

#define DR_ALLOCUNIT    ((LIB_BASE - 0) * LIB_VECTSIZE)
#define DR_FREEUNIT     ((LIB_BASE - 1) * LIB_VECTSIZE)
#define DR_GETUNIT      ((LIB_BASE - 2) * LIB_VECTSIZE)
#define DR_GIVEUNIT     ((LIB_BASE - 3) * LIB_VECTSIZE)
#define DR_GETUNITID    ((LIB_BASE - 4) * LIB_VECTSIZE)
#define DR_READUNITID   ((LIB_BASE - 5) * LIB_VECTSIZE)

#define DR_LASTCOMM  (DR_READUNITID)

#define DRT_AMIGA     (0x00000000)
#define DRT_37422D2S  (0x55555555)
#define DRT_EMPTY     (0xFFFFFFFF)
#define DRT_150RPM    (0xAAAAAAAA)

#endif

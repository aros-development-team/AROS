/*
**	Q&D Protracker DataType
**
**	Written by Martin Blom, based on the AIFF DataType by
**	Olaf `Olsen' Barthel <olsen@sourcery.han.de>
**	Public domain
**
** :ts=8
*/

/******************************************************************************/

#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
#include <intuition/cghooks.h>

#include <dos/dosextens.h>
#include <dos/dostags.h>

#include <exec/execbase.h>
#include <exec/memory.h>

#include <datatypes/soundclass.h>

#include <devices/ahi.h>

#include <clib/intuition_protos.h>
#include <clib/datatypes_protos.h>
#include <clib/utility_protos.h>
#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <clib/ahi_protos.h>

#include <clib/macros.h>

#include <pragmas/intuition_pragmas.h>
#include <pragmas/datatypes_pragmas.h>
#include <pragmas/utility_pragmas.h>
#include <pragmas/exec_pragmas.h>
#include <pragmas/dos_pragmas.h>

#include <pragmas/ahi_pragmas.h>

#include <string.h>

#define MKID ((ULONG) ('M')<<24 | (ULONG) ('.')<<16 | (ULONG) ('K')<<8 | (ULONG) ('.'))

#include "/PT-AHIPlay/PT-AHIPlay.h"

/******************************************************************************/

	// Our custom library structure

struct ClassBase
{
	struct Library		 LibNode;		// Exec link
	UWORD			 Pad;			// Longword alignment
	Class			*SoundClass;		// The class this library implemnts

	struct ExecBase		*SysBase;		// Exec library
	struct DosLibrary	*DOSBase;		// Dos library
	struct Library		*IntuitionBase,		// Intuition library
				*UtilityBase,		// Utility library
				*DataTypesBase,		// DataTypes library
				*SuperClassBase;	// Sound datatype
	struct MsgPort		*AHImp;
	struct AHIRequest	*AHIio;
	BYTE			 AHIDevice, Pad2;
	struct AHIBase		*AHIBase;

	struct SignalSemaphore	 LockSemaphore;		// Shared access semaphore
	BPTR			 Segment;		// Library segment pointer
};

	// Redirect references to global data into the library base

#define SysBase		ClassBase->SysBase
#define DOSBase		ClassBase->DOSBase
#define IntuitionBase	ClassBase->IntuitionBase
#define UtilityBase	ClassBase->UtilityBase
#define DataTypesBase	ClassBase->DataTypesBase
#define SuperClassBase	ClassBase->SuperClassBase
#define LockSemaphore	ClassBase->LockSemaphore
#define SoundClass	ClassBase->SoundClass
#define AHImp		ClassBase->AHImp
#define AHIio		ClassBase->AHIio
#define AHIDevice	ClassBase->AHIDevice
#define AHIBase		ClassBase->AHIBase
#define Segment		ClassBase->Segment

#define COMMAND_ERROR		(0)
#define COMMAND_INIT		(1)
#define COMMAND_START		(2)
#define COMMAND_STOP		(3)
#define COMMAND_END		(4)
#define COMMAND_QUIT		(5)


struct ObjectData
{
	struct Message		 Message;
	UWORD			 Command;
	struct Process		*Slave;			// Makes the mt_#? calls
	BYTE			 ModInitialized;
	struct Hook		 PlayerHook;
	struct PTData		 ptdata;
};


/******************************************************************************/

	// Preprocessor tricks

#define LIBENT			__asm
#define REG(x)			register __ ## x

/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaOS specific partition initialization code.
    Lang: English.
*/

/****************************************************************************************/

#include <exec/libraries.h>
#include <exec/resident.h>
#include <proto/exec.h>

#include <libraries/partition.h>
#include "platform.h"
#include "partition_support.h"

/****************************************************************************************/

#define VERSION 1
#define REVISION 0

#define NAME_STRING    "partition.library"
#define VERSION_STRING "$VER: partition 1.0 (04.04.2002)\r\n"


/****************************************************************************************/

extern const char name[];
extern const char version[];
extern const APTR inittable[4];
extern void *const functable[];
extern struct PartitionBase *libinit();
extern const char libend;

/****************************************************************************************/

int entry(void) {
	return -1;
}

/****************************************************************************************/

const struct Resident ROMTag =
{
  RTC_MATCHWORD,
  (struct Resident *)&ROMTag,
  (APTR)&libend,
  RTF_AUTOINIT,
  VERSION,
  NT_LIBRARY,
  0,
  (char *)name,
  (char *)&version[6],
  (ULONG *)inittable
};

/****************************************************************************************/

const char name[]    = NAME_STRING;
const char version[] = VERSION_STRING;

const APTR inittable[4] =
{
    (APTR)sizeof(struct PartitionBase),
    (APTR)functable,
    NULL,
    (APTR)&libinit
};

/****************************************************************************************/

struct PartitionBase * SAVEDS ASM libinit
	(
		REGPARAM(d0, struct PartitionBase *, PartitionBase),
		REGPARAM(a0, BPTR, segList)
	)
{

	PartitionBase->lh.lh_SysBase = *(struct ExecBase **)4L;
	PartitionBase->lh.lh_SegList = segList;
	PartitionBase->lh.lh_LibNode.lib_Node.ln_Type  = NT_LIBRARY;
	PartitionBase->lh.lh_LibNode.lib_Node.ln_Name  = (char *)name;
	PartitionBase->lh.lh_LibNode.lib_Flags         = LIBF_SUMUSED|LIBF_CHANGED;
	PartitionBase->lh.lh_LibNode.lib_Version       = VERSION;
	PartitionBase->lh.lh_LibNode.lib_Revision      = REVISION;
	PartitionBase->lh.lh_LibNode.lib_IdString      = (char *)&version[6];
	PartitionBase->tables = (struct PartitionTableInfo **)PartitionSupport;
	return PartitionBase;
}

SAVEDS ASM BPTR libExpunge(REGPARAM(a6, struct PartitionBase *, PartitionBase)) {
BPTR retval;

	if (PartitionBase->lh.lh_LibNode.lib_OpenCnt != 0)
	{
		PartitionBase->lh.lh_LibNode.lib_Flags |= LIBF_DELEXP;
		return NULL;
	}
	Remove(&PartitionBase->lh.lh_LibNode.lib_Node);
	retval = PartitionBase->lh.lh_SegList;
	FreeMem
	(
		(char *)PartitionBase - PartitionBase->lh.lh_LibNode.lib_NegSize,
		PartitionBase->lh.lh_LibNode.lib_NegSize + PartitionBase->lh.lh_LibNode.lib_PosSize
	);
	return retval;
}

SAVEDS ASM struct PartitionBase *libOpen
	(
		REGPARAM(a6, struct PartitionBase *, PartitionBase),
		REGPARAM(d0, ULONG, version)
	)
{
	PartitionBase->lh.lh_LibNode.lib_Flags &= ~LIBF_DELEXP;
	PartitionBase->lh.lh_LibNode.lib_OpenCnt++;
	return PartitionBase;
}

SAVEDS ASM BPTR libClose(REGPARAM(a6, struct PartitionBase *, PartitionBase)) {

	PartitionBase->lh.lh_LibNode.lib_OpenCnt--;
	if ((PartitionBase->lh.lh_LibNode.lib_Flags & LIBF_DELEXP) != 0)
	{
		if (PartitionBase->lh.lh_LibNode.lib_OpenCnt == 0)
			return libExpunge(PartitionBase);
		PartitionBase->lh.lh_LibNode.lib_Flags &= ~LIBF_DELEXP;
	}
	return NULL;
}

SAVEDS ASM int libNull(REGPARAM(a6, struct PartitionBase *, PartitionBase)) {
    return 0;
}

extern void OpenRootPartition(void);
extern void CloseRootPartition(void);
extern void OpenPartitionTable(void);
extern void ClosePartitionTable(void);
extern void WritePartitionTable(void);
extern void CreatePartitionTable(void);
extern void AddPartition(void);
extern void DeletePartition(void);
extern void GetPartitionTableAttrs(void);
extern void SetPartitionTableAttrs(void);
extern void GetPartitionAttrs(void);
extern void SetPartitionAttrs(void);
extern void QueryPartitionTableAttrs(void);
extern void QueryPartitionAttrs(void);


void *const functable[]=
{
	libOpen,
	libClose,
	libExpunge,
	libNull,
	OpenRootPartition,     /* 5 */
	CloseRootPartition,
	OpenPartitionTable,
	ClosePartitionTable,
	WritePartitionTable,
	CreatePartitionTable, /* 10 */
	AddPartition,
	DeletePartition,
	GetPartitionTableAttrs,
	SetPartitionTableAttrs,
	GetPartitionAttrs,    /* 15 */
	SetPartitionAttrs,
	QueryPartitionTableAttrs,
	QueryPartitionAttrs,
	(void *)-1L
};

const char libend = 0;

/*

File: resident.c
Author: Neil Cafferkey
Copyright (C) 2008 Neil Cafferkey

This file is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this file; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "handler.h"

#define _STR(A) #A
#define STR(A) _STR(A)

#define HANDLER_NAME "amberram.handler"
#define VERSION 1
#define REVISION 9

#ifdef __AROS__
#define rom_tag Ram_ROMTag
#endif

extern LONG AmberMain(void);

static AROS_UFP3 (APTR, Init,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT handler_name[] = HANDLER_NAME;
static const TEXT version_string[] =
   HANDLER_NAME " " STR(VERSION) "." STR(REVISION) " (" __DATE__ ")\n";
static const TEXT dev_name[] = "RAM";

extern const TEXT dos_name[];

const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AFTERDOS,
   VERSION,
   NT_PROCESS,
   -125,
   (STRPTR)handler_name,
   (STRPTR)version_string,
   (APTR)Init
};

#ifdef DOSBase
#undef DOSBase
#endif

static AROS_UFH3 (APTR, Init,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
   struct DosLibrary *DOSBase;
   struct DeviceNode *dev_node;
   BPTR seg;

    AROS_USERFUNC_INIT

   /* Create device node and add it to the system. The handler will then be
      started when it is first accessed */

   DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", DOS_VERSION);
   dev_node = (APTR)MakeDosEntry(dev_name, DLT_DEVICE);
   if(dev_node == NULL)
      Alert(AT_DeadEnd | AG_NoMemory);

   seg = CreateSegList(AmberMain);
   if (seg == BNULL)
      Alert(AT_DeadEnd | AG_NoMemory);

   dev_node->dn_Handler = (BSTR)handler_name;
   dev_node->dn_StackSize = 10000;
   dev_node->dn_SegList = seg;
   dev_node->dn_Priority = 10;
   if(!AddDosEntry((APTR)dev_node))
      Alert(AT_DeadEnd);

   AROS_USERFUNC_EXIT

   return NULL;
}

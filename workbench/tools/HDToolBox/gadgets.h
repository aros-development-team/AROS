/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef GADGETS_H
#define GADGETS_H

#include <exec/types.h>
#include <libraries/gadtools.h>
#include <graphics/text.h>

struct creategadget {
	ULONG kind;
	struct NewGadget newgadget;
	struct TagItem *tags;
	struct Gadget *gadget;
};

enum
{
	ID_MAIN_FIRST_GADGET,
	ID_MAIN_TEXT=ID_MAIN_FIRST_GADGET,
	ID_MAIN_HARDDISK,
	ID_MAIN_CHANGE_DRIVE_TYPE,
	ID_MAIN_MODIFY_BBL,
	ID_MAIN_LL_FORMAT,
	ID_MAIN_PARTITION_DRIVE,
	ID_MAIN_VERIFY_DD,
	ID_MAIN_SAVE_CHANGES,
	ID_MAIN_HELP,
	ID_MAIN_EXIT,
	ID_MAIN_LAST_GADGET,
	ID_PCP_FIRST_GADGET=ID_MAIN_LAST_GADGET,
	ID_PCP_PARTITION=ID_PCP_FIRST_GADGET,
	ID_PCP_ADD_PARTITION,
	ID_PCP_DELETE_PARTITION,
	ID_PCP_EDIT_PARTITION,
	ID_PCP_STARTCYL,
	ID_PCP_ENDCYL,
	ID_PCP_TOTALCYL,
	ID_PCP_TYPELV,
	ID_PCP_TYPEINTEGER,
	ID_PCP_OK,
	ID_PCP_CANCEL,
	ID_PCP_LAST_GADGET,
	ID_AP_FIRST_GADGET=ID_PCP_LAST_GADGET,
	ID_AP_PARTITION=ID_AP_FIRST_GADGET,
	ID_AP_ADD_PARTITION,
	ID_AP_DELETE_PARTITION,
	ID_AP_NAME,
	ID_AP_STARTCYL,
	ID_AP_ENDCYL,
	ID_AP_TOTALCYL,
	ID_AP_BUFFERS,
	ID_AP_BOOT_PRIORITY,
	ID_AP_LAST_GADGET
};

struct Gadget *createGadgets(struct creategadget *, ULONG, ULONG, APTR);
void freeGadgets(struct Gadget *);
void clearGadgets(struct ExtGadget *, struct Window *, ULONG);

#endif


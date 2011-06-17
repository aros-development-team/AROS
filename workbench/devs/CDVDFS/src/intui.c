/* intui.c:
 *
 * System requester stuff.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 *		(C) Copyright 2002-2009 The AROS Development Team
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 * 
 * 18-Mar-09 sonic     AROS_KERNEL definition is used instead of __AROS__
 * 15-May-07 sonic     Show_CDDA_Icon() behaves better if called twice
 * 08-Apr-07 sonic     Removed DEBUG definition
 * 31-Mar-03 sonic     - fixed warnings
 * 07-Jul-02 sheutlin  various changes when porting to AROS
 *                     - global variables are now in a struct Globals *global
 * 18-Jul-94   fmu   - New CDDA icon (by Olivier Tonino)
 *                   - User-definable icon
 * 05-Jan-94   fmu   Retry displaying CD-DA icon if WB is not open.
 * 02-Jan-94   fmu   Location for CD-DA icon may be defined by user.
 * 11-Dec-93   fmu   Fixed AddAppIconA() call for DICE.
 * 28-Nov-93   fmu   Added custom CD-DA icon; removed GetDefDiskObject() call.
 * 09-Oct-93   fmu   SAS/C support added
 */

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/iobsolete.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <stdarg.h>
#include <stdio.h>

#include "debug.h"
#include "intui.h"
#include "globals.h"

extern struct Globals *global;

UWORD g_image_data[] = {
	/* Plane 0 */
		0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0001,0x1D5F,0x5000,
		0x0002,0x8000,0x0000,0x0002,0x8000,0x0000,0x001E,0xB9F1,0xF000,
		0x0030,0xEF0F,0x1800,0x002E,0xAAF2,0xE800,0x002E,0xEEF6,0xB800,
		0x002E,0xAB1A,0x8000,0x002E,0xA9EA,0xB800,0x002E,0xA9EA,0xE800,
		0x0030,0xAA1B,0x1800,0x001F,0x39F1,0xF000,0x0000,0x0000,0x0000,
		0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0xFFAB,0x9110,
		0x000F,0x04D5,0x6930,0x4430,0xAAAA,0x8BA4,0x0040,0x54D4,0x5440,
		0x54A8,0x0AA8,0xA87C,0x0955,0x44D5,0x5448,0x52AA,0xA02A,0x8014,
		0x22AA,0xA7D4,0x0062,0x5001,0x4D6A,0xAAD4,0xAAAA,0x07DF,0x554A,
		0x5250,0x882F,0xFED4,0xB805,0x5717,0xFDFA,0x552A,0xAE0B,0xFBD4,
		0xEF95,0x5C45,0xE3EE,0x5D62,0xB80A,0x1F5C,0xFFDD,0x5555,0xFBFE,
		0x7777,0xF777,0xD776,0xFFFE,0xFFFE,0x7FFE,0xFFFF,0xFFFF,0xFFFE,
	/* Plane 1 */
		0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x140D,0x1000,
		0x5455,0x4000,0x0454,0x0001,0x0000,0x0000,0x5541,0x6C04,0x0554,
		0x888F,0x10F0,0xE088,0x555B,0x5505,0x1554,0xAA91,0x1171,0x02AA,
		0x5551,0x54E5,0x5554,0xEED1,0x5595,0x46EE,0x555B,0x5415,0x1554,
		0xFFCF,0x55E4,0xE7FE,0x5540,0x6C04,0x0554,0xFFFF,0xFFFF,0xFFFE,
		0xDDDD,0xDDDD,0xDDDC,0xFFFF,0xFFFF,0xFFFE,0xFFFC,0x55FE,0x6EEE,
		0xFFF6,0xAEFB,0xDEEE,0xBB9F,0xFFF7,0x755A,0xFFFF,0xBEEE,0xAFBE,
		0xAAFF,0xF7DD,0x5FC2,0xF7FF,0xFABA,0xFFB6,0xA9FF,0xFFF7,0xFFEA,
		0xDF55,0x583F,0xFFBC,0xAAAA,0xB6DF,0xFFAA,0x53F5,0xFBAA,0xAAD4,
		0xABEF,0x7FF4,0x054A,0x45FB,0xFDFA,0x02C4,0xABDF,0xF9FD,0x0D2A,
		0x10EF,0xF2BE,0xBA10,0xA2DD,0xE5F7,0xEAA2,0x0013,0xFAAB,0x4800,
		0x8887,0x088A,0x4888,0x0000,0xD00A,0x8000,0x0000,0x0000,0x0000,
	/* Plane 2 */
		0xFFFF,0xFFFF,0xFFFE,0xFFFF,0xFFFF,0xFFFE,0xFFFE,0xE2A0,0xAFFE,
		0xFFFC,0x7FFF,0xFFFE,0xFFFC,0x7FFF,0xFFFE,0xFFE0,0x460E,0x0FFE,
		0xFFD0,0x4505,0x17FE,0xFFC0,0x440C,0x07FE,0xFFC4,0x440C,0x47FE,
		0xFFC4,0x4514,0x7FFE,0xFFC4,0x4604,0x47FE,0xFFC0,0x4604,0x07FE,
		0xFFD0,0x4415,0x17FE,0xFFE0,0xC60E,0x0FFE,0xFFFF,0xFFFF,0xFFFE,
		0xFFFF,0xFFFF,0xFFFE,0xFFFF,0xFFFF,0xFFFE,0xFFFF,0xFFAB,0xFFFE,
		0xFFFF,0x51D5,0x7FFE,0xFFF0,0x00A8,0x8BBE,0xFFC0,0x41D1,0x547E,
		0xFFA8,0x08A2,0xA87E,0xFF55,0x45C5,0x547E,0xFEAA,0xA02A,0x803E,
		0xFEAA,0xA454,0x007E,0xFD55,0x4BAA,0xAAFE,0xFE0A,0x045F,0x553E,
		0xFE10,0xA82F,0xFEBE,0xFE04,0x0357,0xFD3E,0xFE20,0x06AB,0xFA7E,
		0xFF10,0x0D55,0xE4FE,0xFFA2,0x1AAA,0x11FE,0xFFED,0x5555,0x87FE,
		0xFFF8,0xFFFC,0x3FFE,0xFFFF,0x0001,0xFFFE,0xFFFF,0xFFFF,0xFFFE
};

char *g_iconname = "CD-DA";

#ifdef SysBase
#	undef SysBase
#endif
#define SysBase global->SysBase

void Init_Intui() {
#ifndef AROS_KERNEL
struct IconBase *IconBase=NULL;
#endif

	global->IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
#ifndef AROS_KERNEL
	IconBase = (struct IconBase *)OpenLibrary("icon.library", 37);
	if (!IconBase)
	      Display_Error ("cannot open icon.library");
	global->IconBase = IconBase;
	global->WorkbenchBase = (struct WorkbenchBase *)OpenLibrary("workbench.library", 37);
	if (!global->WorkbenchBase)
	      Display_Error("cannot open workbench.library");
	global->g_user_disk_object = GetDiskObject ("env:cdda");
	if ((!IconBase) || (!global->g_user_disk_object))
	{
#else
        global->WorkbenchBase=NULL;
#endif
		global->g_xpos = NO_ICON_POSITION;
		global->g_ypos = NO_ICON_POSITION;
		global->g_disk_object_image.LeftEdge = 0;
		global->g_disk_object_image.TopEdge = 0;
		global->g_disk_object_image.Width = 47;
		global->g_disk_object_image.Height = 36;
		global->g_disk_object_image.Depth = 3;
		global->g_disk_object_image.ImageData = g_image_data;
		global->g_disk_object_image.PlanePick = 0xff;
		global->g_disk_object_image.PlaneOnOff = 0x00;
		global->g_disk_object_image.NextImage = NULL;

		global->g_disk_object.do_Magic = WB_DISKMAGIC;
		global->g_disk_object.do_Version = WB_DISKVERSION;
		global->g_disk_object.do_Gadget.NextGadget = NULL;
		global->g_disk_object.do_Gadget.LeftEdge = 0;
		global->g_disk_object.do_Gadget.TopEdge = 0;
		global->g_disk_object.do_Gadget.Width = 47;
		global->g_disk_object.do_Gadget.Height = 37;
		global->g_disk_object.do_Gadget.Flags = GADGIMAGE | GADGHCOMP;
		global->g_disk_object.do_Gadget.Activation = RELVERIFY | GADGIMMEDIATE;
		global->g_disk_object.do_Gadget.GadgetType = BOOLGADGET;
		global->g_disk_object.do_Gadget.GadgetRender = &global->g_disk_object_image;
		global->g_disk_object.do_Gadget.SelectRender = NULL;
		global->g_disk_object.do_Gadget.GadgetText = NULL;
		global->g_disk_object.do_Gadget.MutualExclude = 0;
		global->g_disk_object.do_Gadget.SpecialInfo = NULL;
		global->g_disk_object.do_Gadget.GadgetID = 0;
		global->g_disk_object.do_Gadget.UserData = NULL;
		global->g_disk_object.do_Type = 0;
		global->g_disk_object.do_DefaultTool = "";
		global->g_disk_object.do_ToolTypes = global->g_tool_types;
		global->g_disk_object.do_CurrentX = NO_ICON_POSITION;
		global->g_disk_object.do_CurrentY = NO_ICON_POSITION;
		global->g_disk_object.do_DrawerData = NULL;
		global->g_disk_object.do_ToolWindow = NULL;
		global->g_disk_object.do_StackSize = 0;
		global->g_user_disk_object = &global->g_disk_object;
#ifndef AROS_KERNEL
	}
	else
	{
	char *name;
		name = FindToolType (global->g_user_disk_object->do_ToolTypes, "ICONNAME");
		if (name)
			g_iconname = name;
	}
#endif

	global->g_app_port = NULL;
	global->g_app_sigbit = 0;
	global->g_app_icon = NULL;
}

void Close_Intui() {
	if (global->WorkbenchBase)
		CloseLibrary ((struct Library *)global->WorkbenchBase);
	if (global->IconBase)
		CloseLibrary ((struct Library *)global->IconBase);
	if (global->IntuitionBase)
		CloseLibrary ((struct Library *)global->IntuitionBase);
}

#ifdef IntuitionBase
#	undef IntutitionBase
#endif
#define IntuitionBase global->IntuitionBase
#ifdef IconBase
#	undef IconBase
#endif
#define IconBase global->IconBase
#ifdef WorkbenchBase
#	undef WorkbenchBase
#endif
#define WorkbenchBase global->WorkbenchBase

void Display_Error_Tags (char *p_message, APTR arg)
{
    static struct EasyStruct req =
    {
	sizeof (struct EasyStruct),
	0,
	(UBYTE *) "CDROM Handler Error",
	NULL,
	(UBYTE *) "Abort"
    };

    if (IntuitionBase)
    {
#ifdef AROS_KERNEL
	if (!IntuitionBase->FirstScreen)
	    return;
#endif
        req.es_TextFormat = (UBYTE *) p_message;
        EasyRequestArgs (NULL, &req, NULL, arg);
    }
}

void Show_CDDA_Icon (void) {

	if (!IconBase || !WorkbenchBase)
		return;
	if (global->g_app_icon) {
		Display_Error("Show_CDDA_Icon called twice!");
		return;
	}

	global->g_user_disk_object->do_CurrentX = global->g_xpos;
	global->g_user_disk_object->do_CurrentY = global->g_ypos;

	global->g_app_port = CreateMsgPort ();
	if (!global->g_app_port)
		return;

	global->g_app_sigbit = 1<<global->g_app_port->mp_SigBit;

	global->g_app_icon = AddAppIconA
		(
			0,
			0,
			(UBYTE *) g_iconname,
			global->g_app_port,
			NULL,
			global->g_user_disk_object,
			NULL
		);

	/*
		AddAppIconA may fail if the Workbench has not yet been loaded.
   */
	if (!global->g_app_icon)
	{
		DeleteMsgPort (global->g_app_port);
		global->g_app_port = NULL;
		global->g_retry_show_cdda_icon = TRUE;
		return;
	}
	global->g_retry_show_cdda_icon = FALSE;
}

void Hide_CDDA_Icon(void) {
struct Message *msg;

	global->g_retry_show_cdda_icon = FALSE;

	if (!IconBase || !WorkbenchBase)
		return;

	if (global->g_app_icon)
		RemoveAppIcon (global->g_app_icon);
	if (global->g_app_port)
	{
		while ((msg = GetMsg (global->g_app_port)))
			ReplyMsg (msg);
		DeleteMsgPort (global->g_app_port);
	}

	global->g_app_port = NULL;
	global->g_app_sigbit = 0;
	global->g_app_icon = NULL;
}


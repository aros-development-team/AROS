/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Manage the Desktop
    Lang: English
*/

/*********************************************************************************************/

#define   DEBUG  0
#include  <aros/debug.h>

#include <intuition/intuitionbase.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/alib.h>

#include <stdlib.h>

#include "Desktop.h"

extern ULONG notifysig;
extern struct Screen  * wbscreen;
extern APTR vi;

struct Window  * wbwindow;
struct Menu *    menus = NULL;

struct Desktop Desktop =
{
    TRUE
};

static struct NewMenu nm[] =
{
  {NM_TITLE, "Workbench"              },
    {NM_ITEM,  "Backdrop",           "B", CHECKIT},
    {NM_ITEM,  "Execute Command...", "E"},
    {NM_ITEM,  "Redraw All" },
    {NM_ITEM,  "Update All" },
    {NM_ITEM,  "Last Message" },
    {NM_ITEM,  "Shell",              "Z"},
    {NM_ITEM,  "About...",           "?"},
    {NM_ITEM,  "Quit...",            "Q"},

  {NM_TITLE, "Window",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "New Drawer", "N"},
    {NM_ITEM,  "Open Parent" },
    {NM_ITEM,  "Close", "K"},
    {NM_ITEM,  "Update" },
    {NM_ITEM,  "Select Contents", "A"},
    {NM_ITEM,  "Clean Up", "."},
    {NM_ITEM,  "Snapshot" },
      {NM_SUB, "Window"},
      {NM_SUB, "All"},
    {NM_ITEM,  "Show" },
      {NM_SUB, "Only Icons", NULL, CHECKIT | CHECKED, 32},
      {NM_SUB, "All Files", NULL, CHECKIT, 16 },
    {NM_ITEM,  "View By" },
      {NM_SUB, "Icon", NULL, CHECKIT | CHECKED, 32 + 64 + 128},
      {NM_SUB, "Name",NULL, CHECKIT, 16 + 64 + 128},
      {NM_SUB, "Size",NULL, CHECKIT, 16 + 32 + 128},
      {NM_SUB, "Date", NULL, CHECKIT, 16 + 32 + 64},

  {NM_TITLE, "Icon",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "Open", "O"},
    {NM_ITEM,  "Close","C" },
    {NM_ITEM,  "Rename...", "R"},
    {NM_ITEM,  "Information...", "I" },
    {NM_ITEM,  "Snapshot", "S" },
    {NM_ITEM,  "Unsnapshot", "U" },
    {NM_ITEM,  "Leave Out", "L" },
    {NM_ITEM,  "Put Away", "P" },
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM,  "Delete..." },
    {NM_ITEM,  "Format Disk..." },
    {NM_ITEM,  "Empty Trash..." },

  {NM_TITLE, "Tools",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "ResetWB" },
  {NM_END}

};


/*********************************************************************************************/

int OpenDesktop(ULONG what)
{
int retval = RETURN_OK;
ULONG WinFlags;

    kprintf("LoadWB.OpenDesktop\n");

    WinFlags = (WFLG_ACTIVATE | WFLG_WBENCHWINDOW);
    if(Desktop.Backdrop)
    {
	nm[1].nm_Flags |= CHECKED;
	WinFlags |= (WFLG_BACKDROP | WFLG_BORDERLESS);
    }
    else
    {
	nm[1].nm_Flags &= ~CHECKED;
	WinFlags |= (WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_SIZEBRIGHT);
    }

    if(what & DESKTOP_Main)
    {
	wbwindow = OpenWindowTags(NULL,
	WA_ScreenTitle, "AROS Workbench Alpha 0.1 (No file navigation) FreeMem-xxxxxxx",
		WA_Title,	    "AROS Workbench Alpha Version 0.1 (No file navigation)",
		WA_Flags,	   WinFlags,
		WA_IDCMP,	   (IDCMP_MENUPICK | IDCMP_CLOSEWINDOW),
		WA_Left,	   0,
		WA_Top,		   wbscreen->BarHeight + 1,
		WA_Width,	   wbscreen->Width,
		WA_Height,	   wbscreen->Height - wbscreen->BarHeight -1,
		TAG_DONE
		);

	WindowToBack(wbwindow);

	notifysig |= 1L << wbwindow->UserPort->mp_SigBit;

	if(menus)
	{
	    ResetMenuStrip(wbwindow, menus);
	}
	else
	{
	    menus = CreateMenusA(nm, NULL);
	    LayoutMenusA(menus, vi, NULL);
	    SetMenuStrip(wbwindow, menus);
	}
    }

    return retval;
}

/*********************************************************************************************/

int CloseDesktop(ULONG what)
{
int retval;
    kprintf("LoadWB.CloseDesktop\n");

    if(what & DESKTOP_Main)
    {
	ClearMenuStrip(wbwindow);
	CloseWindow(wbwindow);
    }

    return retval;
}

/*********************************************************************************************/

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include "mainwin.h"
#include "error.h"
#include "gadgets.h"

#include <aros/debug.h>

struct TagItem txttags[]=
{
	{GTTX_Text, (ULONG)"Interface  Address   LUN   Status    Drive Type"},
	{GTTX_CopyText, TRUE},
	{GTTX_Clipped, TRUE},
	{TAG_DONE,0}
};
struct TagItem hdtags[]=
{
	{GTLV_Labels,0},
	{GTLV_ShowSelected,0},
	{TAG_DONE,0}
};
struct TagItem cdttags[]=
{
	{GA_Disabled,TRUE},
	{TAG_DONE,0}
};
struct TagItem mbbltags[]=
{
	{GA_Disabled,TRUE},
	{TAG_DONE,0}
};
struct TagItem llftags[]=
{
	{GA_Disabled,TRUE},
	{TAG_DONE,0}
};
struct TagItem pdtags[]=
{
	{GA_Disabled,TRUE},
	{TAG_DONE,0}
};
struct TagItem vdodtags[]=
{
	{GA_Disabled,TRUE},
	{TAG_DONE,0}
};
struct TagItem sctdtags[]=
{
	{GA_Disabled,TRUE},
	{TAG_DONE,0}
};
struct TagItem helptags[]=
{
	{TAG_DONE,0}
};
struct TagItem exittags[]=
{
	{TAG_DONE,0}
};


struct creategadget maingadgets[]=
{
	{
		TEXT_KIND,
		{
			10,20,400,20,
			NULL, NULL,
			ID_MAIN_TEXT, NULL, NULL, NULL
		},
		txttags
	},
	{
		LISTVIEW_KIND,
		{
			10,40,620,80,
			NULL, NULL,
			ID_MAIN_HARDDISK, NULL, NULL, NULL
		},
		hdtags
	},
	{
		BUTTON_KIND,
		{
			70,140,190,20,
			"Change Drive Type", NULL,
			ID_MAIN_CHANGE_DRIVE_TYPE, PLACETEXT_IN, NULL, NULL
		},
		cdttags
	},
	{
		BUTTON_KIND,
		{
			70,165,190,20,
			"Modify Bad Block List", NULL,
			ID_MAIN_MODIFY_BBL, PLACETEXT_IN, NULL, NULL
		},
		mbbltags
	},
	{
		BUTTON_KIND,
		{
			70,190,190,20,
			"Low Level Format Drive", NULL,
			ID_MAIN_LL_FORMAT, PLACETEXT_IN, NULL, NULL
		},
		llftags
	},
	{
		BUTTON_KIND,
		{
			380,140,190,20,
			"Partition Drive", NULL,
			ID_MAIN_PARTITION_DRIVE, PLACETEXT_IN, NULL, NULL
		},
		pdtags
	},
	{
		BUTTON_KIND,
		{
			380,165,190,20,
			"Verify Data on Drive", NULL,
			ID_MAIN_VERIFY_DD, PLACETEXT_IN, NULL, NULL
		},
		vdodtags
	},
	{
		BUTTON_KIND,
		{
			380,190,190,20,
			"Save Changes to Drive", NULL,
			ID_MAIN_SAVE_CHANGES, PLACETEXT_IN, NULL, NULL
		},
		sctdtags
	},
	{
		BUTTON_KIND,
		{
			290,160,60,20,
			"Help", NULL,
			ID_MAIN_HELP, PLACETEXT_IN, NULL, NULL
		},
		helptags
	},
	{
		BUTTON_KIND,
		{
			250,215,140,20,
			"Exit", NULL,
			ID_MAIN_EXIT, PLACETEXT_IN, NULL, NULL
		},
		exittags
	}
};


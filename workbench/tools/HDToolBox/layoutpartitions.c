/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/lists.h>
#include <exec/nodes.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include "gadgets.h"
#include "platform.h"

struct List partition_list;
struct TagItem pcpartitiontags[] =
{
	{GTLV_Labels, 0},
	{GTLV_MakeVisible, 0},
	{GTLV_Selected, ~0},
	{GTLV_ShowSelected, 0},
	{TAG_DONE, NULL}
};
struct TagItem pcpaddpartitiontags[] =
{
	{GA_Disabled, TRUE},
	{TAG_DONE,NULL}
};
struct TagItem pcpdeletepartitiontags[] =
{
	{GA_Disabled, TRUE},
	{TAG_DONE,NULL}
};
struct TagItem pcpstartcyltags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem pcpendcyltags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem pcptotalcyltags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem pcpsizetags[] =
{
	{GA_Disabled, TRUE},
	{GTTX_Text, (STACKIPTR)""},
	{GTTX_Clipped, TRUE},
	{TAG_DONE,NULL}
};
struct TagItem pcpnametags[] =
{
	{GA_Disabled, TRUE},
	{GTST_String, (STACKIPTR)""},
	{GTST_MaxChars, 16},
	{TAG_DONE,NULL}
};
struct TagItem pcpbootabletags[] =
{
	{GA_Disabled, TRUE},
	{GTCB_Checked, FALSE},
	{TAG_DONE,NULL}
};
struct TagItem pcpbootpritags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem pcpfilesystemtags[] =
{
	{GA_Disabled, TRUE},
	{GTTX_Text, (STACKIPTR)""},
	{GTTX_Clipped, TRUE},
	{GTTX_Justification, GTJ_CENTER},
	{TAG_DONE,NULL}
};
struct TagItem pcpupdatefstags[] =
{
	{GA_Disabled, TRUE},
	{TAG_DONE,NULL}
};
struct TagItem pcpeditarospartitiontags[] =
{
	{GA_Disabled, TRUE},
	{TAG_DONE,NULL}
};
struct TagItem pcpoktags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem pcpcanceltags[] =
{
	{TAG_DONE,NULL}
};



struct creategadget pcpgadgets[] =
{
	{
		LISTVIEW_KIND,
		{
			20,90,181,60,
			NULL, NULL,
			ID_PCP_PARTITION, NULL, NULL, NULL
		},
		pcpartitiontags
	},
	{
		BUTTON_KIND,
		{
			20,150,90,20,
			"Add", NULL,
			ID_PCP_ADD_PARTITION, PLACETEXT_IN, NULL, NULL
		},
		pcpaddpartitiontags
	},
	{
		BUTTON_KIND,
		{
			110,150,90,20,
			"Delete", NULL,
			ID_PCP_DELETE_PARTITION, PLACETEXT_IN, NULL, NULL
		},
		pcpdeletepartitiontags
	},
	{
		INTEGER_KIND,
		{
			100,180,80,15,
			"Start Cyl", NULL,
			ID_PCP_STARTCYL, PLACETEXT_LEFT, NULL, NULL
		},
		pcpstartcyltags
	},
	{
		INTEGER_KIND,
		{
			100,200,80,15,
			"End Cyl", NULL,
			ID_PCP_ENDCYL, PLACETEXT_LEFT, NULL, NULL
		},
		pcpendcyltags
	},
	{
		INTEGER_KIND,
		{
			100,220,80,15,
			"Total Cyl", NULL,
			ID_PCP_TOTALCYL, PLACETEXT_LEFT, NULL, NULL
		},
		pcptotalcyltags
	},
	{
		TEXT_KIND,
		{
			300, 90, 80, 15,
			"Size:", NULL,
			ID_PCP_SIZE, PLACETEXT_LEFT, NULL, NULL
		},
		pcpsizetags
	},
	{
		STRING_KIND,
		{
			300, 108, 80, 15,
			"Name:", NULL,
			ID_PCP_NAME, PLACETEXT_LEFT, NULL, NULL
		},
		pcpnametags
	},
	{
		CHECKBOX_KIND,
		{
			300, 126, 0, 0,
			"Bootable:", NULL,
			ID_PCP_BOOTABLE, PLACETEXT_LEFT, NULL, NULL
		},
		pcpbootabletags
	},
	{
		INTEGER_KIND,
		{
			300,140,80,15,
			"Boot Pri:", NULL,
			ID_PCP_BOOTPRI, PLACETEXT_LEFT, NULL, NULL
		},
		pcpbootpritags
	},
	{
		TEXT_KIND,
		{
			400, 90, 180, 15,
			"FileSystem:", NULL,
			ID_PCP_FILESYSTEM, PLACETEXT_ABOVE, NULL, NULL
		},
		pcpfilesystemtags,
	},
	{
		BUTTON_KIND,
		{
			400,108,180,20,
			"Update FileSystems", NULL,
			ID_PCP_UPDATE_FS, PLACETEXT_IN, NULL, NULL
		},
		pcpupdatefstags
	},
	{
		BUTTON_KIND,
		{
			400,136,180,20,
			"Edit", NULL,
			ID_PCP_EDIT_PARTITION, PLACETEXT_IN, NULL, NULL
		},
		pcpeditarospartitiontags
	},
	{
		BUTTON_KIND,
		{
			570,190,50,20,
			"Ok", NULL,
			ID_PCP_OK, PLACETEXT_IN, NULL, NULL
		},
		pcpoktags
	},
	{
		BUTTON_KIND,
		{
			570,215,50,20,
			"Cancel", NULL,
			ID_PCP_CANCEL, PLACETEXT_IN, NULL, NULL
		},
		pcpcanceltags
	}
};


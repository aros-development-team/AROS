/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/lists.h>
#include <exec/nodes.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include "gadgets.h"
#include "platform.h"

struct TagItem dettypelvtags[] =
{
	{GA_Disabled, FALSE},
	{GTLV_Labels,0},
	{GTLV_MakeVisible, 0},
	{GTLV_Selected, ~0},
	{GTLV_ShowSelected, 0},
	{TAG_DONE,NULL}
};
struct TagItem dettypestringtags[] =
{
	{GA_Disabled, FALSE},
	{GTST_String, 0},
	{GTST_MaxChars, 12},
	{TAG_DONE,NULL}
};
struct TagItem detpartitiontabletags[] =
{
	{GA_Disabled, FALSE},
	{GTCB_Checked, FALSE},
	{TAG_DONE,NULL}
};
STRPTR *blocksizelabels[]={"512","1024","2048",NULL};
struct TagItem detblocksizetags[] =
{
	{GA_Disabled, TRUE},
	{GTCY_Active, 0},
	{GTCY_Labels, (STACKIPTR)blocksizelabels},
	{TAG_DONE,NULL}
};
struct TagItem detbufferstags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem detmasktags[] =
{
	{GA_Disabled, TRUE},
	{GTST_String, (STACKIPTR)"0xFFFFFFFE"},
	{GTST_MaxChars, 10},
	{TAG_DONE,NULL}
};
struct TagItem detmaxtransfertags[] =
{
	{GA_Disabled, TRUE},
	{GTST_String, (STACKIPTR)"0x003FFFFF"},
	{GTST_MaxChars, 10},
	{TAG_DONE,NULL}
};
struct TagItem detautomounttags[] =
{
	{GA_Disabled, TRUE},
	{GTCB_Checked, FALSE},
	{TAG_DONE,NULL}
};
struct TagItem detcustboottags[] =
{
	{GA_Disabled, TRUE},
	{GTCB_Checked, FALSE},
	{TAG_DONE,NULL}
};
struct TagItem detcustbbtags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem detreservedtags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem detbegintags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem detendtags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem detoktags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem detcanceltags[] =
{
	{TAG_DONE,NULL}
};



struct creategadget detailsgadgets[] =
{
	{
		LISTVIEW_KIND,
		{
			20,45,220,120,
			NULL, NULL,
			ID_DET_TYPELV, PLACETEXT_LEFT, NULL, NULL
		},
		dettypelvtags
	},
	{
		STRING_KIND,
		{
			20,165,100,15,
			NULL, NULL,
			ID_DET_TYPESTRING, NULL, NULL, NULL
		},
		dettypestringtags
	},
	{
		CHECKBOX_KIND,
		{
			180, 30, 0, 0,
			"Partition Table:", NULL,
			ID_DET_PARTITION_TABLE, PLACETEXT_LEFT, NULL, NULL
		},
		detpartitiontabletags
	},
	{
		CYCLE_KIND,
		{
			120,190,100,15,
			"Block Size:", NULL,
			ID_DET_BLOCKSIZE, PLACETEXT_LEFT, NULL, NULL
		},
		detblocksizetags		
	},
	{
		INTEGER_KIND,
		{
			120,208,100,15,
			"Buffers:", NULL,
			ID_DET_BUFFERS, PLACETEXT_LEFT, NULL, NULL
		},
		detbufferstags
	},
	{
		STRING_KIND,
		{
			420, 30,100,15,
			"Mask:", NULL,
			ID_DET_MASK, PLACETEXT_LEFT, NULL, NULL
		},
		detmasktags
	},
	{
		STRING_KIND,
		{
			420, 48,100,15,
			"Max Transfer:", NULL,
			ID_DET_MAX_TRANSFER, PLACETEXT_LEFT, NULL, NULL
		},
		detmaxtransfertags
	},
	{
		CHECKBOX_KIND,
		{
			420, 70, 0, 0,
			"Automount:", NULL,
			ID_DET_AUTOMOUNT, PLACETEXT_LEFT, NULL, NULL
		},
		detautomounttags
	},
	{
		CHECKBOX_KIND,
		{
			420, 86, 0, 0,
			"Custom Bootcode:", NULL,
			ID_DET_CUSTBOOT, PLACETEXT_LEFT, NULL, NULL
		},
		detcustboottags
	},
	{
		INTEGER_KIND,
		{
			420,102,100,15,
			"Custom Bootblocks:", NULL,
			ID_DET_CUSTBB, PLACETEXT_LEFT, NULL, NULL
		},
		detcustbbtags
	},
	{
		TEXT_KIND,
		{
			420,120,100,15,
			"Reserved blocks at:", NULL,
			ID_DET_RESERVED, PLACETEXT_LEFT, NULL, NULL
		},
		detreservedtags
	},
	{
		INTEGER_KIND,
		{
			420,138,100,15,
			"Begining:", NULL,
			ID_DET_BEGINING, PLACETEXT_LEFT, NULL, NULL
		},
		detbegintags
	},
	{
		INTEGER_KIND,
		{
			420,156,100,15,
			"End:", NULL,
			ID_DET_END, PLACETEXT_LEFT, NULL, NULL
		},
		detendtags
	},
	{
		BUTTON_KIND,
		{
			570,190,50,20,
			"Ok", NULL,
			ID_DET_OK, PLACETEXT_IN, NULL, NULL
		},
		detoktags
	},
	{
		BUTTON_KIND,
		{
			570,215,50,20,
			"Cancel", NULL,
			ID_DET_CANCEL, PLACETEXT_IN, NULL, NULL
		},
		detcanceltags
	}
};


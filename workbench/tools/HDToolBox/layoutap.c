/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gadgets.h"

struct TagItem apartitiontags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem apaddpartitiontags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem apdeletepartitiontags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem apnametags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem apstartcyltags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem apendcyltags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem aptotalcyltags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem apbufferstags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem apbootprioritytags[] =
{
	{TAG_DONE,NULL}
};

struct creategadget apgadgets[]=
{
	{
		LISTVIEW_KIND,
		{
			20,90,220,125,
			NULL, NULL,
			ID_AP_PARTITION, NULL, NULL, NULL
		},
		apartitiontags
	},
	{
		BUTTON_KIND,
		{
			20,215,110,20,
			"Add Partition", NULL,
			ID_AP_ADD_PARTITION, PLACETEXT_IN, NULL, NULL
		},
		apaddpartitiontags
	},
	{
		BUTTON_KIND,
		{
			130,215,110,20,
			"Delete Partition", NULL,
			ID_AP_DELETE_PARTITION, PLACETEXT_IN, NULL, NULL
		},
		apdeletepartitiontags
	},
	{
		STRING_KIND,
		{
			320,90,50,15,
			"Name", NULL,
			ID_AP_NAME, PLACETEXT_LEFT, NULL, NULL
		},
		apnametags
	},
	{
		INTEGER_KIND,
		{
			320,110,50,15,
			"Start Cyl", NULL,
			ID_AP_STARTCYL, PLACETEXT_LEFT, NULL, NULL
		},
		apstartcyltags
	},
	{
		INTEGER_KIND,
		{
			320,130,50,15,
			"End Cyl", NULL,
			ID_AP_ENDCYL, PLACETEXT_LEFT, NULL, NULL
		},
		apendcyltags
	},
	{
		INTEGER_KIND,
		{
			320,150,50,15,
			"Total Cyl", NULL,
			ID_AP_TOTALCYL, PLACETEXT_LEFT, NULL, NULL
		},
		aptotalcyltags
	},
	{
		INTEGER_KIND,
		{
			320,170,50,15,
			"Buffers", NULL,
			ID_AP_BUFFERS, PLACETEXT_LEFT, NULL, NULL
		},
		apbufferstags
	},
	{
		INTEGER_KIND,
		{
			320,190,50,15,
			"Boot Priority", NULL,
			ID_AP_BOOT_PRIORITY, PLACETEXT_LEFT, NULL, NULL
		},
		apbootprioritytags
	}
};

	

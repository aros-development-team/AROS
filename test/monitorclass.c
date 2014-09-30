/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <cybergraphx/cybergraphics.h>
#include <intuition/monitorclass.h>

#include <proto/alib.h>
#include <proto/intuition.h>

#include <stdio.h>

static STRPTR pfnames[] =
{
    "LUT8",
    "RGB15",
    "BGR15",
    "RGB15PC",
    "BGR15PC",
    "RGB16",
    "BGR16",
    "RGB16PC",
    "BGR16PC",
    "RGB24",
    "BGR24",
    "ARGB32",
    "BGRA32",
    "RGBA32"
};

static UBYTE depths[] = {8, 15, 16, 24, 32, 0};

int main(void)
{
    Object **monitors, **mon;
    
    monitors = GetMonitorList(NULL);
    
    if (!monitors)
    {
	printf("Failed to obtain monitors list!\n");
	return 0;
    }
    
    for (mon = monitors; *mon; mon++)
    {
	STRPTR name, drvname;
	ULONG *pfs;
	UBYTE i;

	GetAttr(MA_MonitorName, *mon, (IPTR *)&name);
	GetAttr(MA_DriverName, *mon, (IPTR *)&drvname);
	printf("Monitor %p %s %s\n", *mon, name, drvname);

	printf("Supported pixelformats:\n");
	GetAttr(MA_PixelFormats, *mon, (IPTR *)&pfs);
	for (i = PIXFMT_LUT8; i <= PIXFMT_RGBA32; i++)
	    printf(" %7s %s\n", pfnames[i], pfs[i] ? "yes" : "no");

	printf("Preferred pixelformats:\n");
	for (i = 0; depths[i]; i++)
	{
	    IPTR pf;

	    printf(" %2d ", depths[i]);
	    DoMethod(*mon, MM_GetDefaultPixelFormat, depths[i], (IPTR *)&pf);

	    if (pf <= PIXFMT_RGBA32)
		printf("%s\n", pfnames[pf]);
	    else if (pf == -1)
		printf("Not supported\n");
	    else
		printf("Unknown (%ld)\n", pf);
	}
	printf("\n");
    }
    
    FreeMonitorList(monitors);
    return 0;
}

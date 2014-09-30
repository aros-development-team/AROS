/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <dos/dosextens.h>

#include <stddef.h>

int __nocommandline = 1;

int main(void)
{
    const unsigned int dl_offset = offsetof(struct DeviceList, dl_Name);
    const unsigned int di_offset = offsetof(struct DevInfo, dvi_Name);
    const unsigned int gen_offset = offsetof(struct DosList, dol_Name);

    if ((dl_offset != di_offset) || (dl_offset != gen_offset))
    {
    	bug("Broken DosList alignment.\n");
    	bug("DosList.dol_Name  : %u bytes\n", gen_offset);
    	bug("DeviceList.dl_Name: %u bytes\n", dl_offset);
    	bug("DevInfo.dvi_Name  : %u bytes\n", di_offset);
    	
    	return 20;
    }
    else
    	return 0;
}

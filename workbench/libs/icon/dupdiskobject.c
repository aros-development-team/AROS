/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <graphics/gfx.h>
#include <proto/arossupport.h>
#include "icon_intern.h"

/*#define OUTPUT_DATA*/

struct OldDrawerData
{
    struct NewWindow dd_NewWindow;
    LONG             dd_CurrentX;
    LONG             dd_CurrentY;
};


STATIC STRPTR StrDupPooled(APTR pool, STRPTR str)
{
    char *newstr;
    if (!str) return NULL;
    newstr = AllocPooled(pool,strlen(str)+1);
    if (newstr) strcpy(newstr,str);
    return newstr;
}

STATIC struct Image *ImageDupPooled(APTR pool, struct Image *src)
{
    struct Image *dest;

    if (!src) return NULL;

    dest = (struct Image*)AllocPooled(pool,sizeof(struct Image));
    if (dest)
    {
	int data_size = 0;
	int plane_size;
	int i;
	int plane_pick = src->PlanePick;

	*dest = *src;
	dest->NextImage = NULL;

	/* Calc the size all used planes */
	plane_size = RASSIZE(src->Width,src->Height);
	for (i=0;i<8;i++)
	{
	    if (plane_pick & 1) data_size += plane_size;
	    plane_pick >>= 1;
	}

	if ((dest->ImageData = AllocPooled(pool,data_size)))
	{
	    memcpy(dest->ImageData,src->ImageData,data_size);
#ifdef OUTPUT_DATA
	    kprintf("plane_pick = %ld\n",src->PlanePick);
	    kprintf("width = %ld\n",src->Width);
	    kprintf("height = %ld\n",src->Height);

	    for (i=0;i<data_size;i++)
	    {
		kprintf("0x%02lx,",((unsigned char*)src->ImageData)[i]);
		if (i%16 == 15) kprintf("\n");
	    }
	     kprintf("\n");
#endif
	    return dest;
	}

	/* Something failed so we fail also */
	FreePooled(pool,dest,sizeof(struct Image));
    }
    return NULL;
}

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>

	AROS_LH2(struct DiskObject *, DupDiskObjectA,

/*  SYNOPSIS */
	AROS_LHA(struct DiskObject *, icon, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct Library *, IconBase, 25, Icon)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)

    APTR pool;
    void **mem;
    struct DiskObject *dob;

    if (!icon) return NULL;

    pool = CreatePool(0,1024,1024);
    if (!pool) return NULL;

    /* AROS doesn't need the gfx to be placed in chip memory, so we can use pools */
    mem = (void**)AllocPooled(pool, sizeof(struct DiskObject) + sizeof(void*));
    if (!mem)
    {
	DeletePool(pool);
	return NULL;
    }
    mem[0] = pool;
    dob = (struct DiskObject*)(mem + 1);

    /* copy the contents */
    *dob = *icon;
#ifdef OUTPUT_DATA
    kprintf("gadgetwidth = %ld\ngadgetheight = %ld\n",dob->do_Gadget.Width,dob->do_Gadget.Height);
#endif

    /* and now the pointers and the rest */
#warning TODO: check for errors here
    dob->do_DefaultTool = StrDupPooled(pool,icon->do_DefaultTool);
    dob->do_ToolWindow = StrDupPooled(pool,icon->do_ToolWindow);

    if (icon->do_DrawerData)
    {
	if ((dob->do_DrawerData = AllocPooled(pool,sizeof(struct DrawerData))))
 	{
#warning FIXME: we copy only sizeof(OldDrawerData) here
	    memset(dob->do_DrawerData,0,sizeof(struct DrawerData));
	    memcpy(dob->do_DrawerData,icon->do_DrawerData,sizeof(struct OldDrawerData));
        }
    }

    /* Duplicate the image data */
    dob->do_Gadget.GadgetRender = ImageDupPooled(pool,(struct Image*)icon->do_Gadget.GadgetRender);
    dob->do_Gadget.SelectRender = ImageDupPooled(pool,(struct Image*)icon->do_Gadget.SelectRender);

    /* Duplicate the tool types */
    if (icon->do_ToolTypes)
    {
	int num_tts;
	/* Get number of tool types */
	for (num_tts = 0;icon->do_ToolTypes[num_tts];num_tts++);

	if ((dob->do_ToolTypes = (char**)AllocPooled(pool,sizeof(char*)*(num_tts+1))))
	{
	    int i;
	    for (i=0;i<num_tts;i++)
		dob->do_ToolTypes[i] = StrDupPooled(pool,icon->do_ToolTypes[i]);
	    dob->do_ToolTypes[i] = NULL;
	}
    }

    return dob;

    AROS_LIBFUNC_EXIT
} /* FreeDiskObject */

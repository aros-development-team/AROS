/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Sync info class
    Lang: English.
*/

/****************************************************************************************/

#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <graphics/gfxnodes.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <hidd/graphics.h>

#define DEBUG 0
#include <aros/debug.h>

#include <stdio.h>

#include "graphics_intern.h"

#undef csd
#define GfxBase csd->cs_GfxBase

/****************************************************************************************/

static BOOL parse_sync_tags(struct class_static_data *csd, struct sync_data *data, struct TagItem *tags, BOOL init);

/****************************************************************************************/

OOP_Object *Sync__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct class_static_data *csd = CSD(cl);
    struct Library *UtilityBase = csd->cs_UtilityBase;
    struct Library *OOPBase = csd->cs_OOPBase;
    BOOL    	    	ok = TRUE;

    EnterFunc(bug("Sync::New()\n"));

    /*
     * We need graphics.library in order to be able to create MonitorSpec.
     * we do it here because graphics.hidd is initialized before
     * graphics.library
     */
    ObtainSemaphore(&csd->sema);

    if (!GfxBase)
    {
        GfxBase = (void *)OpenLibrary("graphics.library", 41);
        if (!GfxBase)
	    ok = FALSE;
    }
    ReleaseSemaphore(&csd->sema);

    if (!ok)
        return NULL;

    /* Get object from superclass */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
    	struct sync_data *data = OOP_INST_DATA(cl, o);
    	struct TagItem *tstate = msg->attrList;
    	char *s = NULL;
    	ULONG board = 0;
    	struct TagItem *tag;

	/* Parse mandatory attributes */
	while ((tag = NextTagItem(&tstate)))
    	{
            ULONG idx;

	    if (IS_SYNC_ATTR(tag->ti_Tag, idx))
	    {
	    	switch (idx)
	    	{
	    	case aoHidd_Sync_HDisp:
	    	    data->hdisp = tag->ti_Data;
	    	    break;

	    	case aoHidd_Sync_VDisp:
	    	    data->vdisp = tag->ti_Data;
	    	    break;

		case aoHidd_Sync_Flags:
		    data->flags = tag->ti_Data;
		    break;

		case aoHidd_Sync_Description:
		    s = (char *)tag->ti_Data;
		    break;

		case aoHidd_Sync_BoardNumber:
		    board = tag->ti_Data;
		    break;

		case aoHidd_Sync_Variable:
	    	    if (tag->ti_Data)
	    	    	data->InternalFlags |= SYNC_VARIABLE;
		    break;

	    	case aoHidd_Sync_MonitorSpec:
	    	    data->mspc = (struct MonitorSpec *)tag->ti_Data;
	    	    break;

	    	case aoHidd_Sync_GfxHidd:
	    	    data->gfxhidd = (OOP_Object *)tag->ti_Data;
	    	    break;
	    	}
	    }
    	}

	/* We must have HDisp, VDisp and GfxHidd */
        if ((!data->hdisp) || (!data->vdisp) || (!data->gfxhidd))
            ok = FALSE;

    	if (ok && (!data->mspc))
    	{
	    /*
	     * We must have a MonitorSpec. Either it's pre-cooked by the driver
	     * (useful for Amiga(tm) chipset), or we create it ourselves
	     */
	    data->mspc = (struct MonitorSpec *)GfxNew(MONITOR_SPEC_TYPE);
	    if (data->mspc)
	    {
	    	data->mspc->ms_Node.xln_Name = data->description;
	    	data->mspc->ratioh = RATIO_UNITY;
	    	data->mspc->ratiov = RATIO_UNITY;
	    	InitSemaphore(&data->mspc->DisplayInfoDataBaseSemaphore);

	    	data->InternalFlags |= SYNC_FREE_MONITORSPEC;
	    }
	    else
	        ok = FALSE;
	}

	if (ok)
	{
	    /* By default minimum/maximum bitmap size is equal to display size */
	    data->hmin = GetTagData(aHidd_Sync_HMin, data->hdisp, msg->attrList);
	    data->hmax = GetTagData(aHidd_Sync_HMax, data->hdisp, msg->attrList);
	    data->vmin = GetTagData(aHidd_Sync_VMin, data->vdisp, msg->attrList);
	    data->vmax = GetTagData(aHidd_Sync_VMax, data->vdisp, msg->attrList);

	    /* Format description */
	    if (s)
	    {
		char *d = data->description;
	    	int dlen = sizeof(data->description);
	    	char c;
	    	int l;

	    	for (;;)
	    	{
	            c = *s++;
	            if (c == '%')
	            {
		    	/* It's a format prefix, let's deal with it */
		    	c = *s++;
		    	switch (c)
		        {
		    	case 'b':
		            l = snprintf(d, dlen, "%u", (unsigned)board);
		            break;

		    	case 'h':
		            l = snprintf(d, dlen, "%u", (unsigned)data->hdisp);
			    break;

		    	case 'v':
		            l = snprintf(d, dlen, "%u", (unsigned)data->vdisp);
			    break;

		    	default:
			    /* Just copy over two chars */
		            d[0] = '%';
			    l = 1;
			    /* Copy next character only if we have room for it */
			    if (dlen > 2)
			    {
			    	d[1] = c;
			    	l++;
			    }
			    break;
		    	}
		    }
		    else
		    {
		    	/* Copy one character */
		    	*d = c;
		    	l = 1;
		    }

		    /* If NULL byte has been just transferred, exit, the string is already terminated */
		    if (!c)
		    	break;

		    /* Increment pointer, decrement length */
		    d    += l;
		    dlen -= l;

		    /* If we have only one byte in the destination left, terminate the string and exit */
		    if (dlen < 2)
		    {
		    	*d = 0;
		    	break;
		    }
		}
	    } /* if (s) */

	    ok = parse_sync_tags(CSD(cl), data, msg->attrList, TRUE);
	} /* if (ok) */

	if (ok)
	{
	    /* Set object pointer and add the MonitorSpec to the GfxBase->MonitorList */
	    data->mspc->ms_Object = (void *)o;

	    ObtainSemaphore(GfxBase->MonitorListSemaphore);
	    Enqueue(&GfxBase->MonitorList, (struct Node*)data->mspc);
	    ReleaseSemaphore(GfxBase->MonitorListSemaphore);

	    return o;
	}
	else
    	{
	    OOP_MethodID dispose_mid;

	    D(bug("!!! ERROR PARSING SYNC ATTRS IN Sync::New() !!!\n"));

	    dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    OOP_CoerceMethod(cl, o, &dispose_mid);
	}
    }

    return NULL;
}

/****************************************************************************************/

VOID Sync__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct sync_data *data = OOP_INST_DATA(cl, o);
    struct class_static_data *csd = CSD(cl);

    /* First we remove the MonitorSpec from the list, if it's our MonitorSpec
       and it's in the list */
    if ((data->InternalFlags & SYNC_FREE_MONITORSPEC) &&
        data->mspc->ms_Node.xln_Succ) {

	ObtainSemaphore(GfxBase->MonitorListSemaphore);
        Remove((struct Node *)data->mspc);
	ReleaseSemaphore(GfxBase->MonitorListSemaphore);
    }

    /* Then we dispose things that we created */
    if (data->InternalFlags & SYNC_FREE_SPECIALMONITOR)
        GfxFree(&data->mspc->ms_Special->spm_Node);
    if (data->InternalFlags & SYNC_FREE_MONITORSPEC)
        GfxFree(&data->mspc->ms_Node);

    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

VOID Sync__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct sync_data 	*data;
    struct class_static_data *csd;
    ULONG   	    	idx;

    data = OOP_INST_DATA(cl, o);
    csd = CSD(cl);

    if (IS_SYNC_ATTR(msg->attrID, idx))
    {
        UWORD hsync_start, hsync_end, vsync_start, vsync_end;

	if (data->mspc->ms_Special) {
	    hsync_start = data->mspc->ms_Special->hsync.asi_Start;
	    hsync_end   = data->mspc->ms_Special->hsync.asi_Stop;
	    vsync_start = data->mspc->ms_Special->vsync.asi_Start;
	    vsync_end   = data->mspc->ms_Special->vsync.asi_Stop;
	} else {
	    /* Special failback values that will result in zero margins and sync lengths */
	    hsync_start = data->hdisp;
	    hsync_end   = data->hdisp;
	    vsync_start = data->vdisp;
	    vsync_end   = data->vdisp;
	}

    	switch (idx)
	{
	    case aoHidd_Sync_PixelTime:
	        if (data->pixelclock) {
		    ULONG khz = data->pixelclock / 1000;

		    *msg->storage = 1000000000 / khz;
	        } else
		    *msg->storage = 0;

	    case aoHidd_Sync_PixelClock:
	        *msg->storage = data->pixelclock;
		break;

	    case aoHidd_Sync_LeftMargin:
		*msg->storage = data->htotal - hsync_end;
		break;

	    case aoHidd_Sync_RightMargin:
		*msg->storage = hsync_start - data->hdisp;
		break;

	    case aoHidd_Sync_HSyncLength:
		*msg->storage = hsync_end - hsync_start;
		break;

	    case aoHidd_Sync_UpperMargin:
		*msg->storage = data->mspc->total_rows - vsync_end;
		break;

	    case aoHidd_Sync_LowerMargin:
		*msg->storage = vsync_end - data->vdisp;
		break;

	    case aoHidd_Sync_VSyncLength:
		*msg->storage = vsync_end - vsync_start;
		break;

	    case aoHidd_Sync_HDisp:
		*msg->storage = data->hdisp;
		break;

	    case aoHidd_Sync_VDisp:
		*msg->storage = data->vdisp;
		break;

	    case aoHidd_Sync_HSyncStart:
		*msg->storage = hsync_start;
		break;

	    case aoHidd_Sync_HSyncEnd:
		*msg->storage = hsync_end;
		break;

	    case aoHidd_Sync_HTotal:
		*msg->storage = data->htotal;
		break;

	    case aoHidd_Sync_VSyncStart:
		*msg->storage = vsync_start;
		break;

	    case aoHidd_Sync_VSyncEnd:
		*msg->storage = vsync_end;
		break;

	    case aoHidd_Sync_VTotal:
		*msg->storage = data->mspc->total_rows;
		break;

	    case aoHidd_Sync_Description:
	    	*msg->storage = (IPTR)data->description;
		break;

	    case aoHidd_Sync_HMin:
		*msg->storage = data->hmin;
		break;

	    case aoHidd_Sync_HMax:
		*msg->storage = data->hmax;
		break;

	    case aoHidd_Sync_VMin:
		*msg->storage = data->vmin;
		break;

	    case aoHidd_Sync_VMax:
		*msg->storage = data->vmax;
		break;

	    case aoHidd_Sync_Flags:
	        *msg->storage = data->flags;
		break;

	    case aoHidd_Sync_Variable:
	        *msg->storage = (data->InternalFlags & SYNC_VARIABLE) ? TRUE : FALSE;
	        break;

	    case aoHidd_Sync_MonitorSpec:
	        *msg->storage = (IPTR)data->mspc;
		break;

	    case aoHidd_Sync_GfxHidd:
		*msg->storage = (IPTR)data->gfxhidd;
		break;

	    default:
	     	D(bug("!!! TRYING TO GET UNKNOWN ATTR FROM SYNC OBJECT !!!\n"));
    		OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		break;

	}
    
    }
    else
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

void Sync__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct sync_data *data = OOP_INST_DATA(cl, o);

    /* Set actually works only if the object is variable */
    if (data->InternalFlags & SYNC_VARIABLE)
    {
    	struct class_static_data *csd = CSD(cl);
    	BOOL notify_driver = parse_sync_tags(csd, data, msg->attrList, FALSE);
    
    	if (notify_driver)
	    HIDD_Gfx_SetMode(data->gfxhidd, (OOP_Object *)data->mspc->ms_Object);
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/****************************************************************************************/

/*
 * A backwards compatibility callback that recalculates new htotal value from
 * MonitorSpec->total_colorclocks.
 * Future sync editor for AROS will not have to use this API. Instead it is
 * suggested to use OOP API on a sync object, obtained via VecInfo. It is
 * more straightforward and flexible.
 */

static LONG do_monitor(struct MonitorSpec *mspc)
{
    /* Extract sync object data from the MonitorSpec */
    OOP_Object *obj        = (OOP_Object *)mspc->ms_Object;
    OOP_Class *cl          = OOP_OCLASS(obj);
    struct sync_data *data = OOP_INST_DATA(cl, obj);
    struct class_static_data *csd = CSD(cl);

    data->htotal = 100000000 / mspc->total_colorclocks / 28 / data->pixelclock;
    HIDD_Gfx_SetMode(data->gfxhidd, obj);

    return 0;
}

/****************************************************************************************/

/*
 * Parses the tags supplied in 'tags' and puts the result into 'data'.
 * It also checks to see if all needed attrs are supplied.
 * Return value is treated as:
 * - Creation time (init = TRUE) - success/failure flag.
 * - Set method (init = FALSE) - TRUE if something changed and display driver needs to be notified.
 */
static BOOL parse_sync_tags(struct class_static_data *csd, struct sync_data *data, struct TagItem *tags, BOOL init)
{
    struct Library *UtilityBase = csd->cs_UtilityBase;
    UWORD hsync_start     = 0;
    UWORD vsync_start     = 0;
    UWORD hsync_end       = data->hdisp;
    UWORD vsync_end       = data->vdisp;
    BOOL have_hsync_start = FALSE;
    BOOL have_hsync_end   = FALSE;
    BOOL have_vsync_start = FALSE;
    BOOL have_vsync_end   = FALSE;
    BOOL have_htotal      = FALSE;
    BOOL have_vtotal      = FALSE;
    BOOL change_totclk    = init;
    BOOL notify_driver    = FALSE;
    struct TagItem *tag, *tstate = tags;

    /*
     * Parse sync signal parameters. They may come either as start, stop and total
     * values (which most of drivers use), or as LinuxFB-style specification (margins and
     * sync length.
     * The latter specification is deprecated since no drivers except LinuxFB (which is
     * broken anyway) use it.
     */
    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;

	if (IS_SYNC_ATTR(tag->ti_Tag, idx))
	{
	    switch (idx)
	    {
	    case aoHidd_Sync_PixelClock:
	        data->pixelclock = tag->ti_Data;
	        change_totclk = TRUE;
	    	break;

	    case aoHidd_Sync_PixelTime:
		/*
		 * According to the HOWTO, PixelTime is one million divided by pixelclock in mHz.
	   	 * Pixelclock is not always a multiple of 1 mHz, but it seems to always be a multiple
	   	 * of 1 kHz. We rely on this fact in order to be able to calculate everything in integers.
	   	 * Anyway, this attribute is deprecated, don't use it.
	   	 * I intentionally don't simplify this expression in order to make it clear. Let's leave
	   	 * it to the compiler - sonic.
	   	 */
		data->pixelclock = (1000000000 / tag->ti_Data) * 1000;
		change_totclk = TRUE;
		break;

	    case aoHidd_Sync_HSyncStart:
	    	hsync_start = tag->ti_Data;
	    	have_hsync_start = TRUE;
	    	break;

	    case aoHidd_Sync_HSyncEnd:
	    	hsync_end = tag->ti_Data;
	    	have_hsync_end = TRUE;
	    	break;

	    case aoHidd_Sync_VSyncStart:
	    	vsync_start = tag->ti_Data;
	    	have_vsync_start = TRUE;
	    	break;

	    case aoHidd_Sync_VSyncEnd:
	    	vsync_end = tag->ti_Data;
	    	have_vsync_end = TRUE;
	    	break;

	    case aoHidd_Sync_HTotal:
		data->htotal = tag->ti_Data;
		have_htotal   = TRUE;
		change_totclk = TRUE;
		break;

	    case aoHidd_Sync_VTotal:
	        data->mspc->total_rows = tag->ti_Data;
	        have_vtotal   = TRUE;
		notify_driver = TRUE;
		break;
	    }
	}
    }

    D(bug("[sync] PixelClock is set to %u\n", data->pixelclock));

    /*
     * Old LFB-style specification needs to be processed in a particular order,
     * so we do it here, if needed.
     */
    if (!have_hsync_start)
    {
    	tag = FindTagItem(aHidd_Sync_RightMargin, tags);
    	if (tag)
    	{
	    hsync_start = data->hdisp + tag->ti_Data;
	    have_hsync_start = TRUE;
	}
    }

    if (!have_hsync_end)
    {
    	tag = FindTagItem(aHidd_Sync_HSyncLength, tags);
    	if (tag)
    	{
	    hsync_end = hsync_start + tag->ti_Data;
            have_hsync_end = TRUE;
        }
    }

    if (!have_vsync_start)
    {
    	tag = FindTagItem(aHidd_Sync_LowerMargin, tags);
    	if (tag)
    	{
	    vsync_start = data->vdisp + tag->ti_Data;
	    have_vsync_start = TRUE;
	}
    }

    if (!have_vsync_end)
    {
    	tag = FindTagItem(aHidd_Sync_VSyncLength, tags);
    	if (tag)
    	{
	    vsync_end = vsync_start + tag->ti_Data;
	    have_vsync_end = TRUE;
	}
    }

    if (have_hsync_start || have_hsync_end || have_vsync_start || have_vsync_end)
    {
    	/* Sync data changed */
	if (init)
	{
	    /* During object creation this means we need to attach SpecialMonitor to our MonitorSpec. */
	    if (!data->mspc->ms_Special)
	    {
		data->mspc->ms_Special = (struct SpecialMonitor *)GfxNew(SPECIAL_MONITOR_TYPE);
		if (!data->mspc->ms_Special)
		    return FALSE;

		if (data->InternalFlags & SYNC_VARIABLE)
		    data->mspc->ms_Special->do_monitor = do_monitor;

		data->mspc->ms_Flags |= MSF_REQUEST_SPECIAL;
		data->InternalFlags  |= SYNC_FREE_SPECIALMONITOR;
	    }
	}
	/* Notification is needed */
	notify_driver = TRUE;
    }

    if (data->mspc->ms_Special)
    {
        if (have_hsync_start)
	    data->mspc->ms_Special->hsync.asi_Start = hsync_start;
	if (have_hsync_end)
	    data->mspc->ms_Special->hsync.asi_Stop  = hsync_end;
	if (have_vsync_start)
	    data->mspc->ms_Special->vsync.asi_Start = vsync_start;
	if (have_vsync_end)
	    data->mspc->ms_Special->vsync.asi_Stop  = vsync_end;
    }

    if (!have_htotal)
    {
        UWORD left_margin = 0;

    	tag = FindTagItem(aHidd_Sync_LeftMargin, tags);
    	if (tag)
    	{
    	    left_margin = tag->ti_Data;
	    change_totclk = TRUE;
	}
        /*
         * If we have neither HTotal nor LeftMargin, htotal will be equal to hsync_end here.
	 * Previously, hsync_end gets equal to hdisp if no horizontal sync data was specified.
         * This is done for poor man's drivers which can't provide complete sync information
	 * (like hosted drivers, especially SDL). In this case total = disp, it's better than
	 * nothing. The same is done below with vtotal.
	 */
        data->htotal = hsync_end + left_margin;
    }

    if (!have_vtotal)
    {
    	UWORD upper_margin = 0;

    	tag = FindTagItem(aHidd_Sync_UpperMargin, tags);
    	if (tag)
    	{
    	    upper_margin = tag->ti_Data;
    	    notify_driver = TRUE;
    	}
	data->mspc->total_rows = vsync_end + upper_margin;
    }

    if (change_totclk)
    {
        if (data->pixelclock)
            data->mspc->total_colorclocks = 100000000 / (data->pixelclock / data->htotal * 28);
	else
	{
	    /*
	     * Another kludge for drivers without sync data. Amiga software never expects
	     * to get zero in total_colorclocks, so we have to fill it in with something.
	     * This value will have totally nothing to do with real display refresh rate,
	     * but we can do nothing with it
	     */
	    data->mspc->total_colorclocks = VGA_COLORCLOCKS;
	}
	/* change_totclk always implies notify_driver, for code simplicity */
	notify_driver = TRUE;
    }

    return notify_driver;
}

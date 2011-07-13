/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
#define GfxBase csd->GfxBase

/****************************************************************************************/

static BOOL parse_sync_tags(OOP_Class *cl, OOP_Object *o, struct TagItem *tags, BOOL init);

/****************************************************************************************/

OOP_Object *Sync__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct class_static_data *csd = CSD(cl);
    BOOL    	    	ok = TRUE;

    EnterFunc(bug("Sync::New()\n"));

    /* We need graphics.library in order to be able to create MonitorSpec.
       we do it here because graphics.hidd is initialized before
       graphics.library */
    ObtainSemaphore(&csd->sema);
    if (!GfxBase) {
        GfxBase = (void *)OpenLibrary("graphics.library", 41);
        if (!GfxBase)
	    ok = FALSE;
    }
    ReleaseSemaphore(&csd->sema);

    if (!ok)
        return NULL;

    /* Get object from superclass */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
	return NULL;

    ok = parse_sync_tags(cl, o, msg->attrList, TRUE);
    if (!ok) {
	OOP_MethodID dispose_mid;

	D(bug("!!! ERROR PARSING SYNC ATTRS IN Sync::New() !!!\n"));
	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
	o = NULL;
    }

    return o;
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
    parse_sync_tags(cl, o, msg->attrList, FALSE);

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg); 
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
    struct sync_data *data = (struct sync_data *)mspc->ms_Special->reserved1;

    data->htotal = 100000000 / mspc->total_colorclocks / 28 / data->pixelclock;

    if (data->gfxhidd)
        HIDD_Gfx_SetMode(data->gfxhidd, (OOP_Object *)mspc->ms_Object);

    return 0;
}

/****************************************************************************************/

/*
    Parses the tags supplied in 'tags' and puts the result into 'data'.
    It also checks to see if all needed attrs are supplied.
*/

#define SYAO(x) (aoHidd_Sync_ ## x)

static BOOL parse_sync_tags(OOP_Class *cl, OOP_Object *o, struct TagItem *tags, BOOL init)
{
    DECLARE_ATTRCHECK(sync);
    IPTR attrs[num_Hidd_Sync_Attrs] = {0};
    BOOL ok = TRUE;
    UWORD hsync_start = 0;
    UWORD vsync_start = 0;
    UWORD hsync_end, vsync_end;
    BOOL have_hsync_start, have_hsync_end, have_vsync_start, have_vsync_end;
    BOOL change_totclk = init;
    BOOL notify_driver = FALSE;
    struct sync_data *data = OOP_INST_DATA(cl, o);
    struct class_static_data *csd = CSD(cl);

    if (0 != OOP_ParseAttrs(tags, attrs, num_Hidd_Sync_Attrs, &ATTRCHECK(sync), csd->hiddSyncAttrBase))
    {
	D(bug("!!! parse_sync_tags: ERROR PARSING ATTRS !!!\n"));
	return FALSE;
    }

    if (init) {
    	/* The following can be processed only during init */

	/* We must have a MonitorSpec. Either it's pre-cooked by the driver (useful for Amiga(tm)
	   chipset), or we create it ourselves */
	if (GOT_SYNC_ATTR(MonitorSpec))
            data->mspc = (struct MonitorSpec *)attrs[SYAO(MonitorSpec)];
	else {
	    data->mspc = (struct MonitorSpec *)GfxNew(MONITOR_SPEC_TYPE);
	    if (!data->mspc)
	        return FALSE;
	    data->mspc->ms_Node.xln_Name = data->description;
	    InitSemaphore(&data->mspc->DisplayInfoDataBaseSemaphore);
	    data->mspc->ms_Object = (void *)o;

	    data->InternalFlags |= SYNC_FREE_MONITORSPEC;
	}


	/* During init we must get HDisp and VDisp, so we set the
           returncode to FALSE if we don't get them */
	if (GOT_SYNC_ATTR(HDisp))
	    data->hdisp = attrs[SYAO(HDisp)];
	else
            ok = FALSE;

	if (GOT_SYNC_ATTR(VDisp))
	    data->vdisp = attrs[SYAO(VDisp)];
	else
	    ok = FALSE;
	
	if (GOT_SYNC_ATTR(GfxHidd))
	    data->gfxhidd = (OOP_Object *)attrs[SYAO(GfxHidd)];
	else
	    ok = FALSE;

	/* By default minimum/maximum bitmap size is equal to display size */
	if (GOT_SYNC_ATTR(HMin))
            data->hmin = attrs[SYAO(HMin)];
	else
            data->hmin = data->hdisp;
	if (GOT_SYNC_ATTR(HMax))
	    data->hmax = attrs[SYAO(HMax)];
	else
	    data->hmax = data->hdisp;
	if (GOT_SYNC_ATTR(VMin))
            data->vmin = attrs[SYAO(VMin)];
	else
            data->vmin = data->vdisp;
	if (GOT_SYNC_ATTR(VMax))
	    data->vmax = attrs[SYAO(VMax)];
	else
	    data->vmax = data->vdisp;

	data->flags = attrs[SYAO(Flags)];

	if (attrs[SYAO(Variable)])
	    data->InternalFlags |= SYNC_VARIABLE;

	if (GOT_SYNC_ATTR(Description)) {
	    char *s = (char *)attrs[SYAO(Description)];
	    char *d = data->description;
	    int dlen = sizeof(data->description);
	    char c;
	    int l;

	    for (;;) {
	        c = *s++;
	        if (c == '%') {
		    /* It's a format prefix, let's deal with it */
		    c = *s++;
		    switch(c) {
		    case 'b':
		        l = snprintf(d, dlen, "%lu", attrs[SYAO(BoardNumber)]);
		        break;
		    case 'h':
		        l = snprintf(d, dlen, "%lu", (unsigned long)data->hdisp);
			break;
		    case 'v':
		        l = snprintf(d, dlen, "%lu", (unsigned long)data->vdisp);
			break;
		    default:
			/* Just copy over two chars */
		        d[0] = '%';
			l = 1;
			/* Copy next character only if we have room for it */
			if (dlen > 2) {
			    d[1] = c;
			    l++;
			}
			break;
		    }
		} else {
		    /* Copy one character */
		    *d = c;
		    l = 1;
		}

		/* If NULL byte has been just transferred, exit, the string is already terminated */
		if (!c)
		    break;

		/* Increment pointer, decrement length */
		d += l;
		dlen -= l;

		/* If we have only one byte in the destination left,
		   terminate the string and exit */
		if (dlen < 2) {
		    *d = 0;
		    break;
		}
	    }
	}

    } else if (!(data->InternalFlags & SYNC_VARIABLE))
        /* During set, we don't process further data if the object
	   is marked as non-variable */
        return ok;

    /* Now parse sync signal parameters. They may come either as start, stop and total
       values (which most of drivers use), or as LinuxFB-style specification (margins and
       sync length.
       The latter specification is deprecated since no drivers except LinuxFB (which is
       broken anyway) use it. */
    if (GOT_SYNC_ATTR(PixelClock)) {
	data->pixelclock = attrs[SYAO(PixelClock)];
	change_totclk = TRUE;
    } else if (GOT_SYNC_ATTR(PixelTime)) {
	/* According to the HOWTO, PixelTime is one million divided by pixelclock in mHz.
	   Pixelclock is not always a multiple of 1 mHz, but it seems to always be a multiple
	   of 1 kHz. We rely on this fact in order to be able to calculate everything in integers.
	   Anyway, this attribute is deprecated, don't use it. */
        ULONG khz = 1000000000 / attrs[SYAO(PixelTime)];

	data->pixelclock = khz * 1000;
	change_totclk = TRUE;
    }
    D(bug("[sync] PixelClock is set to %u\n", data->pixelclock));

    /* Process sync start/stop */
    have_hsync_start = GOT_SYNC_ATTR(HSyncStart);
    if (have_hsync_start) {
        hsync_start = attrs[SYAO(HSyncStart)];
    } else if (GOT_SYNC_ATTR(RightMargin)) {
	hsync_start = data->hdisp + attrs[SYAO(RightMargin)];
	have_hsync_start = TRUE;
    }

    have_hsync_end = GOT_SYNC_ATTR(HSyncEnd);
    if (have_hsync_end)
	hsync_end = attrs[SYAO(HSyncEnd)];
    else if (GOT_SYNC_ATTR(HSyncLength)) {
	hsync_end = hsync_start + attrs[SYAO(HSyncLength)];
        have_hsync_end = TRUE;
    } else
        hsync_end = data->hdisp;

    have_vsync_start = GOT_SYNC_ATTR(VSyncStart);
    if (have_vsync_start)
	vsync_start = attrs[SYAO(VSyncStart)];
    else if (GOT_SYNC_ATTR(LowerMargin)) {
	vsync_start = data->vdisp + attrs[SYAO(LowerMargin)];
        have_vsync_start = TRUE;
    }

    have_vsync_end = GOT_SYNC_ATTR(VSyncEnd);
    if (have_vsync_end)
	vsync_end = attrs[SYAO(VSyncEnd)];
    else if (GOT_SYNC_ATTR(VSyncLength)) {
	vsync_end = vsync_start + attrs[SYAO(VSyncLength)];
        have_vsync_end = TRUE;
    } else
        vsync_end = data->vdisp;

    if (have_hsync_start || have_hsync_end || have_vsync_start || have_vsync_end) {
	if (init) {
	    if (!data->mspc->ms_Special) {
		data->mspc->ms_Special = (struct SpecialMonitor *)GfxNew(SPECIAL_MONITOR_TYPE);
		if (data->mspc->ms_Special) {
		    if (data->InternalFlags & SYNC_VARIABLE) {
			data->mspc->ms_Special->do_monitor = do_monitor;
			data->mspc->ms_Special->reserved1 = (LONG (*)(void))data;
		    }
		    data->mspc->ms_Flags |= MSF_REQUEST_SPECIAL;
		    data->InternalFlags |= SYNC_FREE_SPECIALMONITOR;
		} else
		    ok = FALSE;
	    }
	}
	notify_driver = TRUE;
    }

    if (data->mspc->ms_Special) {
        if (have_hsync_start)
	    data->mspc->ms_Special->hsync.asi_Start = hsync_start;
	if (have_hsync_end)
	    data->mspc->ms_Special->hsync.asi_Stop  = hsync_end;
	if (have_vsync_start)
	    data->mspc->ms_Special->vsync.asi_Start = vsync_start;
	if (have_vsync_end)
	    data->mspc->ms_Special->vsync.asi_Stop  = vsync_end;
    }

    if (GOT_SYNC_ATTR(HTotal)) {
	data->htotal = attrs[SYAO(HTotal)];
	change_totclk = TRUE;
    } else {
        if (GOT_SYNC_ATTR(LeftMargin))
	    change_totclk = TRUE;
        /* If we have neither HTotal nor LeftMargin, htotal will be equal to hsync_end here.
	   Previously, hsync_end gets equal to hdisp if no horizontal sync data was specified.
           This is done for poor man's drivers which can't provide complete sync information
	   (like hosted drivers, especially SDL). In this case total = disp, it's better than
	   nothing. The same is done below with vtotal. */
        data->htotal = hsync_end + attrs[SYAO(LeftMargin)];
    }

    if (GOT_SYNC_ATTR(VTotal)) {
	data->mspc->total_rows = attrs[SYAO(VTotal)];
	notify_driver = TRUE;
    } else {
	data->mspc->total_rows = vsync_end + attrs[SYAO(UpperMargin)];
	if (GOT_SYNC_ATTR(UpperMargin))
	    notify_driver = TRUE;
    }

    if (change_totclk) {
        if (data->pixelclock)
            data->mspc->total_colorclocks = 100000000 / (data->pixelclock / data->htotal * 28);
	else
	    /* Another kludge for drivers without sync data. Amiga software never expects
	       to get zero in total_colorclocks, so we have to fill it in with something.
	       This value will have totally nothing to do with real display refresh rate,
	       but we can do nothing with it */
	    data->mspc->total_colorclocks = VGA_COLORCLOCKS;
	notify_driver = TRUE;
    }

    /* Post-processing after success */
    if (ok) {
        if (init) {
            if (data->InternalFlags & SYNC_FREE_MONITORSPEC) {
		ObtainSemaphore(GfxBase->MonitorListSemaphore);
		/* FIXME: use Enqueue() here after switch to ABI v1 */
		AddTail(&GfxBase->MonitorList, (struct Node*)data->mspc);
		ReleaseSemaphore(GfxBase->MonitorListSemaphore);
	    }
	} else if (notify_driver)
	    HIDD_Gfx_SetMode(data->gfxhidd, (OOP_Object *)data->mspc->ms_Object);
    }

    return ok;
}

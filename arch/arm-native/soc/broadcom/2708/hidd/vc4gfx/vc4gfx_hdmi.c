/*
    Copyright © 2013-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/mbox.h>
#include <stdio.h>

#include "vc4gfx_hidd.h"

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      xsd->vcsd_MBoxBase

int FNAME_SUPPORT(HDMI_SyncGen)(struct List *modelist, OOP_Class *cl)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    struct DisplayMode *hdmi_mode;
    int hdmi_modecount = 0;

    D(bug("[VideoCoreGfx] %s()\n", __PRETTY_FUNCTION__));

//#warning "TODO: check if anything is attached to HDMI/DVI"
    xsd->vcsd_MBoxMessage[0] = AROS_LONG2LE(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LONG2LE(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LONG2LE(VCTAG_GETRES);
    xsd->vcsd_MBoxMessage[3] = AROS_LONG2LE(8);
    xsd->vcsd_MBoxMessage[4] = 0;
    xsd->vcsd_MBoxMessage[5] = 0;
    xsd->vcsd_MBoxMessage[6] = 0;
    xsd->vcsd_MBoxMessage[7] = 0;		        // terminate tag

    /* for now lets just get the mode the display is currently running.. */
    MBoxWrite((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    if ((MBoxRead((void*)VCMB_BASE, VCMB_PROPCHAN) == xsd->vcsd_MBoxMessage) &&
        (xsd->vcsd_MBoxMessage[1] == AROS_LE2LONG(VCTAG_RESP)))
    {
        if ((hdmi_mode = AllocMem(sizeof(struct DisplayMode), MEMF_PUBLIC)) != NULL)
        {
            hdmi_mode->dm_clock = 25174;
            hdmi_mode->dm_hdisp = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
            hdmi_mode->dm_hstart = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
            hdmi_mode->dm_hend = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
            hdmi_mode->dm_htotal = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
            hdmi_mode->dm_vdisp = AROS_LE2LONG(xsd->vcsd_MBoxMessage[6]);
            hdmi_mode->dm_vstart = AROS_LE2LONG(xsd->vcsd_MBoxMessage[6]);
            hdmi_mode->dm_vend = AROS_LE2LONG(xsd->vcsd_MBoxMessage[6]);
            hdmi_mode->dm_vtotal = AROS_LE2LONG(xsd->vcsd_MBoxMessage[6]);
            hdmi_mode->dm_descr = AllocVec(256, MEMF_CLEAR);

            sprintf(hdmi_mode->dm_descr, "VideoCore: HDMI %dx%d", hdmi_mode->dm_hdisp, hdmi_mode->dm_vdisp);
            AddTail(modelist, &hdmi_mode->dm_Node);
            hdmi_modecount++;
        }
    }

    return hdmi_modecount;
}

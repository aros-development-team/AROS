/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/vcmbox.h>
#include <stdio.h> 

#include "videocoregfx_class.h"

#ifdef VCMBoxBase
#undef VCMBoxBase
#endif

#define VCMBoxBase      xsd->vcsd_VCMBoxBase

int FNAME_SUPPORT(HDMI_SyncGen)(struct List *modelist, OOP_Class *cl)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    struct DisplayMode *hdmi_mode;
    int hdmi_modecount = 0;

    D(bug("[VideoCoreGfx] %s()\n", __PRETTY_FUNCTION__));

#warning "TODO: check if anything is attached to HDMI/DVI"
    xsd->vcsd_VCMBoxMessage[0] = 8 * 4;
    xsd->vcsd_VCMBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_VCMBoxMessage[2] = VCTAG_GETRES;
    xsd->vcsd_VCMBoxMessage[3] = 8;
    xsd->vcsd_VCMBoxMessage[4] = 0;
    xsd->vcsd_VCMBoxMessage[5] = 0;
    xsd->vcsd_VCMBoxMessage[6] = 0;
    xsd->vcsd_VCMBoxMessage[7] = 0;		        // terminate tag

    /* for now lets just get the mode the display is currently running.. */
    VCMBoxWrite(VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_VCMBoxMessage);
    if ((VCMBoxRead(VCMB_BASE, VCMB_PROPCHAN) == xsd->vcsd_VCMBoxMessage) &&
        (xsd->vcsd_VCMBoxMessage[1] == VCTAG_RESP))
    {
        if ((hdmi_mode = AllocMem(sizeof(struct DisplayMode), MEMF_PUBLIC)) != NULL)
        {
            hdmi_mode->dm_clock = 25174;
            hdmi_mode->dm_hdisp = xsd->vcsd_VCMBoxMessage[5];
            hdmi_mode->dm_hstart = xsd->vcsd_VCMBoxMessage[5];
            hdmi_mode->dm_hend = xsd->vcsd_VCMBoxMessage[5];
            hdmi_mode->dm_htotal = xsd->vcsd_VCMBoxMessage[5];
            hdmi_mode->dm_vdisp = xsd->vcsd_VCMBoxMessage[6];
            hdmi_mode->dm_vstart = xsd->vcsd_VCMBoxMessage[6];
            hdmi_mode->dm_vend = xsd->vcsd_VCMBoxMessage[6];
            hdmi_mode->dm_vtotal = xsd->vcsd_VCMBoxMessage[6];
            hdmi_mode->dm_descr = AllocVec(256, MEMF_CLEAR);
            
            sprintf(hdmi_mode->dm_descr, "VideoCore: HDMI %dx%d", hdmi_mode->dm_hdisp, hdmi_mode->dm_vdisp);
            AddTail(modelist, &hdmi_mode->dm_Node);
            hdmi_modecount++;
        }
    }

    return hdmi_modecount;
}

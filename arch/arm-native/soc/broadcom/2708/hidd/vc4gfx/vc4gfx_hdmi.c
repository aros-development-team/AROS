/*
    Copyright (C) 2013-2017, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/mbox.h>
#include <stdio.h>

#include "vc4gfx_hidd.h"

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      xsd->vcsd_MBoxBase

/* VESA DMT timings for common modes (clocks in kHz). */
struct VC4ModeEntry
{
    ULONG width;
    ULONG height;
    ULONG clock;        /* pixel clock in kHz */
    ULONG hstart;
    ULONG hend;
    ULONG htotal;
    ULONG vstart;
    ULONG vend;
    ULONG vtotal;
};

static const struct VC4ModeEntry vc4_candidate_modes[] =
{
    {  640,  480,  25175,  656,  752,  800, 490, 492, 525 },
    {  800,  600,  40000,  840,  968, 1056, 601, 605, 628 },
    { 1024,  768,  65000, 1048, 1184, 1344, 771, 777, 806 },
    { 1152,  864,  81000, 1216, 1344, 1600, 865, 868, 900 },
    { 1280,  720,  74250, 1390, 1430, 1650, 725, 730, 750 },
    { 1280,  800,  83500, 1352, 1480, 1680, 803, 809, 831 },
    { 1280, 1024, 108000, 1328, 1440, 1688,1025,1028,1066 },
    { 1366,  768,  85500, 1436, 1579, 1792, 771, 774, 798 },
    { 1440,  900,  88750, 1488, 1520, 1600, 903, 909, 926 },
    { 1600,  900, 108000, 1624, 1704, 1800, 901, 904, 1000 },
    { 1680, 1050, 119000, 1728, 1760, 1840,1053,1059,1080 },
    { 1920, 1080, 148500, 2008, 2052, 2200,1084,1089,1125 },
    { 1920, 1200, 154000, 1968, 2000, 2080,1203,1209,1235 },
};

#define VC4_NUM_CANDIDATES (sizeof(vc4_candidate_modes) / sizeof(vc4_candidate_modes[0]))

static struct DisplayMode *VC4_BuildMode(const struct VC4ModeEntry *e)
{
    struct DisplayMode *m = AllocMem(sizeof(struct DisplayMode), MEMF_PUBLIC | MEMF_CLEAR);
    if (!m)
        return NULL;

    m->dm_clock  = e->clock;
    m->dm_hdisp  = e->width;
    m->dm_hstart = e->hstart;
    m->dm_hend   = e->hend;
    m->dm_htotal = e->htotal;
    m->dm_vdisp  = e->height;
    m->dm_vstart = e->vstart;
    m->dm_vend   = e->vend;
    m->dm_vtotal = e->vtotal;
    m->dm_descr  = AllocVec(64, MEMF_CLEAR);
    if (m->dm_descr)
        sprintf(m->dm_descr, "VideoCore: HDMI %dx%d", (int)e->width, (int)e->height);
    return m;
}

/* Probe a single resolution via VCTAG_TESTRES. The firmware returns the
 * (possibly snapped) width/height it would actually program.
 */
static BOOL VC4_TestRes(struct VideoCoreGfx_staticdata *xsd, ULONG w, ULONG h)
{
    BOOL ok;

    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LONG2LE(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LONG2LE(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LONG2LE(VCTAG_TESTRES);
    xsd->vcsd_MBoxMessage[3] = AROS_LONG2LE(8);
    xsd->vcsd_MBoxMessage[4] = AROS_LONG2LE(8);
    xsd->vcsd_MBoxMessage[5] = AROS_LONG2LE(w);
    xsd->vcsd_MBoxMessage[6] = AROS_LONG2LE(h);
    xsd->vcsd_MBoxMessage[7] = 0;

    ok = (MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
              != (volatile unsigned int *)-1)
         && (xsd->vcsd_MBoxMessage[1] == AROS_LE2LONG(VCTAG_RESP));
    VC4_MBOX_UNLOCK(xsd);
    return ok;
}

int FNAME_SUPPORT(HDMI_SyncGen)(struct List *modelist, OOP_Class *cl)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    struct DisplayMode *hdmi_mode;
    ULONG native_w = 0, native_h = 0;
    int hdmi_modecount = 0;
    int i;

    D(bug("[VideoCoreGfx] %s()\n", __PRETTY_FUNCTION__));

    /* Query the firmware for the currently-active display mode. This is the
     * resolution the HDMI link was negotiated at and serves as our default.
     */
    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LONG2LE(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LONG2LE(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LONG2LE(VCTAG_GETRES);
    xsd->vcsd_MBoxMessage[3] = AROS_LONG2LE(8);
    xsd->vcsd_MBoxMessage[4] = 0;
    xsd->vcsd_MBoxMessage[5] = 0;
    xsd->vcsd_MBoxMessage[6] = 0;
    xsd->vcsd_MBoxMessage[7] = 0;

    if ((MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
            != (volatile unsigned int *)-1) &&
        (xsd->vcsd_MBoxMessage[1] == AROS_LE2LONG(VCTAG_RESP)))
    {
        native_w = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
        native_h = AROS_LE2LONG(xsd->vcsd_MBoxMessage[6]);
    }
    VC4_MBOX_UNLOCK(xsd);

    xsd->vcsd_NativeWidth  = native_w;
    xsd->vcsd_NativeHeight = native_h;

    if (native_w && native_h)
    {
        struct VC4ModeEntry n = { native_w, native_h, 25174,
            native_w, native_w, native_w,
            native_h, native_h, native_h };
        if ((hdmi_mode = VC4_BuildMode(&n)) != NULL)
        {
            AddTail(modelist, &hdmi_mode->dm_Node);
            hdmi_modecount++;
            D(bug("[VideoCoreGfx] %s: native HDMI mode %dx%d\n",
                __PRETTY_FUNCTION__, (int)native_w, (int)native_h));
        }
    }

    /* Probe the candidate table. We add a mode if firmware accepts the test
     * (VideoCore can scale arbitrary framebuffer sizes onto the panel via the
     * HVS, so this is mostly a sanity check), skip duplicates of the
     * already-added native mode, and skip anything larger than native -
     * the panel can't display larger modes correctly even if the HVS would
     * happily produce them.
     */
    for (i = 0; i < VC4_NUM_CANDIDATES; i++)
    {
        const struct VC4ModeEntry *e = &vc4_candidate_modes[i];

        if (e->width == native_w && e->height == native_h)
            continue;

        if (native_w && native_h &&
            (e->width > native_w || e->height > native_h))
        {
            D(bug("[VideoCoreGfx] %s: skipping %dx%d (> native %dx%d)\n",
                __PRETTY_FUNCTION__, (int)e->width, (int)e->height,
                (int)native_w, (int)native_h));
            continue;
        }

        if (!VC4_TestRes(xsd, e->width, e->height))
        {
            D(bug("[VideoCoreGfx] %s: TESTRES rejected %dx%d\n",
                __PRETTY_FUNCTION__, (int)e->width, (int)e->height));
            continue;
        }

        if ((hdmi_mode = VC4_BuildMode(e)) != NULL)
        {
            AddTail(modelist, &hdmi_mode->dm_Node);
            hdmi_modecount++;
            D(bug("[VideoCoreGfx] %s: added candidate %dx%d\n",
                __PRETTY_FUNCTION__, (int)e->width, (int)e->height));
        }
    }

    return hdmi_modecount;
}

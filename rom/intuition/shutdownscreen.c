/*
   Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/libraries.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "intuition_intern.h"
#include "monitorclass_private.h"

static VOID ShowShutdownScreen();
static struct Screen *OpenFinalScreen(BYTE MinDepth, BOOL squarePixels,
    struct IntuitionBase *IntuitionBase);
static VOID ShowPic(struct Screen *scr, struct IntuitionBase *IntuitionBase);

static const UWORD empty_pointer[1] = { 0 };

/* This reset handler is called if software power-off or reboot has not
 * occurred */
AROS_INTH1(ShutdownScreenHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    if (!(handler->is_Node.ln_Type & (0x80 | SD_FLAG_EMERGENCY)))
        ShowShutdownScreen();

    return FALSE;

    AROS_INTFUNC_EXIT
}

static VOID ShowShutdownScreen()
{
    struct IntuitionBase *IntuitionBase =
        (void *)TaggedOpenLibrary(TAGGEDOPEN_INTUITION);

    struct Screen *scr = NULL;

    if (!IsListEmpty(&GetPrivIBase(IntuitionBase)->MonitorList))
        scr = OpenFinalScreen(2, TRUE, IntuitionBase);

    if (scr != NULL)
        ShowPic(scr, IntuitionBase);

    return;
}

static struct Screen *OpenFinalScreen(BYTE MinDepth, BOOL squarePixels,
    struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    UWORD height;
    ULONG mode;
    struct Screen *scr = NULL;
    Object *pointer;
    struct SharedPointer *shared_pointer;

    height = 480;
    mode = BestModeID(BIDTAG_DesiredWidth, 640, BIDTAG_DesiredHeight, height,
        BIDTAG_Depth, MinDepth, TAG_DONE);
    if (mode == INVALID_ID)
        Alert(AN_SysScrnType);

    /* Set PAL or NTSC default height if we are running on Amiga(tm) hardware.
     * We also need to check if this is really PAL or NTSC mode because we
     * have to use PC 640x480 mode if user has Amiga hardware + RTG board.
     * Check DisplayFlags first because non-Amiga mode IDs use different format.
     */
    if (GfxBase->DisplayFlags & (NTSC | PAL))
    {
        if ((mode & MONITOR_ID_MASK) == NTSC_MONITOR_ID)
            height = squarePixels ? 400 : 200;
        else if ((mode & MONITOR_ID_MASK) == PAL_MONITOR_ID)
            height = squarePixels ? 512 : 256;
    }

    /* We want the screen to occupy the whole display, so we find the best
       matching mode ID and then open a screen with that mode */
    mode = BestModeID(BIDTAG_DesiredWidth, 640, BIDTAG_DesiredHeight, height,
        BIDTAG_Depth, MinDepth, TAG_DONE);

    /* Reset to default decorator. The one installed by C:Decoration may try
     * to load images from disk while opening the screen, which isn't a good
     * idea when the system is partly shut down */
    GetPrivIBase(IntuitionBase)->Decorator = NULL;
    GetPrivIBase(IntuitionBase)->ScrDecorClass = FindClass(SCRDECORCLASS);
    GetPrivIBase(IntuitionBase)->ScrDecorTags = NULL;
    GetPrivIBase(IntuitionBase)->MenuDecorClass = FindClass(MENUDECORCLASS);
    GetPrivIBase(IntuitionBase)->MenuDecorTags = NULL;
    GetPrivIBase(IntuitionBase)->WinDecorClass = FindClass(WINDECORCLASS);
    GetPrivIBase(IntuitionBase)->WinDecorTags = NULL;

    if (mode != INVALID_ID)
    {
        scr = OpenScreenTags(NULL, SA_DisplayID, mode, SA_Draggable, FALSE,
            SA_Quiet, TRUE, SA_Depth, MinDepth, TAG_DONE);

        /* Hide mouse pointer */
        if (scr)
        {
            struct msSetPointerShape pmsg;
            pmsg.MethodID = MM_SetPointerShape;
            pmsg.pointer = NULL;

            pointer = MakePointerFromData(IntuitionBase, empty_pointer,
                0, 0, 1, 1);
            GetAttr(POINTERA_SharedPointer, pointer,
                (IPTR *) &pmsg.pointer);
            DoMethodA(GetPrivScreen(scr)->IMonitorNode, &pmsg);
        }
    }

    return scr;
}

static VOID ShowPic(struct Screen *scr, struct IntuitionBase *IntuitionBase)
{
    static const char message1[] =
        "Please turn off your system using the power switch.";
    static const char message2[] =
        "You may need to press the switch for up to five seconds.";
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort *rp = &scr->RastPort;
    WORD xoff = ((WORD)scr->Width - 640) / 2;
    WORD yoff = ((WORD)scr->Height - 480) / 2;
    WORD y, x;

    SetRGB32(&scr->ViewPort, 0, 0xffffffff, 0xffffffff, 0xffffffff);
    SetRGB32(&scr->ViewPort, 1, 0x00000000, 0x5d5d5d5d, 0xb7b7b7b7);
    SetRGB32(&scr->ViewPort, 2, 0xffffffff, 0x88888888, 0x00000000);

    SetDrMd(rp, JAM1);
    SetAPen(rp, 0);
    RectFill(rp, 0, 0, scr->Width - 1, scr->Height - 1);

    SetAPen(rp, 1);
    RectFill(rp, xoff + 54, yoff + 165, xoff + 506, yoff + 205);
    SetAPen(rp, 0);
    x = xoff + 54 + (453 - TextLength(rp, message1,
        sizeof(message1) - 1)) / 2;
    y = yoff + 165 + (41 - rp->TxHeight) / 2 + rp->TxBaseline;
    Move(rp, x, y);
    Text(rp, message1, sizeof(message1) - 1);

    SetAPen(rp, 2);
    RectFill(rp, xoff + 153, yoff + 230, xoff + 606, yoff + 270);
    SetAPen(rp, 1);
    x = xoff + 153 + (454 - TextLength(rp, message2,
        sizeof(message2) - 1)) / 2;
    y = yoff + 230 + (41 - rp->TxHeight) / 2 + rp->TxBaseline;
    Move(rp, x, y);
    Text(rp, message2, sizeof(message2) - 1);
}

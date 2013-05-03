/*
   Copyright © 1995-2013, The AROS Development Team. All rights reserved.
   $Id$
 */

#include <exec/libraries.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "intuition_intern.h"
#include "monitorclass_private.h"
#include "shutdown_image.h"

static VOID ShowShutdownScreen();
static struct Screen *OpenFinalScreen(BYTE MinDepth, BOOL squarePixels,
    struct IntuitionBase *IntuitionBase);
static VOID ShowPic(struct Screen *scr, struct IntuitionBase *IntuitionBase);
static const UBYTE *UnpackByterun(const UBYTE * source, UBYTE * dest,
    LONG unpackedsize);

static const UWORD empty_pointer[1] = { 0 };

/* This reset handler is called if software power-off or reboot has not
 * occurred */
AROS_INTH1(ShutdownScreenHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    ShowShutdownScreen();

    return FALSE;

    AROS_INTFUNC_EXIT
}

static VOID ShowShutdownScreen()
{
    struct IntuitionBase *IntuitionBase =
        (void *)TaggedOpenLibrary(TAGGEDOPEN_INTUITION);

    struct Screen *scr = OpenFinalScreen(4, TRUE, IntuitionBase);

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

    if (mode != INVALID_ID)
    {
        scr = OpenScreenTags(NULL, SA_DisplayID, mode, SA_Draggable, FALSE,
            SA_Quiet, TRUE, SA_Depth, MinDepth, TAG_DONE);

        /* Hide mouse pointer */
        if (scr)
        {
            pointer = MakePointerFromData(IntuitionBase, empty_pointer,
                0, 0, 1, 1);
            GetAttr(POINTERA_SharedPointer, pointer,
                (IPTR *) & shared_pointer);
            DoMethod(GetPrivScreen(scr)->IMonitorNode, MM_SetPointerShape,
                shared_pointer);
        }
    }

    return scr;
}

static VOID ShowPic(struct Screen *scr, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    UBYTE *picture;
    UWORD x, y;

    if ((scr->Width >= SHUTDOWN_WIDTH) && (scr->Height >= SHUTDOWN_HEIGHT)
        && (scr->RastPort.BitMap->Depth >= SHUTDOWN_PLANES))
    {
        ULONG size = SHUTDOWN_WIDTH * SHUTDOWN_HEIGHT;

        picture = AllocVec(size, MEMF_ANY);

        if (picture != NULL)
        {
            ULONG i;

            UnpackByterun(shutdown_data, picture, size);

            for (i = 0; i < SHUTDOWN_COLORS; i++)
                SetRGB32(&scr->ViewPort, i,
                    (shutdown_pal[i] << 8) & 0xFF000000,
                    (shutdown_pal[i] << 16) & 0xFF000000,
                    (shutdown_pal[i] << 24) & 0xFF000000);

            SetAPen(&scr->RastPort, 0);
            RectFill(&scr->RastPort, 0, 0, scr->Width, scr->Height);

            x = (scr->Width - SHUTDOWN_WIDTH) >> 1;
            y = (scr->Height - SHUTDOWN_HEIGHT) >> 1;
            WriteChunkyPixels(&scr->RastPort, x, y,
                x + SHUTDOWN_WIDTH - 1, y + SHUTDOWN_HEIGHT - 1,
                picture, SHUTDOWN_WIDTH);

            return;
        }
    }
    return;
}

static const UBYTE *UnpackByterun(const UBYTE * source, UBYTE * dest,
    LONG unpackedsize)
{
    UBYTE r;
    BYTE c;

    for (;;)
    {
        c = (BYTE) (*source++);
        if (c >= 0)
        {
            while (c-- >= 0)
            {
                *dest++ = *source++;
                if (--unpackedsize <= 0)
                    return source;
            }
        }
        else if (c != -128)
        {
            c = -c;
            r = *source++;

            while (c-- >= 0)
            {
                *dest++ = r;
                if (--unpackedsize <= 0)
                    return source;
            }
        }
    }
}


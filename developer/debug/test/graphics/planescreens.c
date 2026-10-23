/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Opens several true colour screens at once, for drivers that show
          each screen as its own display plane.

    Each screen gets its own colour, a 64 pixel grid, a frame and a label,
    so scaling, clipping and stacking can be checked by eye. Screen n opens
    STEP*n pixels down, so the ones behind stay partly visible. SIZES is a
    list of WxH cycled over the screens; the Workbench size, the default,
    reuses its mode so no mode switch is involved. CYCLE sends the front
    screen to the back every so many seconds.

    Usage: planescreens [COUNT=n] [SIZES=WxH ...] [STEP=n] [CYCLE=s] [SECONDS=s]
*/

#include <exec/types.h>
#include <intuition/screens.h>
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdio.h>
#include <string.h>

#define ARG_TEMPLATE "COUNT/N,SIZES/M,STEP/N,CYCLE/N,SECONDS/N"
enum { ARG_COUNT, ARG_SIZES, ARG_STEP, ARG_CYCLE, ARG_SECONDS, NUM_ARGS };

#define MAX_SCREENS 16

static const ULONG colours[8] =
{
    0x802020, 0x208020, 0x202080, 0x808020, 0x802080, 0x208080, 0x606060, 0xa05010
};

static void paint(struct Screen *scr, ULONG n, ULONG colour)
{
    struct RastPort *rp = &scr->RastPort;
    ULONG w = scr->Width, h = scr->Height, x, y;
    char label[64];

    FillPixelArray(rp, 0, 0, w, h, colour);
    for (x = 0; x < w; x += 64)
        FillPixelArray(rp, x, 0, 1, h, 0x000000);
    for (y = 0; y < h; y += 64)
        FillPixelArray(rp, 0, y, w, 1, 0x000000);

    /* A frame, so a clipped edge shows */
    FillPixelArray(rp, 0, 0, w, 4, 0xffffff);
    FillPixelArray(rp, 0, h - 4, w, 4, 0xffffff);
    FillPixelArray(rp, 0, 0, 4, h, 0xffffff);
    FillPixelArray(rp, w - 4, 0, 4, h, 0xffffff);

    snprintf(label, sizeof(label), "screen %lu: %lux%lu", (unsigned long)n,
             (unsigned long)w, (unsigned long)h);
    SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, 0xffffff, TAG_DONE);
    SetDrMd(rp, JAM1);
    Move(rp, 20, scr->BarHeight + 20 + rp->TxBaseline);
    Text(rp, label, strlen(label));
}

int main(void)
{
    IPTR args[NUM_ARGS] = { 0 };
    struct RDArgs *rda;
    struct Screen *scr[MAX_SCREENS] = { NULL };
    STRPTR *sizes = NULL;
    ULONG count = 5, step = 40, cycle = 0, seconds = 0;
    ULONG wbw = 640, wbh = 480, wbmode = INVALID_ID, i, n = 0, ticks, front;
    struct Screen *wb;
    int rc = RETURN_FAIL;

    if (!(rda = ReadArgs(ARG_TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "planescreens");
        return RETURN_FAIL;
    }
    if (args[ARG_COUNT])   count   = *(LONG *)args[ARG_COUNT];
    if (args[ARG_STEP])    step    = *(LONG *)args[ARG_STEP];
    if (args[ARG_CYCLE])   cycle   = *(LONG *)args[ARG_CYCLE];
    if (args[ARG_SECONDS]) seconds = *(LONG *)args[ARG_SECONDS];
    sizes = (STRPTR *)args[ARG_SIZES];
    if (count > MAX_SCREENS)
        count = MAX_SCREENS;

    if ((wb = LockPubScreen(NULL)))
    {
        wbmode = GetVPModeID(&wb->ViewPort);
        wbw = wb->Width;
        wbh = wb->Height;
        UnlockPubScreen(NULL, wb);
    }

    for (i = 0; i < count; i++)
    {
        ULONG w = wbw, h = wbh, mode = wbmode;
        char title[64];

        if (sizes && sizes[0])
        {
            unsigned long sw, sh;
            ULONG k = 0;

            while (sizes[k])
                k++;
            if (sscanf(sizes[i % k], "%lux%lu", &sw, &sh) != 2)
            {
                printf("bad size \"%s\", want WxH\n", sizes[i % k]);
                goto out;
            }
            w = sw;
            h = sh;
            if ((w != wbw) || (h != wbh))
                mode = INVALID_ID;
        }
        if (mode == INVALID_ID)
            mode = BestModeID(BIDTAG_NominalWidth, w, BIDTAG_NominalHeight, h,
                              BIDTAG_Depth, 24, TAG_DONE);
        if (mode == INVALID_ID)
        {
            printf("screen %lu: no %lux%lu true colour mode\n", (unsigned long)i,
                   (unsigned long)w, (unsigned long)h);
            continue;
        }

        snprintf(title, sizeof(title), "planescreens %lu", (unsigned long)i);
        scr[n] = OpenScreenTags(NULL,
                                SA_DisplayID, mode,
                                SA_Width,     w,
                                SA_Height,    h,
                                SA_Depth,     24,
                                SA_Top,       step * i,
                                SA_Title,     (IPTR)title,
                                SA_ShowTitle, TRUE,
                                TAG_DONE);
        if (!scr[n])
        {
            printf("screen %lu: cannot open %lux%lu mode 0x%08lx\n", (unsigned long)i,
                   (unsigned long)w, (unsigned long)h, (unsigned long)mode);
            continue;
        }
        paint(scr[n], i, colours[i % 8]);
        printf("screen %lu: %lux%lu mode 0x%08lx top %lu\n", (unsigned long)i,
               (unsigned long)w, (unsigned long)h, (unsigned long)mode,
               (unsigned long)(step * i));
        n++;
    }

    printf("%lu screens open, Ctrl-C to close\n", (unsigned long)n);

    /* Tenths of a second; front counts down as each goes to the back */
    for (ticks = 0, front = n; n; ticks++)
    {
        if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
            break;
        if (seconds && (ticks >= seconds * 10))
            break;
        if (cycle && ticks && !(ticks % (cycle * 10)))
        {
            front = front ? front - 1 : n - 1;
            ScreenToBack(scr[front]);
        }
        Delay(5);
    }
    rc = RETURN_OK;

out:
    while (n--)
    {
        if (!CloseScreen(scr[n]))
            printf("screen %lu: CloseScreen failed\n", (unsigned long)n);
    }
    FreeArgs(rda);
    return rc;
}

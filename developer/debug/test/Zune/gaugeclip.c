/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A gauge whose info text is wider than the gauge must not draw outside
    its own box. Three gauges are laid out for a short text next to other
    objects, one of them at the right window border. The text is then
    replaced by a long one and the window is compared pixel by pixel with
    its contents before the change: only pixels inside the gauges may differ.

    With the argument WAIT the window stays open until it is closed.
*/

#include <dos/dos.h>
#include <cybergraphx/cybergraphics.h>
#include <intuition/intuition.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <stdio.h>
#include <string.h>

#define GAUGES 3

static const char shorttext[] = "0";
static const char longtext[] = "this info text is wider than the gauge";

static int failures;

static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

/* handle pending input and refreshes for a number of ticks */
static void pump(Object *app, int ticks)
{
    ULONG sigs;

    while (ticks-- > 0)
    {
        DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs);
        Delay(1);
    }
}

static ULONG *grab(struct Window *w)
{
    ULONG *buf = AllocVec(w->Width * w->Height * 4, MEMF_ANY);

    if (buf && !ReadPixelArray(buf, 0, 0, w->Width * 4, w->RPort, 0, 0,
            w->Width, w->Height, RECTFMT_ARGB))
    {
        FreeVec(buf);
        buf = NULL;
    }
    return buf;
}

static int inside(Object *o, int x, int y)
{
    return x >= _left(o) && x <= _right(o) && y >= _top(o) && y <= _bottom(o);
}

int main(int argc, char **argv)
{
    Object *app, *wnd, *gauge[GAUGES];
    struct Window *w = NULL;
    ULONG *before, *after;
    int i, x, y;
    int changed_in = 0, changed_out = 0, minx = 0, miny = 0, maxx = 0, maxy = 0;

    app = ApplicationObject,
        MUIA_Application_Title, (IPTR)"gaugeclip",
        SubWindow, wnd = WindowObject,
            MUIA_Window_Title, (IPTR)"gaugeclip",
            MUIA_Window_Activate, TRUE,
            WindowContents, VGroup,
                Child, HGroup,
                    Child, gauge[0] = GaugeObject,
                        GaugeFrame,
                        MUIA_Gauge_Horiz, TRUE,
                        MUIA_Gauge_InfoText, (IPTR)shorttext,
                        MUIA_Gauge_Current, 50,
                        MUIA_Weight, 0,
                    End,
                    Child, TextObject,
                        TextFrame,
                        MUIA_Text_Contents, (IPTR)"object right of a horizontal gauge",
                    End,
                End,
                Child, HGroup,
                    Child, gauge[1] = GaugeObject,
                        GaugeFrame,
                        MUIA_Gauge_InfoText, (IPTR)shorttext,
                        MUIA_Gauge_Current, 50,
                    End,
                    Child, TextObject,
                        TextFrame,
                        MUIA_Text_Contents, (IPTR)"object right of a vertical gauge",
                    End,
                End,
                Child, HGroup,
                    Child, TextObject,
                        TextFrame,
                        MUIA_Text_Contents, (IPTR)"gauge at the window border",
                    End,
                    Child, gauge[2] = GaugeObject,
                        GaugeFrame,
                        MUIA_Gauge_Horiz, TRUE,
                        MUIA_Gauge_InfoText, (IPTR)shorttext,
                        MUIA_Gauge_Current, 50,
                        MUIA_Weight, 0,
                    End,
                End,
            End,
        End,
    End;
    if (!app)
    {
        printf("FAIL: no application\n");
        return RETURN_FAIL;
    }

    set(wnd, MUIA_Window_Open, TRUE);
    get(wnd, MUIA_Window_Window, &w);
    if (!w)
    {
        printf("FAIL: the window did not open\n");
        MUI_DisposeObject(app);
        return RETURN_FAIL;
    }

    /* let the window become active and finish drawing */
    pump(app, 50);

    for (i = 0; i < GAUGES; i++)
    {
        struct RastPort rp;

        InitRastPort(&rp);
        SetFont(&rp, _font(gauge[i]));
        if (TextLength(&rp, longtext, strlen(longtext)) <= _mwidth(gauge[i]))
        {
            printf("FAIL: gauge %d is %d pixels wide, too wide for this test\n",
                i, (int)_mwidth(gauge[i]));
            failures++;
        }
    }

    before = grab(w);
    for (i = 0; i < GAUGES; i++)
        set(gauge[i], MUIA_Gauge_InfoText, (IPTR)longtext);
    after = grab(w);

    if (before && after)
    {
        for (y = 0; y < w->Height; y++)
        {
            for (x = 0; x < w->Width; x++)
            {
                if (before[y * w->Width + x] == after[y * w->Width + x])
                    continue;
                for (i = 0; i < GAUGES; i++)
                {
                    if (inside(gauge[i], x, y))
                        break;
                }
                if (i < GAUGES)
                {
                    changed_in++;
                    continue;
                }
                if (!changed_out++)
                {
                    minx = maxx = x;
                    miny = maxy = y;
                }
                if (x < minx) minx = x;
                if (x > maxx) maxx = x;
                if (y < miny) miny = y;
                if (y > maxy) maxy = y;
            }
        }
        check(changed_in > 0, "the gauges drew the new text");
        check(changed_out == 0, "nothing was drawn outside the gauges");
        if (changed_out)
        {
            printf("%d pixels changed outside the gauges, from %d,%d to %d,%d"
                " in a window of %dx%d\n", changed_out, minx, miny, maxx, maxy,
                (int)w->Width, (int)w->Height);
            for (i = 0; i < GAUGES; i++)
            {
                printf("gauge %d: %d,%d to %d,%d\n", i,
                    (int)_left(gauge[i]), (int)_top(gauge[i]),
                    (int)_right(gauge[i]), (int)_bottom(gauge[i]));
            }
        }
    }
    else
    {
        printf("FAIL: could not read the window contents\n");
        failures++;
    }
    FreeVec(before);
    FreeVec(after);

    if (argc > 1 && !stricmp(argv[1], "WAIT"))
    {
        ULONG sigs = 0;

        DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app,
            2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        while (DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs)
            != MUIV_Application_ReturnID_Quit)
        {
            if (sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                if (sigs & SIGBREAKF_CTRL_C)
                    break;
            }
        }
    }

    set(wnd, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);

    return failures ? RETURN_ERROR : RETURN_OK;
}

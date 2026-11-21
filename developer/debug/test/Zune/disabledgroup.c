/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Children of a disabled group. A group that is set to MUIA_Disabled TRUE
    twice and to FALSE once must leave its children enabled. An object that
    is redrawn on its own while it is disabled, by its group or itself, must
    look as it does when the whole window is drawn: a text whose contents
    are set, a slider whose value is set. The window is read after such
    changes and again after it was drawn as a whole, and the two are
    compared pixel by pixel.

    With the argument WAIT the window stays open until it is closed.
*/

#include <dos/dos.h>
#include <cybergraphx/cybergraphics.h>
#include <intuition/intuition.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <stdio.h>
#include <string.h>

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

/* the number of pixels that differ inside an object, or in the window */
static int differ(struct Window *w, const ULONG *a, const ULONG *b, Object *o)
{
    int x0 = 0, y0 = 0, x1 = w->Width - 1, y1 = w->Height - 1;
    int x, y, n = 0;

    if (o)
    {
        x0 = _left(o);
        y0 = _top(o);
        x1 = _right(o);
        y1 = _bottom(o);
    }
    for (y = y0; y <= y1; y++)
    {
        for (x = x0; x <= x1; x++)
        {
            if (a[y * w->Width + x] != b[y * w->Width + x])
                n++;
        }
    }
    return n;
}

int main(int argc, char **argv)
{
    Object *app, *wnd, *root, *countgroup, *counted, *group, *text, *slider;
    Object *own;
    struct Window *w = NULL;
    ULONG *before, *alone, *whole, *enabled;
    int i, in_text, in_slider, in_own;

    app = ApplicationObject,
        MUIA_Application_Title, (IPTR)"disabledgroup",
        SubWindow, wnd = WindowObject,
            MUIA_Window_Title, (IPTR)"disabledgroup",
            MUIA_Window_Activate, TRUE,
            WindowContents, root = VGroup,
                Child, countgroup = HGroup,
                    GroupFrame,
                    Child, counted = SimpleButton("child of a group"),
                End,
                Child, group = VGroup,
                    GroupFrame,
                    Child, text = TextObject,
                        TextFrame,
                        MUIA_Text_Contents, (IPTR)"text in a disabled group",
                    End,
                    Child, slider = SliderObject,
                        MUIA_Numeric_Max, 100,
                    End,
                End,
                Child, own = SliderObject,
                    MUIA_Numeric_Max, 100,
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

    set(countgroup, MUIA_Disabled, TRUE);
    check(XGET(counted, MUIA_Disabled) != 0,
        "a disabled group disables its child");
    set(countgroup, MUIA_Disabled, TRUE);
    set(countgroup, MUIA_Disabled, FALSE);
    check(XGET(counted, MUIA_Disabled) == 0,
        "TRUE, TRUE, FALSE leaves the child enabled");
    set(countgroup, MUIA_Disabled, FALSE);
    set(countgroup, MUIA_Disabled, TRUE);
    check(XGET(counted, MUIA_Disabled) != 0,
        "FALSE, FALSE, TRUE leaves the child disabled");
    set(countgroup, MUIA_Disabled, FALSE);
    check(XGET(counted, MUIA_Disabled) == 0, "FALSE enables the child again");

    set(group, MUIA_Disabled, TRUE);
    set(own, MUIA_Disabled, TRUE);
    before = grab(w);

    set(text, MUIA_Text_Contents, (IPTR)"contents set");
    for (i = 1; i <= 3; i++)
    {
        set(slider, MUIA_Numeric_Value, 25 * i);
        set(own, MUIA_Numeric_Value, 25 * i);
    }
    alone = grab(w);

    MUI_Redraw(root, MADF_DRAWOBJECT);
    whole = grab(w);

    set(group, MUIA_Disabled, FALSE);
    set(own, MUIA_Disabled, FALSE);
    enabled = grab(w);

    if (before && alone && whole && enabled)
    {
        check(differ(w, before, alone, text) && differ(w, before, alone, slider)
            && differ(w, before, alone, own), "the changes were drawn");
        check(differ(w, whole, enabled, text)
            && differ(w, whole, enabled, slider)
            && differ(w, whole, enabled, own),
            "disabled objects are drawn with the pattern");

        in_text = differ(w, alone, whole, text);
        in_slider = differ(w, alone, whole, slider);
        in_own = differ(w, alone, whole, own);
        check(in_text == 0, "a text set in a disabled group keeps the pattern");
        check(in_slider == 0,
            "a slider set in a disabled group keeps the pattern");
        check(in_own == 0, "a disabled slider that is set keeps its look");
        check(differ(w, alone, whole, NULL) == in_text + in_slider + in_own,
            "nothing else differs from the window drawn as a whole");
        if (in_text || in_slider || in_own)
        {
            printf("pixels that differ from the window drawn as a whole: "
                "text %d of %d, slider %d of %d, disabled slider %d of %d\n",
                in_text, (int)(_width(text) * _height(text)),
                in_slider, (int)(_width(slider) * _height(slider)),
                in_own, (int)(_width(own) * _height(own)));
        }
    }
    else
    {
        printf("FAIL: could not read the window contents\n");
        failures++;
    }
    FreeVec(before);
    FreeVec(alone);
    FreeVec(whole);
    FreeVec(enabled);

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

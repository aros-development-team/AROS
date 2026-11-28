/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Stepslider.mcc: a window with three sliders (four steps, a reversed
    one, a disabled one) and a longer range. The text beside each slider
    shows the value from its notification.

    stepslider CHECK runs the checks that need no user and prints
    "PASS n of n" or the failed checks; the return code is 0 or 20.
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/mui.h>
#include <zune/stepslider.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>

#define ID_LEVEL   1
#define ID_REVERSE 2
#define ID_LONG    3

static CONST_STRPTR levels[] = { "Store only", "Fast", "Normal", "Best" };

static LONG checks, failed;

static void Check(BOOL ok, CONST_STRPTR what)
{
    checks++;
    if (!ok)
    {
        failed++;
        printf("FAIL %s\n", what);
    }
}

static LONG Value(Object *obj)
{
    return (LONG)XGET(obj, MUIA_Numeric_Value);
}

static void ShowValue(Object *text, Object *slider, BOOL named)
{
    static char buf[3][32];
    static int next;
    char *b = buf[next];
    LONG v = Value(slider);

    next = (next + 1) % 3;
    if (named && v >= 0 && v <= 3)
        snprintf(b, sizeof(buf[0]), "%s", levels[v]);
    else
        snprintf(b, sizeof(buf[0]), "%ld", (long)v);
    set(text, MUIA_Text_Contents, (IPTR)b);
}

/* Returns the number of ReturnIDs waiting in the application */
static LONG Pending(Object *app, ULONG id)
{
    ULONG sigs = 0;
    LONG count = 0, i;

    for (i = 0; i < 20; i++)
    {
        IPTR rid = DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs);

        if (rid == id)
            count++;
        else if (rid == 0)
            break;
    }
    return count;
}

static void RunChecks(Object *app, Object *win, Object *level,
    Object *reverse, Object *disabled, Object *longer)
{
    Object *bad;
    LONG height;

    Pending(app, ID_LEVEL);

    Check(Value(level) == 2, "initial value");
    set(level, MUIA_Numeric_Value, 3);
    Check(Value(level) == 3, "set value");
    Check(Pending(app, ID_LEVEL) == 1, "one notification for one change");
    set(level, MUIA_Numeric_Value, 3);
    Check(Pending(app, ID_LEVEL) == 0, "no notification for the same value");
    set(level, MUIA_Numeric_Value, 9);
    Check(Value(level) == 3, "value above Max is clamped");
    set(level, MUIA_Numeric_Value, -4);
    Check(Value(level) == 0, "value below Min is clamped");
    DoMethod(level, MUIM_Numeric_Increase, 1);
    Check(Value(level) == 1, "Increase");
    DoMethod(level, MUIM_Numeric_Decrease, 1);
    Check(Value(level) == 0, "Decrease");
    set(level, MUIA_Numeric_Default, 2);
    DoMethod(level, MUIM_Numeric_SetDefault);
    Check(Value(level) == 2, "SetDefault");

    set(level, MUIA_Numeric_Max, 1);
    Check(Value(level) == 1, "smaller Max clamps the value");
    set(level, MUIA_Numeric_Max, 3);
    set(level, MUIA_Numeric_Value, 2);
    Pending(app, ID_LEVEL);

    Check(XGET(reverse, MUIA_Numeric_Reverse) != 0, "Reverse is kept");
    Check(Value(reverse) == 1, "reversed slider keeps its value");
    Check(XGET(disabled, MUIA_Disabled) != 0, "Disabled is kept");
    Check(Value(longer) == 40, "long range value");

    /* One object in its own group has the size the class asks for */
    height = (LONG)XGET(level, MUIA_Height);
    Check(height > 0 && height == (LONG)XGET(reverse, MUIA_Height),
        "all sliders have the same fixed height");
    Check((LONG)XGET(level, MUIA_Width) >= 4 * 4,
        "width has room for four steps");
    Check((LONG)XGET(level, MUIA_Width) == (LONG)XGET(longer, MUIA_Width),
        "sliders in one column have one width");

    /* An object that is never shown must be disposable */
    bad = StepsliderObject, MUIA_Numeric_Min, 5, MUIA_Numeric_Max, 5, End;
    Check(bad != NULL, "single-step object");
    if (bad)
    {
        Check(Value(bad) == 5, "single-step value");
        MUI_DisposeObject(bad);
    }

    /* Close and open again: Setup and Cleanup run twice */
    set(win, MUIA_Window_Open, FALSE);
    set(win, MUIA_Window_Open, TRUE);
    Check(XGET(win, MUIA_Window_Open) != 0, "window opens again");
    Check(Value(level) == 2, "value kept across close and open");

    if (failed)
        printf("FAILED %ld of %ld\n", (long)failed, (long)checks);
    else
        printf("PASS %ld of %ld\n", (long)checks, (long)checks);
}

int main(int argc, char **argv)
{
    Object *app, *win, *level, *reverse, *disabled, *longer;
    Object *t_level, *t_reverse, *t_long;
    BOOL check = argc > 1 && !strcmp(argv[1], "CHECK");
    ULONG sigs = 0;
    BOOL running = TRUE;

    app = ApplicationObject,
        MUIA_Application_Title, (IPTR)"Stepslider",
        MUIA_Application_Base, (IPTR)"STEPSLIDERTEST",
        SubWindow, (IPTR)(win = WindowObject,
            MUIA_Window_Title, (IPTR)"Stepslider.mcc",
            MUIA_Window_ID, MAKE_ID('S', 'T', 'S', 'L'),
            WindowContents, (IPTR)(ColGroup(3),
                Child, (IPTR)Label("Compression"),
                Child, (IPTR)(level = StepsliderObject,
                    MUIA_Numeric_Min, 0,
                    MUIA_Numeric_Max, 3,
                    MUIA_Numeric_Value, 2,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)(t_level = TextObject,
                    MUIA_FixWidthTxt, (IPTR)"Store only",
                End),
                Child, (IPTR)Label("Reversed"),
                Child, (IPTR)(reverse = StepsliderObject,
                    MUIA_Numeric_Min, 1,
                    MUIA_Numeric_Max, 5,
                    MUIA_Numeric_Value, 1,
                    MUIA_Numeric_Reverse, TRUE,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)(t_reverse = TextObject,
                    MUIA_Text_Contents, (IPTR)"0",
                End),
                Child, (IPTR)Label("Disabled"),
                Child, (IPTR)(disabled = StepsliderObject,
                    MUIA_Numeric_Min, 0,
                    MUIA_Numeric_Max, 2,
                    MUIA_Numeric_Value, 1,
                    MUIA_Disabled, TRUE,
                End),
                Child, (IPTR)HVSpace,
                Child, (IPTR)Label("0 to 200"),
                Child, (IPTR)(longer = StepsliderObject,
                    MUIA_Numeric_Min, 0,
                    MUIA_Numeric_Max, 200,
                    MUIA_Numeric_Value, 40,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)(t_long = TextObject,
                    MUIA_Text_Contents, (IPTR)"000",
                End),
            End),
        End),
    End;

    if (!app)
    {
        printf("FAIL no application (Stepslider.mcc missing?)\n");
        return RETURN_FAIL;
    }

    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR)app, 2, MUIM_Application_ReturnID,
        MUIV_Application_ReturnID_Quit);
    DoMethod(level, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR)app, 2, MUIM_Application_ReturnID, ID_LEVEL);
    DoMethod(reverse, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR)app, 2, MUIM_Application_ReturnID, ID_REVERSE);
    DoMethod(longer, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR)app, 2, MUIM_Application_ReturnID, ID_LONG);

    ShowValue(t_level, level, TRUE);
    ShowValue(t_reverse, reverse, FALSE);
    ShowValue(t_long, longer, FALSE);

    set(win, MUIA_Window_Open, TRUE);
    if (!XGET(win, MUIA_Window_Open))
    {
        printf("FAIL the window does not open\n");
        MUI_DisposeObject(app);
        return RETURN_FAIL;
    }

    if (check)
    {
        RunChecks(app, win, level, reverse, disabled, longer);
        running = FALSE;
    }

    while (running)
    {
        switch (DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs))
        {
        case MUIV_Application_ReturnID_Quit:
            running = FALSE;
            break;
        case ID_LEVEL:
            ShowValue(t_level, level, TRUE);
            break;
        case ID_REVERSE:
            ShowValue(t_reverse, reverse, FALSE);
            break;
        case ID_LONG:
            ShowValue(t_long, longer, FALSE);
            break;
        }

        if (running && sigs)
        {
            sigs = Wait(sigs | SIGBREAKF_CTRL_C);
            if (sigs & SIGBREAKF_CTRL_C)
                running = FALSE;
        }
    }

    MUI_DisposeObject(app);
    return failed ? RETURN_FAIL : RETURN_OK;
}

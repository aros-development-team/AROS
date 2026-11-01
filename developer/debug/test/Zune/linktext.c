/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Test of Linktext.mcc. "linktext CHECK" runs without a window and exits
    with the number of failures; without arguments it opens a window, and
    every action is printed with the length and text of its target.
*/

#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include <zune/linktext.h>
#include <stdio.h>
#include <string.h>

static const char paragraph[] =
    "A long address wraps with its paragraph: "
    "https://example.org/a/long/address/that/goes/over/several/lines/of/this/window?x=1&y=%2F "
    "and the text goes on. A short label: Project website. The same target again: this site. "
    "Endings stay: bang, semicolon.\n\nSecond paragraph.";

static struct MUI_LinktextSpan spans[5];

static void span(int i, const char *label, const char *target, IPTR id)
{
    spans[i].Start = strstr(paragraph, label) - paragraph;
    spans[i].Length = strlen(label);
    spans[i].Flags = 0;
    spans[i].Target = (CONST_STRPTR)target;
    spans[i].ID = id;
}

static void make_spans(void)
{
    span(0, "https://example.org/a/long/address/that/goes/over/several/lines/of/this/window?x=1&y=%2F",
         "https://example.org/a/long/address/that/goes/over/several/lines/of/this/window?x=1&y=%2F", 1);
    span(1, "Project website", "https://example.org/project", 2);
    span(2, "this site", "https://example.org/project", 3);
    span(3, "bang", "https://example.org/download?token=abc!", 4);
    span(4, "semicolon", "https://example.org/x;", 5);
}

static int fails;

static void expect(Object *o, const char *what, CONST_STRPTR text, ULONG count, ULONG want)
{
    ULONG r = DoMethod(o, MUIM_Linktext_SetContent, (IPTR)text, (ULONG)~0UL, (IPTR)spans, count);
    if (r != want)
    {
        printf("FAIL %s: status %lu, wanted %lu\n", what, (unsigned long)r, (unsigned long)want);
        fails++;
    }
    else
        printf("ok   %s\n", what);
}

static int check(void)
{
    Object *o = MUI_NewObject(MUIC_Linktext, TAG_DONE);
    if (o == NULL)
    {
        printf("FAIL no Linktext.mcc\n");
        return 1;
    }
    make_spans();
    expect(o, "valid content", (CONST_STRPTR)paragraph, 5, MUIV_Linktext_Status_OK);
    expect(o, "no content", NULL, 0, MUIV_Linktext_Status_OK);
    spans[4].Start = 100000;
    expect(o, "span past the end", (CONST_STRPTR)paragraph, 5, MUIV_Linktext_Status_Range);
    spans[4].Start = 0xFFFFFFF0;
    spans[4].Length = 0x20;
    expect(o, "span wrapping around", (CONST_STRPTR)paragraph, 5, MUIV_Linktext_Status_Range);
    make_spans();
    spans[1].Start = spans[0].Start + 1;
    expect(o, "overlapping spans", (CONST_STRPTR)paragraph, 5, MUIV_Linktext_Status_Order);
    make_spans();
    spans[0].Target = (CONST_STRPTR)"";
    expect(o, "empty target", (CONST_STRPTR)paragraph, 5, MUIV_Linktext_Status_Target);
    MUI_DisposeObject(o);
    return fails;
}

AROS_UFH3(static IPTR, action,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(struct MUI_LinktextAction *, a, A1))
{
    AROS_USERFUNC_INIT

    printf("%s link %lu id %lu length %lu: %s\n",
        a->Action == MUIV_Linktext_Action_Open ? "open" : "copy",
        (unsigned long)a->Link, (unsigned long)a->ID,
        (unsigned long)a->TargetLength, (const char *)a->Target);
    fflush(stdout);
    return a->Action == MUIV_Linktext_Action_Open;

    AROS_USERFUNC_EXIT
}

int main(int argc, char **argv)
{
    static struct Hook hook;
    Object *app, *win, *view, *bar;
    ULONG sigs = 0;

    if (argc > 1)
        return check();

    make_spans();
    hook.h_Entry = (HOOKFUNC)action;
    bar = ScrollbarObject, End;
    app = ApplicationObject,
        MUIA_Application_Title, (IPTR)"Linktext test",
        SubWindow, (IPTR)(win = WindowObject,
            MUIA_Window_Title, (IPTR)"Linktext",
            WindowContents, (IPTR)(HGroup,
                MUIA_Group_Spacing, 0,
                Child, (IPTR)(view = MUI_NewObject(MUIC_Linktext,
                    MUIA_Frame, MUIV_Frame_ReadList,
                    MUIA_Background, (IPTR)MUII_ReadListBack,
                    MUIA_CycleChain, 1,
                    MUIA_Linktext_ActionHook, (IPTR)&hook,
                    MUIA_Linktext_Scroller, (IPTR)bar,
                    MUIA_Linktext_Lines, 8,
                    TAG_DONE)),
                Child, (IPTR)bar,
            End),
        End),
    End;
    if (app == NULL)
    {
        printf("no window: is Linktext.mcc installed?\n");
        return 20;
    }
    if (DoMethod(view, MUIM_Linktext_SetContent, (IPTR)paragraph, (ULONG)~0UL, (IPTR)spans, 5) != MUIV_Linktext_Status_OK)
        printf("the content was refused\n");
    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2,
        MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    set(win, MUIA_Window_Open, TRUE);
    while (DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs) != (ULONG)MUIV_Application_ReturnID_Quit)
        if (sigs)
            sigs = Wait(sigs | SIGBREAKF_CTRL_C);
    MUI_DisposeObject(app);
    return 0;
}

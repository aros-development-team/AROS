/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    MUIA_Text_Shorten: a window with one long text in every shortening
    mode, each in a text object without a minimum width. Make the window
    narrower or wider to see them change. The lower group holds the same
    modes at a fixed width of 150 pixels, a text of two lines and a short
    text that needs no shortening.

    textshorten CHECK runs the checks that need no user and prints
    "PASS n of n" or the failed checks; the return code is 0 or 20.
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>

#define FIXED_WIDTH 150
#define MODES 6

static CONST_STRPTR mode_names[MODES] =
{
    "Nothing", "Cutoff", "Hide", "ElideLeft", "ElideCenter", "ElideRight"
};

static CONST_STRPTR long_text =
    "The quick brown fox \033bjumps over\033n the lazy dog and runs on.";
static CONST_STRPTR two_lines =
    "A first line that is much too long to fit\nshort";
static CONST_STRPTR short_text = "fits";

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

static Object *ShortText(CONST_STRPTR contents, LONG mode, LONG width)
{
    return TextObject,
        MUIA_Frame, MUIV_Frame_Text,
        MUIA_Background, MUII_TextBack,
        MUIA_Text_Contents, (IPTR) contents,
        MUIA_Text_SetMin, FALSE,
        MUIA_Text_Shorten, mode,
        width ? MUIA_FixWidth : TAG_IGNORE, width,
        End;
}

static BOOL Shortened(Object *obj)
{
    return XGET(obj, MUIA_Text_Shortened) != 0;
}

static void RunChecks(Object *fixed[MODES], Object *lines, Object *fits,
    BOOL open)
{
    char what[96];
    LONG i;

    for (i = 0; i < MODES; i++)
    {
        snprintf(what, sizeof(what), "%s: MUIA_Text_Shorten reads back",
            mode_names[i]);
        Check((LONG) XGET(fixed[i], MUIA_Text_Shorten) == i, what);

        if (!open)
        {
            snprintf(what, sizeof(what),
                "%s: not shortened before the window is open", mode_names[i]);
            Check(!Shortened(fixed[i]), what);
            continue;
        }

        /* The object with the short text has the same frame and the same
         * fixed width: a long text must not make its object any wider. */
        snprintf(what, sizeof(what), "%s: the long text does not widen it",
            mode_names[i]);
        Check(XGET(fixed[i], MUIA_Width) == XGET(fits, MUIA_Width), what);

        snprintf(what, sizeof(what), "%s: MUIA_Text_Shortened is %s",
            mode_names[i], i == MUIV_Text_Shorten_Nothing ? "FALSE" : "TRUE");
        Check(Shortened(fixed[i]) == (i != MUIV_Text_Shorten_Nothing), what);
    }

    if (!open)
        return;

    Check(Shortened(lines), "two lines, the first too long: shortened");
    Check(!Shortened(fits), "a text that fits is not shortened");

    /* A new text is measured again. */
    set(fits, MUIA_Text_Contents, (IPTR) long_text);
    Check(Shortened(fits), "after a longer text: shortened");
    set(fits, MUIA_Text_Contents, (IPTR) short_text);
    Check(!Shortened(fits), "after the short text again: not shortened");

    /* The mode can be changed on an object that is shown. */
    set(fixed[MUIV_Text_Shorten_ElideRight], MUIA_Text_Shorten,
        MUIV_Text_Shorten_Nothing);
    Check(!Shortened(fixed[MUIV_Text_Shorten_ElideRight]),
        "set to Nothing: not shortened");
    set(fixed[MUIV_Text_Shorten_ElideRight], MUIA_Text_Shorten,
        MUIV_Text_Shorten_ElideRight);
    Check(Shortened(fixed[MUIV_Text_Shorten_ElideRight]),
        "set to ElideRight again: shortened");
    Check((LONG) XGET(fixed[MUIV_Text_Shorten_ElideRight], MUIA_Text_Shorten)
        == MUIV_Text_Shorten_ElideRight, "the mode that was set reads back");

    /* Texts that give elision nothing to keep or nothing to cut. */
    set(fits, MUIA_Text_Contents, (IPTR) "");
    Check(!Shortened(fits), "an empty text is not shortened");
    set(fits, MUIA_Text_Contents,
        (IPTR) "\033-An unparsed line that is far too long to fit here");
    Check(Shortened(fits), "a line with the engine off: shortened (cut)");
    set(fits, MUIA_Text_Contents,
        (IPTR) "\033c\033b\0338Only codes and one very long centred bold line");
    Check(Shortened(fits), "codes before the text: shortened");
    set(fits, MUIA_Text_Contents, (IPTR) short_text);
}

int main(int argc, char **argv)
{
    Object *app, *win, *free_text[MODES], *fixed[MODES], *lines, *fits;
    Object *free_group, *fixed_group;
    BOOL check = argc > 1 && !stricmp(argv[1], "CHECK");
    ULONG sigs = 0;
    LONG i;

    free_group = ColGroup(2), GroupFrameT("Follows the window's width"), End;
    fixed_group = ColGroup(2), GroupFrameT("150 pixels wide"), End;
    if (!free_group || !fixed_group)
    {
        printf("Cannot create the groups\n");
        return RETURN_FAIL;
    }

    for (i = 0; i < MODES; i++)
    {
        free_text[i] = ShortText(long_text, i, 0);
        fixed[i] = ShortText(long_text, i, FIXED_WIDTH);
        if (!free_text[i] || !fixed[i])
        {
            printf("Cannot create the text objects\n");
            return RETURN_FAIL;
        }
        DoMethod(free_group, OM_ADDMEMBER, (IPTR) Label(mode_names[i]));
        DoMethod(free_group, OM_ADDMEMBER, (IPTR) free_text[i]);
        DoMethod(fixed_group, OM_ADDMEMBER, (IPTR) Label(mode_names[i]));
        DoMethod(fixed_group, OM_ADDMEMBER,
            (IPTR) (HGroup, Child, (IPTR) fixed[i], Child, (IPTR) HSpace(0),
                End));
    }
    lines = ShortText(two_lines, MUIV_Text_Shorten_ElideCenter, FIXED_WIDTH);
    fits = ShortText(short_text, MUIV_Text_Shorten_ElideRight, FIXED_WIDTH);
    DoMethod(fixed_group, OM_ADDMEMBER, (IPTR) Label("Two lines"));
    DoMethod(fixed_group, OM_ADDMEMBER,
        (IPTR) (HGroup, Child, (IPTR) lines, Child, (IPTR) HSpace(0), End));
    DoMethod(fixed_group, OM_ADDMEMBER, (IPTR) Label("Fits"));
    DoMethod(fixed_group, OM_ADDMEMBER,
        (IPTR) (HGroup, Child, (IPTR) fits, Child, (IPTR) HSpace(0), End));

    app = ApplicationObject,
        MUIA_Application_Title, (IPTR) "textshorten",
        MUIA_Application_Base, (IPTR) "TEXTSHORTEN",
        SubWindow, (IPTR) (win = WindowObject,
            MUIA_Window_Title, (IPTR) "MUIA_Text_Shorten",
            MUIA_Window_ID, MAKE_ID('T', 'S', 'H', 'O'),
            MUIA_Window_Width, 420,
            WindowContents, (IPTR) (VGroup,
                Child, (IPTR) free_group,
                Child, (IPTR) fixed_group,
                End),
            End),
        End;

    if (!app)
    {
        printf("Cannot create the application\n");
        return RETURN_FAIL;
    }

    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR) app, 2, MUIM_Application_ReturnID,
        MUIV_Application_ReturnID_Quit);

    if (check)
        RunChecks(fixed, lines, fits, FALSE);

    set(win, MUIA_Window_Open, TRUE);
    if (!XGET(win, MUIA_Window_Open))
    {
        printf("Cannot open the window\n");
        MUI_DisposeObject(app);
        return RETURN_FAIL;
    }

    if (check)
    {
        RunChecks(fixed, lines, fits, TRUE);
    }
    else
    {
        while ((LONG) DoMethod(app, MUIM_Application_NewInput, (IPTR) &sigs)
            != (LONG) MUIV_Application_ReturnID_Quit)
        {
            if (sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                if (sigs & SIGBREAKF_CTRL_C)
                    break;
            }
        }
    }

    MUI_DisposeObject(app);

    if (check)
    {
        if (failed)
        {
            printf("FAILED %ld of %ld\n", (long) failed, (long) checks);
            return RETURN_FAIL;
        }
        printf("PASS %ld of %ld\n", (long) checks, (long) checks);
    }
    return RETURN_OK;
}

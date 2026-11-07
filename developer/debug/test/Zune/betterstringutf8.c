/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BetterString.mcc in its UTF-8 mode (MUIA_BetterString_UTF8),
          beside an 8-bit BetterString in the same window.

    BetterStringUTF8 [CLIP]

    Opens a window with both fields and checks, without user input:
    UTF-8 contents kept as given, text that is not UTF-8 refused, positions
    moved to grapheme cluster boundaries, MaxLen cutting at a boundary,
    MUIM_BetterString_Insert, cursor keys and Backspace by clusters (key
    events sent to the field), and the 8-bit field unchanged by any of it.
    With CLIP it also copies and pastes through the clipboard (unit 0, whose
    contents it replaces) and checks the UTF8 chunk. Prints one line per
    check, then "PASS n" or "FAIL n of m"; the return code is 0 or 10. It
    writes no files and can run again at once.
*/

#include <exec/types.h>
#include <libraries/mui.h>
#include <libraries/iffparse.h>
#include <intuition/intuition.h>
#include <devices/rawkeycodes.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/iffparse.h>
#include <proto/utility.h>
#include <mui/BetterString_mcc.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>

#ifndef MAKE_ID
#define MAKE_ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (ULONG)(c) << 8 | (ULONG)(d))
#endif

/* Polish, a combining accent, an emoji sequence (one cluster) */
#define ZOLC    "Za\xc5\xbc\xc3\xb3\xc5\x82\xc4\x87"             /* Zażółć */
#define E_ACUTE "e\xcc\x81"                                       /* e + U+0301 */
#define FAMILY  "\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa7"    /* woman ZWJ girl */

static int checks, failed;

static void check(int ok, const char *what)
{
    checks++;
    if (!ok)
        failed++;
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
}

static STRPTR contents(Object *o)
{
    IPTR s = 0;
    get(o, MUIA_String_Contents, &s);
    return (STRPTR)s;
}

static LONG pos(Object *o)
{
    IPTR p = 0;
    get(o, MUIA_String_BufferPos, &p);
    return (LONG)p;
}

/* A key to the active object, as Zune hands it to an event handler. */
static void key(Object *o, UWORD code)
{
    struct IntuiMessage im;
    ULONG prev = 0;
    memset(&im, 0, sizeof im);
    im.Class = IDCMP_RAWKEY;
    im.Code = code;
    im.IAddress = &prev;
    DoMethod(o, MUIM_HandleEvent, (IPTR)&im, 0);
    im.Code = code | IECODE_UP_PREFIX;
    DoMethod(o, MUIM_HandleEvent, (IPTR)&im, 0);
}

/* The clipboard's UTF8 chunk into buf; its length or -1. */
static LONG clip_utf8(char *buf, LONG max)
{
    struct Library *IFFParseBase = OpenLibrary("iffparse.library", 39);
    struct IFFHandle *iff;
    LONG got = -1;
    if (!IFFParseBase)
        return -1;
    if ((iff = AllocIFF())) {
        if ((iff->iff_Stream = (IPTR)OpenClipboard(0))) {
            InitIFFasClip(iff);
            if (!OpenIFF(iff, IFFF_READ)) {
                if (!StopChunk(iff, MAKE_ID('F','T','X','T'), MAKE_ID('U','T','F','8')) && !ParseIFF(iff, IFFPARSE_SCAN))
                    got = ReadChunkBytes(iff, buf, max);
                CloseIFF(iff);
            }
            CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
        }
        FreeIFF(iff);
    }
    CloseLibrary(IFFParseBase);
    return got;
}

int main(int argc, char **argv)
{
    Object *app, *win, *u8, *old, *limited;
    IPTR status = 99, open = 0;
    BOOL clip = argc > 1 && !strcmp(argv[1], "CLIP");
    char buf[256];

    setvbuf(stdout, NULL, _IONBF, 0);       /* every line out at once */

    app = ApplicationObject,
        MUIA_Application_Title, (IPTR)"BetterStringUTF8",
        SubWindow, (IPTR)(win = WindowObject,
            MUIA_Window_Title, (IPTR)"BetterString UTF-8",
            WindowContents, (IPTR)(VGroup,
                Child, (IPTR)(u8 = BetterStringObject, StringFrame,
                    MUIA_BetterString_UTF8, TRUE, MUIA_String_Contents, (IPTR)ZOLC, End),
                Child, (IPTR)(limited = BetterStringObject, StringFrame,
                    MUIA_BetterString_UTF8, TRUE, MUIA_String_MaxLen, 7, End),
                Child, (IPTR)(old = BetterStringObject, StringFrame,
                    MUIA_String_Contents, (IPTR)"ab\xb1", End),
            End),
        End),
    End;
    if (!app) {
        printf("FAIL the objects could not be made\n");
        return 20;
    }

    get(u8, MUIA_BetterString_UTF8Status, &status);
    if (status != 0) {
        printf("FAIL the UTF-8 mode cannot draw here (status %ld: %s)\n", (long)status,
               status == 1 ? "no unitext.library or freetype2.library" :
               status == 2 ? "no font for Unicode text" : "no memory");
        MUI_DisposeObject(app);
        return 20;
    }
    printf("..   opening the window\n");
    set(win, MUIA_Window_Open, TRUE);
    get(win, MUIA_Window_Open, &open);
    check(open, "the window opens with a UTF-8 field and an 8-bit field");
    if (!open) {
        MUI_DisposeObject(app);
        return 20;
    }

    check(!strcmp(contents(u8), ZOLC), "UTF-8 contents kept as given");

    set(u8, MUIA_String_Contents, (IPTR)"bad \xff text");
    check(!strcmp(contents(u8), ZOLC), "contents that are not UTF-8 refused, the old ones kept");

    set(u8, MUIA_String_BufferPos, 3);     /* inside the two bytes of z-dot */
    check(pos(u8) == 2, "a position inside a character moves back to its start");

    set(u8, MUIA_String_Contents, (IPTR)("x" FAMILY "y"));
    set(u8, MUIA_String_BufferPos, 5);     /* inside the emoji sequence */
    check(pos(u8) == 1, "a position inside an emoji sequence moves back to its start");

    set(limited, MUIA_String_Contents, (IPTR)("ab" FAMILY));
    check(!strcmp(contents(limited), "ab"), "MaxLen cuts at a cluster boundary, not inside the sequence");

    set(u8, MUIA_String_Contents, (IPTR)"ab");
    set(u8, MUIA_String_BufferPos, 1);
    check(DoMethod(u8, MUIM_BetterString_Insert, (IPTR)E_ACUTE, MUIV_BetterString_Insert_BufferPos) &&
          !strcmp(contents(u8), "a" E_ACUTE "b"), "Insert puts UTF-8 in at the position");
    check(!DoMethod(u8, MUIM_BetterString_Insert, (IPTR)"\xc3", MUIV_BetterString_Insert_BufferPos) &&
          !strcmp(contents(u8), "a" E_ACUTE "b"), "Insert refuses text that is not UTF-8");

    /* keys: the field active, then cursor right and Backspace by clusters */
    printf("..   keys\n");
    set(u8, MUIA_String_Contents, (IPTR)("a" FAMILY E_ACUTE));
    set(win, MUIA_Window_ActiveObject, (IPTR)u8);
    set(u8, MUIA_String_BufferPos, 0);
    key(u8, RAWKEY_RIGHT);
    key(u8, RAWKEY_RIGHT);
    check(pos(u8) == 12, "cursor right steps over the whole emoji sequence");
    key(u8, RAWKEY_RIGHT);
    check(pos(u8) == 15, "cursor right steps over e with its combining accent");
    key(u8, RAWKEY_BACKSPACE);
    check(!strcmp(contents(u8), "a" FAMILY) && pos(u8) == 12, "Backspace removes e with its accent together");
    key(u8, RAWKEY_BACKSPACE);
    check(!strcmp(contents(u8), "a") && pos(u8) == 1, "Backspace removes the emoji sequence as one");

    /* the 8-bit field: bytes as before */
    set(win, MUIA_Window_ActiveObject, (IPTR)old);
    set(old, MUIA_String_BufferPos, 3);
    key(old, RAWKEY_BACKSPACE);
    check(!strcmp(contents(old), "ab"), "the 8-bit field still removes one byte");
    set(old, MUIA_String_Contents, (IPTR)"\xff\xfe");
    check(!strcmp(contents(old), "\xff\xfe"), "the 8-bit field still takes any bytes");

    if (clip) {
        LONG n;
        set(u8, MUIA_String_Contents, (IPTR)(ZOLC " " FAMILY));
        set(win, MUIA_Window_ActiveObject, (IPTR)u8);
        DoMethod(u8, MUIM_BetterString_DoAction, MUIV_BetterString_DoAction_SelectAll);
        DoMethod(u8, MUIM_BetterString_DoAction, MUIV_BetterString_DoAction_Copy);
        n = clip_utf8(buf, sizeof buf - 1);
        if (n >= 0) buf[n] = 0;
        check(n >= 0 && !strcmp(buf, ZOLC " " FAMILY), "copy puts the text as a UTF8 chunk");
        set(limited, MUIA_String_Contents, (IPTR)"");
        set(win, MUIA_Window_ActiveObject, (IPTR)limited);
        DoMethod(limited, MUIM_BetterString_DoAction, MUIV_BetterString_DoAction_Paste);
        check(!strcmp(contents(limited), "Za\xc5\xbc\xc3\xb3"), "paste takes UTF-8 and cuts at MaxLen on a boundary");
    }

    printf("..   closing\n");
    set(win, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);
    if (failed)
        printf("FAIL %d of %d\n", failed, checks);
    else
        printf("PASS %d\n", checks);
    return failed ? 10 : 0;
}

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: TextEditor.mcc in its UTF-8 mode (MUIA_TextEditor_UTF8), beside
          an 8-bit TextEditor in the same window.

    TextEditorUTF8 [CLIP] [WAIT]

    Opens a window with two UTF-8 editors and an 8-bit one and checks,
    without user input: UTF-8 contents kept as given, text that is not
    UTF-8 refused, positions moved to grapheme cluster boundaries, cursor
    keys, Backspace and Delete by clusters (key events sent to the
    editor), a typed key and its undo and redo, a key replacing a
    selection, word moves over Polish words, case conversion of ASCII
    letters only, and the 8-bit editor unchanged by any of it.
    With CLIP it also copies and pastes through the clipboard (unit 0,
    whose contents it replaces): the UTF8 chunks of a copy, a paste into
    the second UTF-8 editor, and a paste of a clip that another program
    wrote with its text in a UTF8 chunk after the CHRS copy.
    With WAIT the window stays open afterwards, for typing by hand, until
    it is closed.

    Prints one line per check, then "PASS n" or "FAIL n of m"; the return
    code is 0 or 10. It writes no files and can run again at once.
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
#include <mui/TextEditor_mcc.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>

#ifndef MAKE_ID
#define MAKE_ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (ULONG)(c) << 8 | (ULONG)(d))
#endif

#define ZOLC    "Za\xc5\xbc\xc3\xb3\xc5\x82\xc4\x87"              /* Zażółć */
#define GESLA   "g\xc4\x99\xc5\x9bl\xc4\x85"                       /* gęślą */
#define GREEK   "\xce\xb1\xce\xb2\xce\xb3"                         /* αβγ */
#define E_ACUTE "e\xcc\x81"                                        /* e + U+0301 */
#define FAMILY  "\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa7"     /* woman ZWJ girl */

static int checks, failed;

static void check(int ok, const char *what)
{
    checks++;
    if (!ok)
        failed++;
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
}

/* Whether the editor's text, without a last line feed, is want. */
static BOOL text_is(Object *o, const char *want)
{
    STRPTR t = (STRPTR)DoMethod(o, MUIM_TextEditor_ExportText);
    BOOL same = FALSE;
    if (t != NULL)
    {
        size_t n = strlen((char *)t);
        if (n > 0 && t[n - 1] == '\n')
            t[--n] = '\0';
        same = !strcmp((char *)t, want);
        FreeVec(t);
    }
    return same;
}

static LONG cx(Object *o)
{
    IPTR x = 0;
    get(o, MUIA_TextEditor_CursorX, &x);
    return (LONG)x;
}

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

static void cmd(Object *o, const char *c)
{
    DoMethod(o, MUIM_TextEditor_ARexxCmd, (IPTR)c);
}

static void at(Object *win, Object *o, const char *text, LONG x)
{
    set(o, MUIA_TextEditor_Contents, (IPTR)text);
    set(win, MUIA_Window_ActiveObject, (IPTR)o);
    set(o, MUIA_TextEditor_CursorX, x);
}

/* The clipboard's UTF8 chunks, one after another, into buf; their length
   or -1. */
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
                StopChunk(iff, MAKE_ID('F','T','X','T'), MAKE_ID('U','T','F','8'));
                got = 0;
                for (;;) {
                    LONG e = ParseIFF(iff, IFFPARSE_SCAN), n;
                    if (e == IFFERR_EOC)
                        continue;
                    if (e)
                        break;
                    n = ReadChunkBytes(iff, buf + got, max - got);
                    if (n > 0)
                        got += n;
                }
                CloseIFF(iff);
            }
            CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
        }
        FreeIFF(iff);
    }
    CloseLibrary(IFFParseBase);
    return got;
}

/* A clip as other programs write it: FTXT with CHRS, then UTF8. */
static BOOL clip_foreign(const char *chrs, const char *utf8)
{
    struct Library *IFFParseBase = OpenLibrary("iffparse.library", 39);
    struct IFFHandle *iff;
    BOOL ok = FALSE;
    if (!IFFParseBase)
        return FALSE;
    if ((iff = AllocIFF())) {
        if ((iff->iff_Stream = (IPTR)OpenClipboard(0))) {
            InitIFFasClip(iff);
            if (!OpenIFF(iff, IFFF_WRITE)) {
                ok = !PushChunk(iff, MAKE_ID('F','T','X','T'), MAKE_ID('F','O','R','M'), IFFSIZE_UNKNOWN)
                  && !PushChunk(iff, 0, MAKE_ID('C','H','R','S'), IFFSIZE_UNKNOWN)
                  && WriteChunkBytes(iff, (APTR)chrs, strlen(chrs)) == (LONG)strlen(chrs)
                  && !PopChunk(iff)
                  && !PushChunk(iff, 0, MAKE_ID('U','T','F','8'), IFFSIZE_UNKNOWN)
                  && WriteChunkBytes(iff, (APTR)utf8, strlen(utf8)) == (LONG)strlen(utf8)
                  && !PopChunk(iff) && !PopChunk(iff);
                CloseIFF(iff);
            }
            CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
        }
        FreeIFF(iff);
    }
    CloseLibrary(IFFParseBase);
    return ok;
}

int main(int argc, char **argv)
{
    Object *app, *win, *u8, *u8b, *old;
    IPTR status = 99, open = 0, mode = 0;
    BOOL clip = FALSE, wait = FALSE;
    char buf[512];
    int a;

    for (a = 1; a < argc; a++) {
        if (!strcmp(argv[a], "CLIP"))
            clip = TRUE;
        else if (!strcmp(argv[a], "WAIT"))
            wait = TRUE;
    }
    setvbuf(stdout, NULL, _IONBF, 0);

    app = ApplicationObject,
        MUIA_Application_Title, (IPTR)"TextEditorUTF8",
        SubWindow, (IPTR)(win = WindowObject,
            MUIA_Window_Title, (IPTR)"TextEditor UTF-8",
            MUIA_Window_Width, MUIV_Window_Width_Visible(50),
            WindowContents, (IPTR)(VGroup,
                Child, (IPTR)(u8 = TextEditorObject, MUIA_TextEditor_UTF8, TRUE,
                    MUIA_TextEditor_Contents, (IPTR)(ZOLC " " GESLA), End),
                Child, (IPTR)(u8b = TextEditorObject, MUIA_TextEditor_UTF8, TRUE, End),
                Child, (IPTR)(old = TextEditorObject,
                    MUIA_TextEditor_Contents, (IPTR)"ab\xb1", End),
            End),
        End),
    End;
    if (!app) {
        printf("FAIL the objects could not be made\n");
        return 20;
    }

    get(u8, MUIA_TextEditor_UTF8, &mode);
    get(u8, MUIA_TextEditor_UTF8Status, &status);
    if (!mode || status != 0) {
        printf("FAIL the UTF-8 mode is %s (status %ld: %s)\n", mode ? "on but cannot draw" : "not there",
               (long)status, status == 1 ? "no unitext.library or freetype2.library" :
               status == 2 ? "no font for Unicode text" : status == 3 ? "no memory" : "unknown attribute");
        MUI_DisposeObject(app);
        return 20;
    }
    printf("..   opening the window\n");
    set(win, MUIA_Window_Open, TRUE);
    get(win, MUIA_Window_Open, &open);
    check(open, "the window opens with two UTF-8 editors and an 8-bit one");
    if (!open) {
        MUI_DisposeObject(app);
        return 20;
    }

    check(text_is(u8, ZOLC " " GESLA), "UTF-8 contents kept as given");
    set(u8, MUIA_TextEditor_Contents, (IPTR)"bad \xff text");
    check(text_is(u8, ZOLC " " GESLA), "contents that are not UTF-8 refused, the old ones kept");
    check(!DoMethod(u8, MUIM_TextEditor_InsertText, (IPTR)"\xc3", MUIV_TextEditor_InsertText_Cursor)
          && text_is(u8, ZOLC " " GESLA), "InsertText refuses text that is not UTF-8");

    at(win, u8, ZOLC, 3);                       /* inside the two bytes of z-dot */
    check(cx(u8) == 2, "a position inside a character moves back to its start");

    /* cursor keys, Backspace and Delete by clusters */
    printf("..   keys\n");
    at(win, u8, "a" FAMILY E_ACUTE, 0);
    key(u8, RAWKEY_RIGHT);
    key(u8, RAWKEY_RIGHT);
    check(cx(u8) == 12, "cursor right steps over the whole emoji sequence");
    key(u8, RAWKEY_RIGHT);
    check(cx(u8) == 15, "cursor right steps over e with its combining accent");
    key(u8, RAWKEY_LEFT);
    check(cx(u8) == 12, "cursor left steps back over e with its accent");
    key(u8, RAWKEY_RIGHT);
    key(u8, RAWKEY_BACKSPACE);
    check(text_is(u8, "a" FAMILY) && cx(u8) == 12, "Backspace removes e with its accent together");
    key(u8, RAWKEY_BACKSPACE);
    check(text_is(u8, "a") && cx(u8) == 1, "Backspace removes the emoji sequence as one");
    at(win, u8, "x" FAMILY "y", 1);
    key(u8, RAWKEY_DELETE);
    check(text_is(u8, "xy") && cx(u8) == 1, "Delete removes the emoji sequence as one");

    /* typing, undo and redo */
    at(win, u8, ZOLC, 10);
    key(u8, RAWKEY_W);
    check(text_is(u8, ZOLC "w") && cx(u8) == 11, "a typed key goes in after the last character");
    cmd(u8, "UNDO");
    check(text_is(u8, ZOLC), "undo takes the typed key out");
    cmd(u8, "REDO");
    check(text_is(u8, ZOLC "w"), "redo puts it back");

    /* a key over a selection */
    at(win, u8, ZOLC, 0);
    DoMethod(u8, MUIM_TextEditor_MarkText, 2, 0, 6, 0);    /* z-dot and o-acute */
    set(win, MUIA_Window_ActiveObject, (IPTR)u8);
    key(u8, RAWKEY_W);
    check(text_is(u8, "Zaw\xc5\x82\xc4\x87"), "a typed key replaces the selection");
    cmd(u8, "UNDO");
    cmd(u8, "UNDO");
    check(text_is(u8, ZOLC), "undo brings the selected text back");

    /* words */
    at(win, u8, ZOLC " " GESLA " " GREEK, 0);
    cmd(u8, "NEXT WORD");
    check(cx(u8) == 11, "the next word starts after the Polish word, not inside it");
    cmd(u8, "NEXT WORD");
    check(cx(u8) == 20, "and the next one at the Greek word");
    cmd(u8, "PREVIOUS WORD");
    check(cx(u8) == 11, "the previous word is the Polish one again");

    /* case conversion: ASCII letters only */
    at(win, u8, "\xc5\xbc" "aba", 0);
    cmd(u8, "SELECTALL");
    cmd(u8, "TOUPPER");
    check(text_is(u8, "\xc5\xbc" "ABA"), "upper case changes ASCII letters and leaves the others whole");

    /* the 8-bit editor: bytes as before */
    at(win, old, "ab\xb1", 3);
    key(old, RAWKEY_BACKSPACE);
    check(text_is(old, "ab"), "the 8-bit editor still removes one byte");
    set(old, MUIA_TextEditor_Contents, (IPTR)"\xff\xfe");
    check(text_is(old, "\xff\xfe"), "the 8-bit editor still takes any bytes");

    if (clip) {
        LONG n;
        printf("..   clipboard\n");
        at(win, u8, ZOLC "\n" GREEK " " FAMILY, 0);
        cmd(u8, "SELECTALL");
        cmd(u8, "COPY");
        n = clip_utf8(buf, sizeof buf - 1);
        if (n >= 0) {
            buf[n] = 0;
            if (n > 0 && buf[n - 1] == '\n')
                buf[--n] = 0;
        }
        check(n >= 0 && !strcmp(buf, ZOLC "\n" GREEK " " FAMILY), "copy writes the lines as UTF8 chunks");
        set(u8b, MUIA_TextEditor_Contents, (IPTR)"");
        set(win, MUIA_Window_ActiveObject, (IPTR)u8b);
        cmd(u8b, "PASTE");
        check(text_is(u8b, ZOLC "\n" GREEK " " FAMILY), "paste into another UTF-8 editor gives the same text");
        check(clip_foreign("Za?o?? ?", ZOLC " " FAMILY), "a clip as other programs write it (CHRS, then UTF8)");
        set(u8b, MUIA_TextEditor_Contents, (IPTR)"");
        cmd(u8b, "PASTE");
        check(text_is(u8b, ZOLC " " FAMILY), "its UTF8 text is pasted, not the 8-bit copy");
    }

    if (wait) {
        ULONG sigs = 0;
        set(u8, MUIA_TextEditor_Contents, (IPTR)(ZOLC " " GESLA "\n" GREEK " " E_ACUTE " " FAMILY "\n"
            "\xd0\x96\xd0\xb8\xd0\xb7\xd0\xbd\xd1\x8c " ZOLC " " GESLA " " GREEK " " ZOLC " " GESLA " "
            GREEK " " ZOLC " " GESLA " " GREEK " " ZOLC " " GESLA " " GREEK " " ZOLC " " GESLA " "
            GREEK " " ZOLC " " GESLA " " GREEK " " ZOLC " " GESLA " " GREEK " " ZOLC " " GESLA));
        /* bold over the Polish words, a colour over the Greek one, a
           selection over the second line */
        DoMethod(u8, MUIM_TextEditor_SetBlock, 0, 0, 19, 0, MUIF_TextEditor_SetBlock_StyleBold, TRUE);
        DoMethod(u8, MUIM_TextEditor_SetBlock, 0, 1, 6, 1, MUIF_TextEditor_SetBlock_Color, 7);
        DoMethod(u8, MUIM_TextEditor_SetBlock, 7, 1, 10, 1, MUIF_TextEditor_SetBlock_StyleUnderline, TRUE);
        DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2,
                 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        printf("..   waiting: close the window to end\n");
        while (DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs) != MUIV_Application_ReturnID_Quit) {
            if (sigs && (sigs = Wait(sigs | SIGBREAKF_CTRL_C)) & SIGBREAKF_CTRL_C)
                break;
        }
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

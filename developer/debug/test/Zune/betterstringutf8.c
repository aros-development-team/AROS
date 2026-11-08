/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BetterString.mcc in its UTF-8 mode (MUIA_BetterString_UTF8),
          beside an 8-bit BetterString in the same window.

    BetterStringUTF8 [CLIP] [WAIT]

    Opens a window with both fields and checks, without user input:
    UTF-8 contents kept as given, text that is not UTF-8 refused, positions
    moved to grapheme cluster boundaries, MaxLen cutting at a boundary,
    MUIM_BetterString_Insert, cursor keys and Backspace by clusters (key
    events sent to the field), a typed key replacing a selection whole or
    not at all (Reject, MaxLen), a secret UTF-8 field drawn and hit by the
    mouse, drawing and mouse hits with each of the class's allocations
    failing in turn, and the 8-bit field unchanged by any of it.

    The allocation failures come from AllocVec patched (SetFunction) for
    this task only, and only for the n-th call from a chosen moment; the
    patch is taken out again before the window closes.

    With WAIT the window stays open after the checks, its fields filled
    with Polish, Greek, a combining accent and an emoji sequence, until it
    is closed: for typing by hand, and for several programs using the
    UTF-8 mode, and unitext.library, at once.
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
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/iffparse.h>
#include <proto/utility.h>
#include <aros/asmcall.h>
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

/* AllocVec failing for this task's n-th call from now (fail_in = n), all
   other calls and tasks passing through */
static APTR old_allocvec;
static struct Task *me;
static volatile LONG fail_in = -1, failures;

AROS_UFH3(APTR, FailingAllocVec,
    AROS_UFHA(IPTR, size, D0),
    AROS_UFHA(ULONG, req, D1),
    AROS_UFHA(struct ExecBase *, sysbase, A6))
{
    AROS_USERFUNC_INIT

    if(FindTask(NULL) == me && fail_in >= 0 && fail_in-- == 0)
    {
        failures++;
        return NULL;
    }
    return AROS_UFC3(APTR, old_allocvec,
        AROS_UFCA(IPTR, size, D0),
        AROS_UFCA(ULONG, req, D1),
        AROS_UFCA(struct ExecBase *, sysbase, A6));

    AROS_USERFUNC_EXIT
}

static void patch(BOOL on)
{
    Forbid();
    if(on)
        old_allocvec = SetFunction((struct Library *)SysBase, -114 * LIB_VECTSIZE, (APTR)AROS_ASMSYMNAME(FailingAllocVec));
    else
        SetFunction((struct Library *)SysBase, -114 * LIB_VECTSIZE, old_allocvec);
    Permit();
}

/* Whether s is well-formed UTF-8 (shortest forms, no surrogates, at most
   U+10FFFF). */
static BOOL valid_utf8(const UBYTE *s)
{
    while(*s)
    {
        ULONG c = *s, n, k, cp;
        if(c < 0x80) { s++; continue; }
        if(c >= 0xc2 && c <= 0xdf) { n = 1; cp = c & 0x1f; }
        else if(c >= 0xe0 && c <= 0xef) { n = 2; cp = c & 0x0f; }
        else if(c >= 0xf0 && c <= 0xf4) { n = 3; cp = c & 0x07; }
        else return FALSE;
        for(k = 1; k <= n; k++)
        {
            if((s[k] & 0xc0) != 0x80)
                return FALSE;
            cp = (cp << 6) | (s[k] & 0x3f);
        }
        if((n == 2 && (cp < 0x800 || (cp >= 0xd800 && cp <= 0xdfff))) || (n == 3 && (cp < 0x10000 || cp > 0x10ffff)))
            return FALSE;
        s += n + 1;
    }
    return TRUE;
}

static LONG status_of(Object *o)
{
    IPTR st = 99;
    get(o, MUIA_BetterString_UTF8Status, &st);
    return (LONG)st;
}

static LONG selsize(Object *o)
{
    IPTR n = 0;
    get(o, MUIA_BetterString_SelectSize, &n);
    return (LONG)n;
}

/* A click at x pixels into the field's text area. */
static void click(Object *o, LONG x, ULONG secs)
{
    struct IntuiMessage im;
    memset(&im, 0, sizeof im);
    im.Class = IDCMP_MOUSEBUTTONS;
    im.Code = IECODE_LBUTTON;
    im.MouseX = _mleft(o) + x;
    im.MouseY = _mtop(o) + 1;
    im.Seconds = secs;
    DoMethod(o, MUIM_HandleEvent, (IPTR)&im, 0);
    im.Code = IECODE_LBUTTON | IECODE_UP_PREFIX;
    DoMethod(o, MUIM_HandleEvent, (IPTR)&im, 0);
}

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
    Object *app, *win, *u8, *old, *limited, *secret;
    IPTR status = 99, open = 0;
    BOOL clip = FALSE, wait = FALSE;
    int a;

    for(a = 1; a < argc; a++)
    {
        if(!strcmp(argv[a], "CLIP"))
            clip = TRUE;
        else if(!strcmp(argv[a], "WAIT"))
            wait = TRUE;
    }
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
                Child, (IPTR)(secret = BetterStringObject, StringFrame,
                    MUIA_BetterString_UTF8, TRUE, MUIA_String_Secret, TRUE,
                    MUIA_String_Contents, (IPTR)ZOLC, End),
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

    /* a typed key in place of a selection: whole or not at all */
    printf("..   replacing a selection\n");
    set(u8, MUIA_String_Reject, (IPTR)"q");
    set(u8, MUIA_String_Contents, (IPTR)ZOLC);
    set(win, MUIA_Window_ActiveObject, (IPTR)u8);
    set(u8, MUIA_String_BufferPos, 2);
    set(u8, MUIA_BetterString_SelectSize, 4);             /* z-dot and o-acute */
    key(u8, RAWKEY_Q);
    check(!strcmp(contents(u8), ZOLC) && selsize(u8) == 4,
          "a key Reject refuses leaves the text and the selection as they were");
    set(u8, MUIA_String_Reject, 0);
    set(u8, MUIA_String_BufferPos, 2);
    set(u8, MUIA_BetterString_SelectSize, 4);
    key(u8, RAWKEY_W);
    check(!strcmp(contents(u8), "Zaw\xc5\x82\xc4\x87") && pos(u8) == 3, "an allowed key replaces the selection");

    set(win, MUIA_Window_ActiveObject, (IPTR)limited);
    set(limited, MUIA_String_Contents, (IPTR)"ab\xc5\xbc\xc3\xb3");  /* 6 bytes: full */
    set(limited, MUIA_String_BufferPos, 6);
    key(limited, RAWKEY_W);
    check(!strcmp(contents(limited), "ab\xc5\xbc\xc3\xb3"), "a key past MaxLen is refused");
    set(limited, MUIA_String_BufferPos, 4);
    set(limited, MUIA_BetterString_SelectSize, 2);         /* o-acute */
    key(limited, RAWKEY_W);
    check(!strcmp(contents(limited), "ab\xc5\xbcw"), "a key that fits once the selection goes replaces it");

    /* a secret field: one '*' a cluster, the mouse hits clusters */
    printf("..   secret field\n");
    set(secret, MUIA_String_Contents, (IPTR)("a" FAMILY E_ACUTE));
    set(win, MUIA_Window_ActiveObject, (IPTR)secret);
    set(secret, MUIA_String_BufferPos, 0);
    MUI_Redraw(secret, MADF_DRAWOBJECT);
    check(status_of(secret) == 0, "a secret UTF-8 field draws");
    click(secret, _mwidth(secret) - 1, 10);
    check(pos(secret) == 15, "a click past the end of a secret field puts the cursor at the end");

    /* each allocation of the drawing and of a mouse hit failing in turn */
    printf("..   allocations failing\n");
    me = FindTask(NULL);
    patch(TRUE);
    {
        LONG k, marked = 0, kept = 0, tries = 0;
        Object *f[2];
        f[0] = secret; f[1] = u8;
        set(u8, MUIA_String_Contents, (IPTR)ZOLC);
        for(k = 0; k < 2; k++)
        {
            LONG n;
            for(n = 0; n < 4; n++)
            {
                failures = 0;
                fail_in = n;
                MUI_Redraw(f[k], MADF_DRAWOBJECT);
                fail_in = -1;
                tries++;
                if(failures && status_of(f[k]) == 3)
                    marked++;
                MUI_Redraw(f[k], MADF_DRAWOBJECT);
                if(status_of(f[k]) != 0)
                    marked = -100;
            }
        }
        check(marked >= 3 && !strcmp(contents(u8), ZOLC) && !strcmp(contents(secret), "a" FAMILY E_ACUTE),
              "a drawing without memory says so (status 3), keeps the text, and the next one draws (status 0)");
        printf("..   %ld of %ld drawings failed and said so\n", (long)marked, (long)tries);
        for(k = 0; k < 3; k++)
        {
            set(secret, MUIA_String_BufferPos, 0);
            failures = 0;
            fail_in = k;
            click(secret, _mwidth(secret) - 1, 20 + 2 * k);
            fail_in = -1;
            if(failures && (pos(secret) == 0 || pos(secret) == 15))
                kept++;
        }
        check(kept >= 2, "a mouse hit without memory leaves the cursor or finds the cluster, never crashes");
    }
    patch(FALSE);
    Delay(2);                       /* a call another task is in leaves the patch */

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

    if(wait)
    {
        ULONG sigs = 0;

        set(u8, MUIA_String_Contents, (IPTR)(ZOLC " \xce\xb1\xce\xb2\xce\xb3 " E_ACUTE " " FAMILY));
        set(limited, MUIA_String_Contents, (IPTR)"");
        set(secret, MUIA_String_Contents, (IPTR)ZOLC);
        DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2,
                 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        printf("..   waiting: close the window to end\n");
        while(DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs) != MUIV_Application_ReturnID_Quit)
        {
            if(sigs && (sigs = Wait(sigs | SIGBREAKF_CTRL_C)) & SIGBREAKF_CTRL_C)
                break;
        }
        check(valid_utf8((const UBYTE *)contents(u8)), "the field typed into by hand still holds UTF-8");
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

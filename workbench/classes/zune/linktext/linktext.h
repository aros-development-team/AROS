/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* Linktext: read-only wrapped text with links inside its paragraphs.
 *
 * The application gives plain text and explicit spans. A span covers bytes
 * of that text and either makes them a link (it has a target) or only sets
 * their style. The class wraps, scrolls, draws with the user's font and
 * pens, selects by dragging, and reports what the user asked of a link. It
 * opens nothing itself: no browser, no Shell, no network.
 *
 * Mouse: a click on a link opens it, a drag selects without opening, the
 * context menu opens or copies the link under the pointer, and a bubble shows
 * its whole target. Keyboard, when the object is active: Left and Right go
 * from link to link, Return opens, Amiga+C copies the link or the selection,
 * Up, Down and the page keys scroll, Tab leaves the object.
 *
 * Text is one byte per character in the system's 8-bit character set, as
 * for Text() and Floattext. Offsets count those bytes. UTF-8 must be
 * converted first. A newline ends a paragraph; any other control character
 * is shown as a space. No escape codes are interpreted.
 *
 * Attribute and method numbers are a proposal until the class has a
 * reserved range. */
#ifndef ZUNE_LINKTEXT_H
#define ZUNE_LINKTEXT_H

#include <exec/types.h>
#include <libraries/mui.h>

#define MUIC_Linktext "Linktext.mcc"

#define MUIB_Linktext (MUIB_AROS | 0x00000C00)

/* [ISG] struct Hook *: called for each action, see below. */
#define MUIA_Linktext_ActionHook (MUIB_Linktext | 0x01)
/* [I.G] ULONG: lines to ask for. Default 4. */
#define MUIA_Linktext_Lines      (MUIB_Linktext | 0x02)
/* [I..] BOOL: exactly MUIA_Linktext_Lines high instead of stretching. */
#define MUIA_Linktext_FixHeight  (MUIB_Linktext | 0x03)
/* [I..] Object *: a Scrollbar or Prop to keep in step, as
   MUIA_TextEditor_Slider. Not disposed by this class. */
#define MUIA_Linktext_Scroller   (MUIB_Linktext | 0x04)
/* [.SG] ULONG: the wrapped line at the top. */
#define MUIA_Linktext_First      (MUIB_Linktext | 0x05)
/* [..G] ULONG: wrapped lines of the current width, 0 before the first show. */
#define MUIA_Linktext_LineCount  (MUIB_Linktext | 0x06)

/* Replaces the whole content, at once or not at all; returns a
   MUIV_Linktext_Status_*. On any failure the content shown before stays
   and nothing changes. The object copies text and targets during the call. */
#define MUIM_Linktext_SetContent (MUIB_Linktext | 0x10)
struct MUIP_Linktext_SetContent
{
    STACKED ULONG MethodID;
    STACKED CONST_STRPTR Text;                 /* NULL: empty                */
    STACKED ULONG Length;                      /* ~0UL: up to the first NUL  */
    STACKED const struct MUI_LinktextSpan *Spans;
    STACKED ULONG SpanCount;
};

/* Where a link is on screen now: fragment 0, 1... of link `Link` (counted
   in text order, style spans not counted) goes to *Box in window
   coordinates. TRUE when that fragment exists and is at least partly
   visible. For tests and for placing a hint. */
#define MUIM_Linktext_LinkBox    (MUIB_Linktext | 0x11)
struct MUIP_Linktext_LinkBox
{
    STACKED ULONG MethodID;
    STACKED ULONG Link;
    STACKED ULONG Fragment;
    STACKED struct IBox *Box;
};

#define MUIV_Linktext_Status_OK       0
#define MUIV_Linktext_Status_Range    1  /* a span outside the text, or empty */
#define MUIV_Linktext_Status_Order    2  /* spans overlap or are not in order */
#define MUIV_Linktext_Status_Target   3  /* a link without a target text      */
#define MUIV_Linktext_Status_TooLarge 4  /* over a limit below                */
#define MUIV_Linktext_Status_NoMemory 5

/* Limits of one content; over them SetContent says TooLarge. */
#define LINKTEXT_MAX_TEXT   (64UL * 1024 * 1024)
#define LINKTEXT_MAX_SPANS  1000000UL
#define LINKTEXT_MAX_TARGET (16UL * 1024 * 1024)

#define MUIV_Linktext_Span_Bold 1

struct MUI_LinktextSpan
{
    ULONG Start;          /* byte offset into the text                       */
    ULONG Length;         /* bytes, at least 1                               */
    ULONG Flags;          /* MUIV_Linktext_Span_*                            */
    CONST_STRPTR Target;  /* NUL-terminated, exact, not parsed; NULL: style  */
    IPTR ID;              /* the application's, handed back in actions       */
};

/* Spans come in text order and do not overlap. Several links may share a
   target or an ID. */

#define MUIV_Linktext_Action_Open 1  /* click, Return, menu                  */
#define MUIV_Linktext_Action_Copy 2  /* menu, copy key on a focused link     */

/* What the hook gets as its message. The hook runs on the application's
   task, inside the object's event handling. Target stays valid until the
   hook returns, even if the hook replaces the content; copy it to keep it.
   The hook may replace the content and close the window. To dispose the
   object, use MUIM_Application_PushMethod.
   Return TRUE when handled. For Copy, FALSE makes the class copy Target
   to clipboard unit 0 itself. */
struct MUI_LinktextAction
{
    ULONG Action;
    ULONG Link;            /* counted as in MUIM_Linktext_LinkBox */
    IPTR ID;
    CONST_STRPTR Target;
    ULONG TargetLength;
    ULONG Qualifier;       /* of the click or key                 */
};

#define LinktextObject MUIOBJMACRO_START(MUIC_Linktext)

#endif

/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "floattext_private.h"

/****** Floattext.mui/MUIC_Floattext ************
*
*   NAME
*       MUIC_Floattext
*
*   SUPERCLASS
*       List.mui
*
*   DESCRIPTION
*       Wrap text into List entries using the native window RastPort's font and
*       the object's width minus inner margins and eight pixels. Newlines
*       delimit paragraphs. Text is copied; SkipChars is borrowed. Defaults are
*       no source text, no skip characters, Justify FALSE and TabSize 8.
*       Construction installs List's string construction/destruction hooks,
*       replacing caller hook selections. Preserve these hooks while the class
*       manages its generated lines.
*       Each typesetting pass clears the list and rebuilds its entries. Without
*       a window or source text it leaves the list empty. Full Draw retypesets
*       when the outer width differs from the cached width; a font or margin
*       change alone does not satisfy that width test. Setting any Floattext
*       attribute also retypesets, even if the value is unchanged. Text source
*       storage and generated line copies have separate ownership. Disposal
*       frees the source and delegates list cleanup to List.
*
*   METHODS
*       The formatter expands tabs to a fixed number of spaces, filters
*       SkipChars and carries two-byte escape commands across wrapped lines
*       within a paragraph. It is not a general parser for all multi-byte Zune
*       escape forms. It fits text with TextFit and prefers spaces as break
*       points. Justification adds whole spaces; even the last paragraph line
*       can be justified unless its computed space multiplier exceeds five.
*       Draw suppresses superclass drawing while typesetting and always returns
*       zero. Typesetting clears its suppression flag and redraws the list only
*       on normal completion. Temporary paragraph/line allocation failures
*       return early with the flag still set and can leave an empty or partially
*       rebuilt list. List insertion results are ignored; success of
*       construction, set or Append does not establish that every line was
*       generated.
*
*   BUGS
*       The formatter uses UWORD positions, counts and allocation sizes, and
*       unchecked arithmetic. Long paragraphs, tab expansion or accumulated
*       escape commands can truncate lengths and overrun buffers. Incomplete
*       escapes can read beyond input. A nonempty paragraph for which TextFit
*       returns zero without advancing the source, including an entirely
*       filtered paragraph, can cause a loop without progress. No cancellation
*       or timeout is provided. Justification assumes a nonzero measured space
*       width.
*       Setting Text frees the old source before copying the replacement. Never
*       pass the current Text getter pointer or a pointer into it to that
*       setter. Append copies into new storage before freeing the old source,
*       but its ULONG allocation-size calculation is unchecked. These
*       limitations describe this implementation, not guaranteed safe
*       arbitrary-input handling.
*
*/
/****** Floattext.mui/MUIA_Floattext_Text ************
*
*   NAME
*       MUIA_Floattext_Text
*
*   FUNCTION
*       Replace or retrieve the source text used for wrapping.
*
*   INPUTS
*       Initialize, set and get, STRPTR. Default NULL. Supply a readable
*       NUL-terminated string; NULL clears the source.
*
*   NOTES
*       The class owns a StrDup copy. Get returns a borrowed pointer valid until
*       replacement, successful Append or disposal; do not modify or free it.
*       The setter frees the previous source before duplication, so the
*       replacement must not alias that allocation. Copy failure loses the
*       previous text and leaves NULL; construction and set do not report this
*       as a separate error.
*       Each set rebuilds the list, including an unchanged value. Repeated
*       constructor Text tags overwrite earlier copies without freeing them. The
*       original tag is forwarded to List and normal notification processing; it
*       contains the supplied pointer, not the owned copy. Formatting
*       temporarily writes terminators into the owned source and restores them,
*       so do not read it concurrently during typesetting.
*
*/
/****** Floattext.mui/MUIA_Floattext_SkipChars ************
*
*   NAME
*       MUIA_Floattext_SkipChars
*
*   FUNCTION
*       Supply bytes to omit during typesetting.
*
*   INPUTS
*       Initialize and set, STRPTR. Default NULL. No local getter.
*
*   NOTES
*       The pointer is borrowed without copying; keep its NUL-terminated string
*       valid through all later retypesetting until replacement or disposal.
*       NULL disables this filter. A non-NULL pointer also selects code paths
*       that strip leading spaces, even when it points to an empty string.
*       Tabs and two-byte escape handling have their own formatting paths;
*       SkipChars is not a general escaping or sanitizing mechanism. Setting
*       this attribute rebuilds all lines. A paragraph filtered to no printable
*       text can prevent formatter progress; arbitrary filtering is not
*       protected against that case.
*
*/
/****** Floattext.mui/MUIA_Floattext_TabSize ************
*
*   NAME
*       MUIA_Floattext_TabSize
*
*   FUNCTION
*       Choose the number of spaces emitted for each tab.
*
*   INPUTS
*       Initialize and set, header LONG, stored ULONG. Default 8. Zero becomes
*       8; values greater than 20 become 20. No local getter.
*
*   NOTES
*       Every tab expands to this many spaces, independent of its column; these
*       are not tab stops. Conversion to ULONG occurs before the upper-limit
*       check, so negative input is not a negative tab width. Construction and
*       any recognized attribute set normalize the stored size before
*       rebuilding. Buffer lengths still use narrower counters and are not
*       protected against expansion overflow.
*
*/
/****** Floattext.mui/MUIA_Floattext_Justify ************
*
*   NAME
*       MUIA_Floattext_Justify
*
*   FUNCTION
*       Enable insertion of spaces to fill wrapped lines.
*
*   INPUTS
*       Initialize, set and get, BOOL. Default FALSE. The setter normalizes
*       nonzero values to TRUE.
*
*   NOTES
*       The formatter inserts whole space characters among words when eligible
*       spaces exist. It can also stretch the last paragraph line; that line is
*       excluded only when the computed multiplier exceeds five. There is no
*       fractional spacing or exact pixel-width guarantee. Get returns the
*       stored flag. Setting it always rebuilds the list; it does not merely
*       change drawing style. A font with zero-width spaces is not protected
*       against division by zero.
*
*/
/****** Floattext.mui/MUIM_Floattext_Append ************
*
*   NAME
*       MUIM_Floattext_Append
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Floattext_Append, (IPTR)text);
*
*   FUNCTION
*       Append a string directly to the stored source and rebuild all lines.
*
*   INPUTS
*       text - Readable NUL-terminated string, or NULL for no operation. No
*       newline or separator is inserted.
*
*   RESULT
*       Always zero, including successful append, NULL input and allocation
*       failure.
*
*   NOTES
*       A new combined allocation is filled before the old source is freed. The
*       input may therefore refer to the current source while copying. Combined
*       length plus terminator must fit the unchecked ULONG allocation
*       calculation. Allocation failure preserves the old source and list.
*       Successful replacement invalidates prior getter pointers and retypesets
*       the entire list; subsequent formatting failure is not reported.
*       The method writes private source state directly and does not send a
*       Floattext_Text setter tag or notification for that attribute. It does
*       not explicitly scroll to the appended text. An empty non-NULL string
*       still allocates and rebuilds.
*
*/

extern struct Library *MUIMasterBase;

// like strlen(), but \n ends string, too.
/*
 * Return byte count before newline or NUL in a readable string. Requires a
 * terminator and representable pointer-to-long subtraction. No mutation,
 * allocation or ownership transfer.
 */
static long MyStrLen(const char *ptr)
{
    const char *start = ptr;

    while (*ptr && (*ptr != '\n'))
        ptr++;

    return (((long)ptr) - ((long)start));
}

/*
 * Calculates the number of characters that will fit on a line of a given
 * pixel width.
 */
/*
 * Use window RastPort TextFit on the remaining paragraph and prefer a
 * preceding space boundary. Return fitted count as UWORD, possibly zero.
 * Requires valid text/length/offset, window and representable width. Does
 * not ensure caller progress or allocate storage.
 */
static UWORD FitParagraphLine(STRPTR text, ULONG length, WORD width,
   ULONG offset, struct Window *window)
{
    struct TextExtent temp_extent;
    ULONG char_count;
    UBYTE *p, *q = text + offset;

    char_count = TextFit(window->RPort, text + offset,
        length - offset, &temp_extent, NULL, 1, width, 32767);

    /* Find the last space in the fitted substring */

    if (offset + char_count != length)
    {
        for (p = text + offset + char_count; p > q && *p != ' '; p--);
        for (; p > q && *p == ' '; p--);
        if (p > q)
            char_count = p - q + 1;
    }

    return char_count;
}

/*
 * Clear List and rebuild copied lines from owned source using borrowed
 * SkipChars and live window geometry. Temporarily mutate/restore source
 * terminators. Own and release temporary buffers on normal exit; allocation
 * failures leave typesetting TRUE. No status; unchecked UWORD lengths and
 * zero-fit paragraphs can overrun or loop without progress. See class
 * limits.
 */
static void SetText(Object *obj, struct Floattext_DATA *data)
{
    WORD width;
    struct Window *window;
    UWORD i, count, pos, line_size = 0, space_count = 0, extra_space_count,
        space_width, space_multiple = 0, bonus_space_count = 0, line_len,
        bonus_space_mod = 1, space_no_in = 0, space_no_out, stripped_pos,
        stripped_count, stripped_size = 0, control_count, old_control_count,
        tab_count;
    UBYTE *text, *p, *q, *r, c, *line = NULL, *old_line,
        *stripped_text = NULL;
    LONG len, stripped_len;
    BOOL justify, found, is_empty;

    /* Avoid redrawing list while inserting lines */

    data->typesetting = TRUE;

    DoMethod(obj, MUIM_List_Clear);

    window = (struct Window *)XGET(obj, MUIA_Window);
    width = XGET(obj, MUIA_Width) - XGET(obj, MUIA_InnerLeft)
        - XGET(obj, MUIA_InnerRight) - 8;

    /* Only do layout if we have some text and a window in which to put it */

    if (data->text && window != NULL)
    {
        /* Get width of a space character */

        space_width = TextLength(window->RPort, " ", 1);

        /* Lay out each paragraph */

        for (text = data->text; text[0] != '\0'; text += pos)
        {
            stripped_pos = pos = 0;
            len = MyStrLen(text);

            if (len > 0)
            {
                /* Allocate paragraph buffer (the buffer size assumes the
                 * worst-case scenario: every character is a tab) */

                if (stripped_text == NULL
                    || len * data->tabsize >= stripped_size)
                {
                    FreeVec(stripped_text);
                    stripped_size = len * data->tabsize + 1;
                    stripped_text = AllocVec(stripped_size, MEMF_ANY);
                }
                if (stripped_text == NULL)
                {
                    FreeVec(line);
                    return;
                }

                /* Make a copy of paragraph text without control sequences
                 * or skip-chars, and with tabs expanded to spaces */

                for (p = text, q = stripped_text; *p != '\0' && *p != '\n'; )
                {
                    if (*p == '\33')
                        p += 2;
                    else if (*p == '\t')
                    {
                        for (i = 0; i < data->tabsize; i++)
                            *q++ = ' ';
                        p++;
                    }
                    else if (data->skipchars != NULL)
                    {
                        for (r = data->skipchars; *r != '\0' && *r != *p; r++);
                        if (*r == '\0')
                            *q++ = *p;
                        p++;
                    }
                    else
                        *q++ = *p++;
                }
                *q = '\0';
                stripped_len = MyStrLen(stripped_text);

                /* Reuse old line buffer, but don't carry over control codes
                 * into new paragraph */

                if (line != NULL)
                    line[0] = '\0';
                old_control_count = control_count = 0;

                /* Divide this paragraph into lines */

                while ((stripped_count = FitParagraphLine(stripped_text,
                    stripped_len, width, stripped_pos, window)) != 0)
                {
                    /* Count number of characters for this line in original
                     * text */

                    old_control_count += control_count;
                    control_count = tab_count = 0;
                    for (i = 0, p = text + pos; i < stripped_count
                        || (i == stripped_count && *p != ' ' && *p != '\t');
                        p++)
                    {
                        if (*p == '\33')
                        {
                            control_count += 2;
                            p++;
                        }
                        else if (*p == '\t')
                        {
                            control_count++;
                            tab_count++;
                            i += data->tabsize;
                        }
                        else if (data->skipchars != NULL)
                        {
                            for (r = data->skipchars;
                                *r != '\0' && *r != *p; r++);
                            if (*r != '\0' || (*p == ' ' && i == 0))
                                control_count++;
                            else
                                i++;
                        }
                        else
                            i++;
                    }
                    count = stripped_count + control_count
                        - tab_count * data->tabsize;

                    /* Default to justified text if it's enabled and this
                     * isn't the last line of the paragraph */

                    justify = data->justify;
                    if (justify)
                    {
                        /* Count number of spaces in stripped line */

                        p = stripped_text + stripped_pos;
                        for (i = 0, space_count = 0; i < stripped_count; i++)
                        {
                            if (p[i] == ' ')
                                space_count++;
                        }
                        space_count -= tab_count * data->tabsize;

                        if (space_count == 0)
                            justify = FALSE;
                    }

                    if (justify)
                    {
                        /* Find out how many extra spaces to insert for fully
                         * justified text */

                        extra_space_count = (width - TextLength(window->RPort,
                            stripped_text + stripped_pos, stripped_count))
                            / space_width;

                        space_multiple = (space_count + extra_space_count)
                            / space_count;
                        bonus_space_count = (space_count + extra_space_count)
                            - space_count * space_multiple;
                        if (bonus_space_count > 0)
                            bonus_space_mod =
                                (space_count + 3) / bonus_space_count;
                        else
                            bonus_space_mod = space_count;

                        /* Don't justify on last line if it will be too
                         * stretched */

                        if (space_multiple > 5
                            && pos + count == len)
                            justify = FALSE;
                    }
                    else
                        extra_space_count = 0;

                    /* Count number of characters in line that will be
                     * inserted into List object */

                    line_len = old_control_count + control_count
                        + stripped_count
                        + tab_count * (data->tabsize - 1) + extra_space_count;

                    /* Allocate line buffer (we allocate more space than
                     * necessary to reduce the need to reallocate if later
                     * lines are longer) */

                    old_line = line;
                    if (line == NULL || line_len >= line_size)
                    {
                        line_size = line_len * 2 + 1;
                        line = AllocVec(line_size, MEMF_ANY);
                    }
                    if (line == NULL)
                    {
                        FreeVec(old_line);
                        FreeVec(stripped_text);
                        return;
                    }

                    /* Prefix new line with all control characters contained
                     * in previous line */

                    q = line;
                    if (old_line != NULL)
                    {
                        for (p = old_line; *p != '\0'; p++)
                        {
                            if (*p == '\33')
                            {
                                *q++ = *p++;
                                *q++ = *p;
                            }
                        }
                    }
                    if (old_line != line)
                        FreeVec(old_line);

                    /* Generate line to insert in List object */

                    is_empty = TRUE;
                    p = text + pos;
                    c = p[count];
                    p[count] = '\0';
                    space_no_in = space_no_out = 0;
                    while (*p != '\0')
                    {
                        if (*p == ' ' && justify && !is_empty)
                        {
                            /* Add extra spaces to justify text */

                            for (i = 0; i < space_multiple; i++)
                                *q++ = ' ';
                            space_no_out += space_multiple;

                            if (bonus_space_count > 0
                                && (space_no_in % bonus_space_mod == 0))
                            {
                                *q++ = ' ';
                                bonus_space_count--;
                            }
                            space_no_out++;

                            p++;
                            space_no_in++;

                            /* Last input space? Make it as wide as
                             * necessary for line to reach right border */

                            if (space_no_in == space_count)
                            {
                                while (bonus_space_count-- > 0)
                                    *q++ = ' ';
                            }
                        }
                        else if (*p == '\t')
                        {
                            /* Expand tab to spaces */

                            for (i = 0; i < data->tabsize; i++)
                                *q++ = ' ';
                            p++;
                        }
                        else if (data->skipchars != NULL)
                        {
                            /* Filter out skip-chars */

                            for (r = data->skipchars;
                                *r != '\0' && *r != *p; r++);
                            if (*r == '\0' && !(*p == ' ' && is_empty))
                            {
                                *q++ = *p;
                                is_empty = FALSE;
                            }
                            p++;
                        }
                        else
                        {
                            /* No skip-chars, so direct copy */

                            *q++ = *p++;
                            is_empty = FALSE;
                        }
                    }
                    *q = '\0';

                    /* Add line to list */

                    DoMethod(obj, MUIM_List_InsertSingle, line,
                        MUIV_List_Insert_Bottom);
                    *p = c;

                    /* Move on to first word of next line */

                    stripped_pos += stripped_count;
                    pos += count;
                    if (pos != len)
                    {
                        if (data->skipchars != NULL)
                        {
                            for (found = FALSE, p = text + pos;
                                !found; pos++, p++)
                            {
                                for (r = data->skipchars;
                                    *r != '\0' && *r != *p; r++);
                                if (*r == '\0' && *p != ' ' && *p != '\t')
                                    found = TRUE;
                            }
                            pos--, p--;
                        }
                        else
                            for (p = text + pos; *p == ' ' || *p == '\t';
                                pos++, p++);
                        for (p = stripped_text + stripped_pos; *(p++) == ' ';
                            stripped_pos++);
                    }
                }
            }
            else
                DoMethod(obj, MUIM_List_InsertSingle, "",
                    MUIV_List_Insert_Bottom);

            if (text[pos] == '\n')
                pos++;
        }
        FreeVec(line);
        FreeVec(stripped_text);
    }

    data->typesetting = FALSE;

    DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_All);
}

/*
 * Construct List, install string hooks, parse attributes, normalize tab
 * size and typeset. Return object or zero on superclass failure. Own copied
 * Text, borrow SkipChars. Text-copy or line-generation failure does not
 * fail construction; repeated Text tags leak earlier copies.
 */
IPTR Floattext__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Floattext_DATA *data;
    struct TagItem *tag;
    struct TagItem *tags;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);

    if (!obj)
    {
        return 0;
    }

    data = INST_DATA(cl, obj);

    SetAttrs(obj, MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
        MUIA_List_DestructHook, MUIV_List_DestructHook_String, TAG_DONE);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Floattext_Justify:
            data->justify = tag->ti_Data;
            break;

        case MUIA_Floattext_SkipChars:
            data->skipchars = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Floattext_TabSize:
            data->tabsize = tag->ti_Data;
            break;

        case MUIA_Floattext_Text:
            data->text = StrDup((STRPTR) tag->ti_Data);
            break;
        }
    }

    if (data->tabsize == 0)
        data->tabsize = 8;
    else if (data->tabsize > 20)
        data->tabsize = 20;

    SetText(obj, data);

    return (IPTR) obj;
}

/*
 * Free owned source text then return List disposal result, which releases
 * generated entries through installed hooks. Requires no active formatting
 * or borrowed getter consumers. SkipChars remains caller-owned.
 */
IPTR Floattext__OM_DISPOSE(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct Floattext_DATA *data = INST_DATA(cl, obj);

    FreeVec(data->text);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Write stored Justify or borrowed source pointer into valid IPTR storage
 * and return one. Delegate other attributes to List. Source pointer expires
 * after successful append, replacement or disposal.
 */
IPTR Floattext__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Floattext_DATA *data = INST_DATA(cl, obj);
#define STORE *(msg->opg_Storage)

    switch (msg->opg_AttrID)
    {
    case MUIA_Floattext_Justify:
        STORE = data->justify;
        return 1;

    case MUIA_Floattext_Text:
        STORE = (IPTR) data->text;
        return 1;

    }

#undef STORE

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Store recognized attributes, freeing source before Text duplication;
 * normalize tabs and rebuild once if any recognized tag occurred. Return
 * List setter result with original tags forwarded. Replacement must not
 * alias owned text; failures are not separately reported and may lose old
 * state.
 */
IPTR Floattext__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Floattext_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tag;
    struct TagItem *tags;
    BOOL changed = FALSE;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Floattext_Justify:
            data->justify = tag->ti_Data != 0;
            changed = TRUE;
            break;

        case MUIA_Floattext_SkipChars:
            data->skipchars = (STRPTR) tag->ti_Data;
            changed = TRUE;
            break;

        case MUIA_Floattext_TabSize:
            data->tabsize = tag->ti_Data;
            changed = TRUE;
            break;

        case MUIA_Floattext_Text:
            FreeVec(data->text);
            data->text = StrDup((STRPTR) tag->ti_Data);
            changed = TRUE;
            break;

        }
    }

    if (changed)                // To avoid recursion
    {
        if (data->tabsize == 0)
            data->tabsize = 8;
        else if (data->tabsize > 20)
            data->tabsize = 20;

        SetText(obj, data);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
/*
 * Draw through List unless typesetting is active; on full draw and changed
 * outer width cache width and rebuild lines. Return zero. Requires live
 * render context; margin/font changes alone do not trigger this test and
 * formatting may fail or not terminate.
 */
IPTR Floattext__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Floattext_DATA *data = INST_DATA(cl, obj);

    if (!data->typesetting)
        DoSuperMethodA(cl, obj, (Msg) msg);

    if ((msg->flags & MADF_DRAWOBJECT) != 0 && data->oldwidth != _width(obj))
    {
        data->oldwidth = _width(obj);
        SetText(obj, data);
    }

    return 0;
}

/**************************************************************************
 MUIM_Floattext_Append
**************************************************************************/
/*
 * For non-NULL input allocate/copy concatenated source before freeing old
 * storage and retypeset. Return zero on all paths. Allocation failure
 * retains state; combined size is unchecked ULONG. No Text setter
 * notification or explicit scrolling.
 */
IPTR Floattext__MUIM_Floattext_Append(struct IClass *cl, Object *obj,
    struct MUIP_Floattext_Append *msg)
{
    struct Floattext_DATA *data = INST_DATA(cl, obj);

    if (msg->Text)
    {
        ULONG newlen = strlen(msg->Text) + 1;
        if (data->text)
        {
            newlen += strlen(data->text);
        }
        TEXT *newtext = AllocVec(newlen, MEMF_ANY);
        if (newtext)
        {
            newtext[0] = '\0';
            if (data->text)
            {
                strcpy(newtext, data->text);
            }
            strcat(newtext, msg->Text);
            FreeVec(data->text);
            data->text = newtext;
            SetText(obj, data);
        }
    }

    return 0;
}

#if ZUNE_BUILTIN_FLOATTEXT
/*
 * When built in, dispatch local overrides and return their results;
 * delegate other messages to List. Requires ABI-correct messages and live
 * object except construction. Ownership and lifecycle follow class
 * contract.
 */
BOOPSI_DISPATCHER(IPTR, Floattext_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Floattext__OM_NEW(cl, obj, msg);
    case OM_DISPOSE:
        return Floattext__OM_DISPOSE(cl, obj, msg);
    case OM_GET:
        return Floattext__OM_GET(cl, obj, msg);
    case OM_SET:
        return Floattext__OM_SET(cl, obj, msg);
    case MUIM_Draw:
        return Floattext__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    case MUIM_Floattext_Append:
        return Floattext__MUIM_Floattext_Append(cl, obj,
            (struct MUIP_Floattext_Append *)msg);

    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Floattext_desc =
{
    MUIC_Floattext,
    MUIC_List,
    sizeof(struct Floattext_DATA),
    (void *) Floattext_Dispatcher
};
#endif /* ZUNE_BUILTIN_FLOATTEXT */

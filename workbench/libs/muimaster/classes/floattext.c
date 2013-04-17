/*
    Copyright © 2002-2013, The AROS Development Team. All rights reserved.
    $Id$
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

extern struct Library *MUIMasterBase;

// like strlen(), but \n ends string, too.
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

static void SetText(Object *obj, struct Floattext_DATA *data)
{
    WORD width;
    struct Window *window;
    UWORD i, count, pos, line_size, space_count = 0, extra_space_count,
        space_width, space_multiple = 0, bonus_space_count, line_len,
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

IPTR Floattext__OM_DISPOSE(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct Floattext_DATA *data = INST_DATA(cl, obj);

    FreeVec(data->text);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

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

#if ZUNE_BUILTIN_FLOATTEXT
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

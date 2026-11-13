/*
    Copyright  2002-2026, The AROS Development Team.
    Copyright  1999, David Le Corfec.
    All rights reserved.

*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "textengine.h"

/****** Text.mui/MUIC_Text ************
*
*   NAME
*       MUIC_Text
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Display copied text using the Zune text engine. Contents and PreParse
*       default to empty strings. SetMin and SetVMax default to TRUE; SetMax
*       defaults to FALSE. No editing, cursor or automatic line wrapping is
*       implemented by this class. Newline bytes separate text-engine lines.
*       Version and Revision both return 1.
*       The object owns separate copies of Contents and PreParse and frees them
*       on disposal. Setup creates and measures a parsed text object; Cleanup
*       destroys it and clears its pointer. Complete Cleanup before disposal
*       because Dispose frees the strings but does not destroy the parsed text.
*       Getters return borrowed pointers into object-owned storage.
*
*   METHODS
*       Successful string setters duplicate the new string before freeing the
*       old one, rebuild parsed text while set up, and request full redraw. They
*       do not request parent relayout. Copy failure leaves the old string and
*       parsed text unchanged, but the original tag still reaches superclass
*       notification processing. The setter returns the superclass result and
*       does not report copy failure separately.
*       Setup returns FALSE on Area failure, otherwise TRUE even if text-engine
*       allocation fails. Bounds measurement tolerates NULL, but AskMinMax and
*       Draw dereference the parsed object without checking it. Constructor copy
*       failure may fall back to an empty string; construction fails only if
*       either final string remains NULL. Repeated constructor string tags can
*       leak earlier copies.
*       Draw calls Area, clips to the content rectangle and centers the parsed
*       block vertically. Horizontal alignment comes from the text engine. Pure
*       update requests return zero while the private update field is zero; this
*       class never sets it to a nonzero value in active code. Normal drawing
*       returns TRUE and resets that field. Show converts the superclass result
*       to BOOL; Hide returns it unchanged. Import and Export are local no-ops
*       returning zero without delegating to Area.
*
*   NOTES
*       The public text macros map to escape-byte commands: MUIX_L, MUIX_C and
*       MUIX_R select left, center and right alignment; MUIX_N resets style;
*       MUIX_B, MUIX_I and MUIX_U add bold, italic and underline; MUIX_PT and
*       MUIX_PH select draw-info pen indices 2 and 8. PreParse is concatenated
*       directly before Contents, with no inserted separator. Alignment and
*       style state can continue across lines. Parsing is byte-oriented, not
*       Unicode character indexing.
*       HiChar and HiCharIdx only control underlining. They do not set
*       Area_ControlChar or register keyboard activation. Supply valid
*       text-engine escapes and keep any external objects or image data
*       referenced by pointer escapes alive; copying the string does not copy
*       those objects. Text bounds use WORD fields, so large strings or
*       dimensions are not an unlimited-size layout contract.
*
*/
/****** Text.mui/MUIA_Text_Contents ************
*
*   NAME
*       MUIA_Text_Contents
*
*   FUNCTION
*       Set or retrieve the displayed text string.
*
*   INPUTS
*       Initialize, set and get, STRPTR. Default empty string. Supply readable
*       NUL-terminated text; setting NULL is equivalent to setting an empty
*       string.
*
*   NOTES
*       The object copies the string. Get returns the stored source text,
*       including escapes and marker characters, not a rendered-text buffer. Do
*       not free or modify the returned pointer; it expires on successful
*       replacement or disposal. Because replacement is copied first, the
*       current returned pointer can be supplied back to the setter.
*       Even identical text is copied and reparsed; no equality suppression
*       occurs. A successful change while set up remeasures the parsed text and
*       requests redraw without relayout. On copy failure the old value remains,
*       yet the original tag is forwarded and can trigger notifications. Do not
*       treat a notification as proof of successful replacement.
*       At construction, NULL Contents tags are ignored; a failed copy can be
*       replaced by the final empty-string fallback. Avoid repeated constructor
*       Contents tags because earlier copies are not freed. Newline bytes create
*       separate lines; text-engine escape sequences are interpreted.
*
*/
/****** Text.mui/MUIA_Text_PreParse ************
*
*   NAME
*       MUIA_Text_PreParse
*
*   FUNCTION
*       Set or retrieve text prepended to Contents before parsing.
*
*   INPUTS
*       Initialize, set and get, STRPTR. Default empty string. NULL becomes an
*       empty string on set; construction also falls back to an empty string.
*
*   NOTES
*       The string is copied and owned like Contents. Get returns a borrowed
*       pointer valid until successful replacement or disposal. It normally
*       supplies formatting escapes, but ordinary characters are rendered too.
*       No separator is inserted before Contents, and highlighting also scans
*       ordinary bytes in this prefix.
*       Each successful set destroys the old parsed text, copies the replacement
*       into stored state, reparses while set up and requests redraw without
*       relayout. Allocation failure retains the old value but does not suppress
*       the forwarded notification tag. Repeated constructor tags overwrite
*       earlier allocated pointers without freeing them. Supplying the current
*       getter pointer to the setter is safe because duplication precedes
*       release.
*
*/
/****** Text.mui/MUIA_Text_HiChar ************
*
*   NAME
*       MUIA_Text_HiChar
*
*   FUNCTION
*       Underline the first matching character during text parsing.
*
*   INPUTS
*       Initialize only, char stored as TEXT. Default zero disables this mode.
*
*   NOTES
*       The text engine compares ordinary bytes against the supplied character
*       converted with ToLower and ToUpper, underlining the first match and then
*       disabling further matches. It scans the concatenated PreParse and
*       Contents, across lines. No local setter or getter exists, and this
*       attribute does not assign a keyboard shortcut.
*       HiChar and HiCharIdx share one character field but use separate flags.
*       If both flags are enabled, HiChar mode wins while using the last stored
*       character value. Use one mode per object to avoid this interaction.
*
*/
/****** Text.mui/MUIA_Text_HiCharIdx ************
*
*   NAME
*       MUIA_Text_HiCharIdx
*
*   FUNCTION
*       Use a marker byte to underline the following byte.
*
*   INPUTS
*       Initialize only, char stored as TEXT. Default zero disables this mode.
*       The value is a marker character, not a numeric position.
*
*   NOTES
*       At the first matching ordinary marker byte, the parser removes the
*       marker from rendered text and underlines the next byte. Later markers
*       remain ordinary text because highlighting is then disabled. Get of
*       Contents still returns the original marker-containing string. There is
*       no local setter or getter and no keyboard binding.
*       The parser advances once more after consuming the underlined byte,
*       skipping inspection of the following byte. A marker at end of input or
*       immediately before the final character can therefore advance beyond the
*       terminator. Use a marker followed by the character to underline and at
*       least one further ordinary byte. Do not place escape or newline bytes at
*       that skipped position. This limitation is in the current text engine,
*       not a positional-index feature.
*       HiChar takes precedence when its flag is also enabled, but both
*       attributes share the same stored character field. Use only one
*       highlighting mode.
*
*/
/****** Text.mui/MUIA_Text_SetMin ************
*
*   NAME
*       MUIA_Text_SetMin
*
*   FUNCTION
*       Choose whether text width contributes to minimum width.
*
*   INPUTS
*       Initialize only, BOOL. Default TRUE. No local setter or getter.
*
*   NOTES
*       AskMinMax first adds measured text width to superclass minimum, default
*       and maximum widths. If SetMin is FALSE it then sets the entire minimum
*       width to zero, including the superclass contribution. Default width
*       still includes text width. Height constraints are unaffected. This
*       permits a narrower allocation; it does not enable line wrapping.
*
*/
/****** Text.mui/MUIA_Text_SetMax ************
*
*   NAME
*       MUIA_Text_SetMax
*
*   FUNCTION
*       Choose whether maximum width follows measured text width.
*
*   INPUTS
*       Initialize only, BOOL. Default FALSE. No local setter or getter.
*
*   NOTES
*       TRUE retains the superclass maximum width plus measured text width.
*       FALSE replaces maximum width with MUI_MAXMAX, allowing horizontal
*       expansion. Minimum and default widths still follow their separate rules.
*       This controls size negotiation, not string length or allocation size.
*
*/
/****** Text.mui/MUIA_Text_SetVMax ************
*
*   NAME
*       MUIA_Text_SetVMax
*
*   FUNCTION
*       Choose whether maximum height follows measured text height.
*
*   INPUTS
*       Initialize only, BOOL. Default TRUE. No local setter or getter.
*
*   NOTES
*       AskMinMax uses the greater of parsed text height and font YSize for
*       minimum and default height. TRUE adds that same height to the superclass
*       maximum. FALSE adds MUI_MAXMAX to the superclass maximum rather than
*       replacing it. Width constraints are unaffected. Drawing centers the
*       parsed text block vertically within the actual content rectangle.
*
*/

extern struct Library *MUIMasterBase;

//#define MYDEBUG 1
#include "debug.h"


struct MUI_TextData
{
    ULONG mtd_Flags;
    STRPTR contents;
    CONST_STRPTR preparse;
    TEXT hichar;
    ZText *ztext;

    LONG update;        /* type of update: 1 - everything,
                         * 2 - insert char, no scroll */
};

#define MTDF_SETMIN    (1<<0)
#define MTDF_SETMAX    (1<<1)
#define MTDF_SETVMAX   (1<<2)
#define MTDF_HICHAR    (1<<3)
#define MTDF_HICHARIDX (1<<4)


static const int __version = 1;
static const int __revision = 1;

/*
 * Create owned parsed text from stored strings, selecting HiChar before
 * HiCharIdx, then measure bounds. Requires live font/render context and no
 * unreleased previous parsed object. Stores NULL on allocation failure
 * without reporting status; later sizing/drawing assume non-NULL. Caller
 * owns lifecycle and must destroy parsed text.
 */
static void setup_text(struct MUI_TextData *data, Object *obj);

/**************************************************************************
OM_NEW
**************************************************************************/
/*
 * Construct Area and copy constructor strings/flags. Fall back to empty
 * strings for NULL final pointers; dispose partial object and return NULL
 * if either final copy is absent. Otherwise return object. Repeated string
 * tags leak old copies; no local notifications or parsing yet.
 */
IPTR Text__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_TextData *data;
    struct TagItem *tags, *tag;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);
    data->mtd_Flags = MTDF_SETMIN | MTDF_SETVMAX;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Text_Contents:
            if (tag->ti_Data)
                data->contents = StrDup((STRPTR) tag->ti_Data);
            break;

        case MUIA_Text_HiChar:
            data->hichar = tag->ti_Data;
            _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_HICHAR);
            break;

        case MUIA_Text_HiCharIdx:
            data->hichar = tag->ti_Data;
            _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_HICHARIDX);
            break;

        case MUIA_Text_PreParse:
            data->preparse = StrDup((STRPTR) tag->ti_Data);
            break;

        case MUIA_Text_SetMin:
            _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_SETMIN);
            break;

        case MUIA_Text_SetMax:
            _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_SETMAX);
            break;

        case MUIA_Text_SetVMax:
            _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_SETVMAX);
            break;

        }
    }

    if (!data->preparse)
        data->preparse = StrDup("");
    if (!data->contents)
        data->contents = StrDup("");

    if (!data->contents || !data->preparse)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return (IPTR) NULL;
    }

    return (IPTR) obj;
}

/**************************************************************************
OM_DISPOSE
**************************************************************************/
/*
 * Free owned Contents and PreParse strings and return Area disposal result.
 * Requires prior Cleanup for parsed text and no active consumers of getter
 * pointers. Does not destroy ztext itself.
 */
IPTR Text__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    FreeVec(data->contents);
    FreeVec((APTR) data->preparse);

    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
OM_SET
**************************************************************************/
/*
 * Copy each replacement before releasing prior string and parsed text; NULL
 * means empty. Reparse if set up and request full redraw, without parent
 * relayout. Failed duplication retains old state. Forward original tags and
 * return Area result, with no separate allocation status.
 */
IPTR Text__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Text_Contents:
            {
                char *new_contents =
                    StrDup(((char *)tag->ti_Data) ? (char *)tag->
                    ti_Data : "");
                if (new_contents)
                {
                    if (data->ztext)
                    {
                        zune_text_destroy(data->ztext);
                        data->ztext = NULL;
                    }
                    FreeVec(data->contents);
                    data->contents = new_contents;
                    if (_flags(obj) & MADF_SETUP)
                        setup_text(data, obj);
                    MUI_Redraw(obj, MADF_DRAWOBJECT); /* should be optimized */
                }
            }
            break;

        case MUIA_Text_PreParse:
            {
                char *new_preparse =
                    StrDup(((char *)tag->ti_Data) ? (char *)tag->
                    ti_Data : "");
                if (new_preparse)
                {
                    if (data->ztext)
                    {
                        zune_text_destroy(data->ztext);
                        data->ztext = NULL;
                    }
                    FreeVec((APTR) data->preparse);
                    data->preparse = new_preparse;
                    if (_flags(obj) & MADF_SETUP)
                        setup_text(data, obj);
                    MUI_Redraw(obj, MADF_DRAWOBJECT); /* should be optimized */
                }
            }
            break;

        case MUIA_Selected:
            D(bug("Text_Set(%p) : MUIA_Selected val=%ld sss=%d\n", obj,
                    tag->ti_Data, ! !(_flags(obj) & MADF_SHOWSELSTATE)));
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/**************************************************************************
OM_GET
**************************************************************************/
/*
 * Write borrowed Contents/PreParse pointer or class version/revision 1 to
 * valid caller-owned IPTR storage and return TRUE. Delegate other
 * attributes. Returned strings expire on replacement/disposal and transfer
 * no ownership.
 */
IPTR Text__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

#define STORE *(msg->opg_Storage)
    switch (msg->opg_AttrID)
    {
    case MUIA_Text_Contents:
        STORE = (IPTR) data->contents;
        return TRUE;

    case MUIA_Text_PreParse:
        STORE = (IPTR) data->preparse;
        return TRUE;

    case MUIA_Version:
        STORE = __version;
        return TRUE;

    case MUIA_Revision:
        STORE = __revision;
        return TRUE;
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/**************************************************************************
...
**************************************************************************/
/*
 * Create owned parsed text from stored strings, selecting HiChar before
 * HiCharIdx, then measure bounds. Requires live font/render context and no
 * unreleased previous parsed object. Stores NULL on allocation failure
 * without reporting status; later sizing/drawing assume non-NULL. Caller
 * owns lifecycle and must destroy parsed text.
 */
static void setup_text(struct MUI_TextData *data, Object *obj)
{
    if (data->mtd_Flags & MTDF_HICHAR)
    {
        data->ztext = zune_text_new(data->preparse, data->contents,
            ZTEXT_ARG_HICHAR, data->hichar);
    }
    else if (data->mtd_Flags & MTDF_HICHARIDX)
    {
        data->ztext = zune_text_new(data->preparse, data->contents,
            ZTEXT_ARG_HICHARIDX, data->hichar);
    }
    else
    {
        data->ztext = zune_text_new(data->preparse, data->contents,
            ZTEXT_ARG_NONE, 0);
    }
    zune_text_get_bounds(data->ztext, obj);

/*      D(bug("muimaster.library/text.c: ZText of 0x%p at 0x%p\n", */
/*          obj, data->ztext)); */
}

/**************************************************************************
MUIM_Setup
**************************************************************************/
/*
 * Call Area setup and return FALSE on failure; otherwise create/measure
 * parsed text and return TRUE even if parsing allocation failed. Requires
 * live font/context and paired Cleanup; no previous unreleased parsed
 * object.
 */
IPTR Text__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return FALSE;

    setup_text(data, obj);

    return TRUE;
}

/**************************************************************************
MUIM_Cleanup
**************************************************************************/
/*
 * Destroy non-NULL parsed text, clear pointer, then return Area Cleanup
 * result. Requires completed drawing and matched lifecycle. Stored strings
 * remain owned until replacement/disposal.
 */
IPTR Text__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    if (data->ztext)
    {
        zune_text_destroy(data->ztext);
        data->ztext = NULL;
    }

    return (DoSuperMethodA(cl, obj, (Msg) msg));
}

/**************************************************************************
MUIM_Show
**************************************************************************/
/*
 * Call Area Show and normalize its result to TRUE or FALSE. No local
 * resource acquisition or drawing; requires normal setup/show lifecycle.
 */
IPTR Text__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    //struct MUI_TextData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return FALSE;

    return TRUE;
}

/**************************************************************************
MUIM_Hide
**************************************************************************/
/*
 * Return Area Hide result unchanged. No local release or parsed-text
 * mutation; Cleanup remains responsible for parsed text.
 */
IPTR Text__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    //struct MUI_TextData *data = INST_DATA(cl, obj);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
MUIM_AskMinMax
**************************************************************************/
/*
 * Call Area, read measured text bounds and font height, then apply
 * SetMin/SetMax/SetVMax rules to writable size fields. Return TRUE ignoring
 * superclass result. Requires successful non-NULL parsed text; does not
 * parse or request relayout.
 */
IPTR Text__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    int height;
    struct MUI_TextData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    height = data->ztext->height;
    if (_font(obj)->tf_YSize > height)
        height = _font(obj)->tf_YSize;
/*      D(bug("YSize=%ld\n", _font(obj)->tf_YSize)); */


    msg->MinMaxInfo->MinWidth += data->ztext->width;
    msg->MinMaxInfo->DefWidth += data->ztext->width;
    msg->MinMaxInfo->MaxWidth += data->ztext->width;

    msg->MinMaxInfo->MinHeight += height;
    msg->MinMaxInfo->DefHeight += height;

    if (!(data->mtd_Flags & MTDF_SETVMAX))
        msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
    else
        msg->MinMaxInfo->MaxHeight += height;

    if (!(data->mtd_Flags & MTDF_SETMAX))
        msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

    if (!(data->mtd_Flags & MTDF_SETMIN))
        msg->MinMaxInfo->MinWidth = 0;

    D(bug
        ("Text_AskMinMax 0x%p (%s): Min=%ldx%ld Max=%ldx%ld Def=%ldx%ld\n",
            obj, data->contents, msg->MinMaxInfo->MinWidth,
            msg->MinMaxInfo->MinHeight, msg->MinMaxInfo->MaxWidth,
            msg->MinMaxInfo->MaxHeight, msg->MinMaxInfo->DefWidth,
            msg->MinMaxInfo->DefHeight));

    return TRUE;
}

/**************************************************************************
MUIM_Draw
**************************************************************************/
/*
 * Call Area; skip update requests when private update field is zero.
 * Otherwise clip and draw parsed block centered vertically, remove clip and
 * clear update. Return zero on skip, TRUE after draw. Requires valid parsed
 * text/window/render context; clipping status is not checked. Active code
 * does not set a nonzero update type.
 */
IPTR Text__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    Object *act = NULL;
    APTR clip;

/*    D(bug("muimaster.library/text.c: Draw Text Object at " */
/*        "0x%p %ldx%ldx%ldx%ld\n", obj, _left(obj), _top(obj), */
/*        _right(obj), _bottom(obj))); */

    DoSuperMethodA(cl, obj, (Msg) msg);

    if ((msg->flags & MADF_DRAWUPDATE) && !data->update)
        return 0;

    if (!(msg->flags & MADF_DRAWUPDATE))
        data->update = 0;

    if (msg->flags & MADF_DRAWUPDATE && data->update == 1)
    {
        DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj),
            _mwidth(obj), _mheight(obj), _mleft(obj), _mtop(obj), 0);
    }

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
        _mwidth(obj), _mheight(obj));

    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);

    {
        get(_win(obj), MUIA_Window_ActiveObject, &act);
        {
            int y = (_mheight(obj) - data->ztext->height) / 2;
            zune_text_draw(data->ztext, obj,
                _mleft(obj), _mright(obj), _mtop(obj) + y);
        }
    }

    MUI_RemoveClipping(muiRenderInfo(obj), clip);
    data->update = 0;
    return TRUE;
}

/**************************************************************************
MUIM_Export : to export an object's "contents" to a dataspace object.
**************************************************************************/
/*
 * Return zero without exporting data or delegating to Area. The historical
 * serialization body is excluded by #if 0. No allocation, ownership change
 * or dataspace access.
 */
IPTR Text__MUIM_Export(struct IClass *cl, Object *obj,
    struct MUIP_Export *msg)
{
    //struct MUI_TextData *data = INST_DATA(cl, obj);
    //STRPTR id;

#if 0
    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
        DoMethod(msg->dataspace, MUIM_Dataspace_AddString,
            _U(id), _U("contents"), _U(data->contents));
    }
#endif
    return 0;
}


/**************************************************************************
MUIM_Import : to import an object's "contents" from a dataspace object.
**************************************************************************/
/*
 * Return zero without importing data or delegating to Area. The historical
 * deserialization body is excluded by #if 0. No allocation, ownership
 * change or dataspace access.
 */
IPTR Text__MUIM_Import(struct IClass *cl, Object *obj,
    struct MUIP_Import *msg)
{
    //STRPTR id;
    //STRPTR s;

#if 0
    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
        if ((s = (STRPTR) DoMethod(msg->dataspace,
                    MUIM_Dataspace_FindString, _U(id), _U("contents"))))
        {
            set(obj, MUIA_Text_Contents, _U(s));
        }
    }
#endif
    return 0;
}


/*
 * Dispatch Text overrides and return selected handler results; delegate all
 * other messages to Area. Requires ABI-correct message and live object
 * except construction. Ownership and lifecycle follow class contract.
 */
BOOPSI_DISPATCHER(IPTR, Text_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Text__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Text__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Text__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Text__OM_GET(cl, obj, (struct opGet *)msg);

    case MUIM_AskMinMax:
        return Text__MUIM_AskMinMax(cl, obj, (APTR) msg);
    case MUIM_Draw:
        return Text__MUIM_Draw(cl, obj, (APTR) msg);
    case MUIM_Setup:
        return Text__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Cleanup:
        return Text__MUIM_Cleanup(cl, obj, (APTR) msg);
    case MUIM_Show:
        return Text__MUIM_Show(cl, obj, (APTR) msg);
    case MUIM_Hide:
        return Text__MUIM_Hide(cl, obj, (APTR) msg);
    case MUIM_Export:
        return Text__MUIM_Export(cl, obj, (APTR) msg);
    case MUIM_Import:
        return Text__MUIM_Import(cl, obj, (APTR) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
* Class descriptor.
*/
const struct __MUIBuiltinClass _MUI_Text_desc =
{
    MUIC_Text,
    MUIC_Area,
    sizeof(struct MUI_TextData),
    (void *) Text_Dispatcher
};

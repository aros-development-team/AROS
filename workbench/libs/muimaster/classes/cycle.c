/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

#include "debug.h"

#include "mui.h"
#include "imspec.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"
#include "textengine.h"

/****** Cycle.mui/MUIC_Cycle ************
*
*   NAME
*       MUIC_Cycle
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Select one entry using a cycling button and, when preferences permit, a
*       popup menu. The class creates an image and a PageGroup of Text children.
*       Children belong to the group; the entry array and its strings remain
*       borrowed because popup rendering reads them again. Omitted Entries
*       selects a static array containing one empty string. Initial Active
*       defaults to zero.
*       Use the normal Setup, Show, Hide and Cleanup lifecycle. Setup registers
*       a window event handler after successful superclass setup. Hide closes an
*       open popup; Cleanup removes the handler and delegates. There is no local
*       Dispose override and Cleanup does not independently close the popup.
*       Keep the object, parent window and entry storage alive during
*       synchronous event and notification processing.
*
*   METHODS
*       Releasing the button advances the selection and wraps to zero. The
*       cycle-menu minimum-entry preference controls whether a popup can open.
*       Keyboard Press opens it when eligible; otherwise, or if opening fails,
*       Press advances as Down does. Without a popup, Up and Down wrap;
*       Top/PageUp and Bottom/PageDown select the first and last entry. With a
*       popup, these keys change its provisional highlight; Press accepts a
*       highlight other than -1, and Window Close cancels.
*       A mouse press to the right of the cycle image opens an eligible popup
*       when drawing is permitted. Pointer motion changes its highlight.
*       SELECTUP or a subsequent SELECTDOWN accepts a highlight other than -1;
*       other closing mouse codes cancel. The popup is a nonactivating,
*       borderless Intuition window on the object's screen. Preferences control
*       its position and recessed entries; a menu larger than the screen is
*       refused, not scrolled.
*       With a nonzero ObjectID, Export writes the current selection as one LONG
*       to the supplied Dataspace. Import finds that ID and sets Active from its
*       first LONG. Both return zero and ignore storage/set failures. Import
*       does not validate record size; provide a matching LONG record and a live
*       Dataspace. With ObjectID zero, neither performs data access.
*
*   BUGS
*       Popup allocation failures are not fully rolled back. Failure to allocate
*       the last popup text can evade the count check because the index is
*       incremented before allocation. OpenWindow failure leaves popup
*       text/background resources allocated; Hide only calls popup cleanup when
*       a window exists, and a retry can overwrite retained resources. No
*       complete allocation-failure cleanup guarantee is provided.
*       The initial popup highlight is -1, not the current selection. Up tests
*       the highlight for nonzero and can decrement this sentinel to -2; this is
*       not a valid selection. The Active setter interprets -2 as Prev, so
*       accepting this invalid highlight can move the selection backward. Popup
*       geometry uses WORD fields without overflow checks; avoid entry counts
*       and text dimensions that overflow these calculations.
*
*/
/****** Cycle.mui/MUIA_Cycle_Entries ************
*
*   NAME
*       MUIA_Cycle_Entries
*
*   FUNCTION
*       Replace the available entries and rebuild the text pages.
*
*   INPUTS
*       Initialize and set; pointer to a NULL-terminated array of pointers to
*       terminated strings. No local getter. Omitted at construction: one empty
*       entry. NULL or an array whose first element is NULL creates no pages.
*
*   NOTES
*       The array and strings are borrowed and must remain readable and
*       unchanged until replacement or disposal. Text children copy their
*       labels, but popup text is later built from the original strings. Do not
*       free or edit the input after construction merely because the closed
*       gadget displays copied text.
*       Set closes the popup, disposes old pages, builds new pages within Group
*       InitChange/ExitChange, then resets Active to zero. This reset does not
*       generate a separate Cycle_Active tag notification. Multiple Entries and
*       Active tags take effect in tag order. Use a nonempty array when a
*       meaningful selected entry is required: with no pages, Get Active still
*       returns zero.
*       Page allocation failure stops construction at the successfully created
*       prefix; no aggregate failure is returned and old pages are not restored.
*       Member-add results are ignored. The build counter is UWORD, so the
*       terminating NULL must be reached before it wraps. Keep the much smaller
*       popup geometry limits in mind as well.
*
*/
/****** Cycle.mui/MUIA_Cycle_Active ************
*
*   NAME
*       MUIA_Cycle_Active
*
*   FUNCTION
*       Select an entry or return the stored zero-based index.
*
*   INPUTS
*       Initialize, set and get, LONG. Default zero. At construction, an index
*       outside the created page range becomes zero. For Set only,
*       MUIV_Cycle_Active_Next (-1) and MUIV_Cycle_Active_Prev (-2) advance or
*       reverse with wrapping.
*
*   NOTES
*       Set changes the page only for a valid index different from the stored
*       value. An unchanged or invalid request is rewritten to TAG_IGNORE in the
*       caller's tag list, suppressing that tag's notification. Supply writable
*       tags and rebuild ignored tags before reusing a list. The special values
*       are resolved only for selection: ti_Data is not replaced with the
*       resulting index. A notification using MUIV_TriggerValue therefore
*       receives the original Next/Prev token; read Active to obtain the
*       resulting index.
*       When the list contains only Cycle Entries/Active tags, the superclass
*       call prepends Group_Forward FALSE, preventing child forwarding without
*       suppressing notifications. Mixed tag lists use normal Group forwarding.
*       Get returns TRUE and stores the current index, including zero for an
*       empty entry list. Setter return values come from Group and do not
*       certify a selection change.
*
*/

extern struct Library *MUIMasterBase;

struct MUI_CycleData
{
    const char **entries;
    int entries_active;
    int entries_num;
    int entries_width;
    int entries_height;
    int cycle_width;
    int cycle_height;
    Object *pageobj;
    Object *imgobj;
    struct MUI_EventHandlerNode ehn;
    struct Hook pressedhook;
    struct MUI_ImageSpec_intern *popbg;
    struct Window *popwin;
    WORD popitemwidth;
    WORD popitemheight;
    WORD popitemoffx;
    WORD popitemoffy;
    WORD activepopitem;

};

#define POPITEM_EXTRAWIDTH 4
#define POPITEM_EXTRAHEIGHT 2

CONST_STRPTR default_entries[] =
{
    (CONST_STRPTR) "",
    (CONST_STRPTR) NULL
};

/*
 * Forward declaration. Close popup and release its transient resources,
 * remove/dispose old Text pages, then create copied labels from borrowed
 * entries. Requires live PageGroup and NULL-terminated readable array if
 * non-NULL. Stop on Text allocation failure, retain successful prefix,
 * ignore member-add results. UWORD counter must not wrap; no result or
 * rollback.
 */
static void UpdateEntries(Object *obj, struct MUI_CycleData *data);
/*
 * Forward declaration. Close existing popup, hide/clean background, and
 * destroy consecutive non-NULL child UserData text pointers, clearing each.
 * Requires owned popup text and a live parent Window when mouse-move events
 * are enabled. Stop at first missing text; remove/re-add handler without
 * mouse motion. No status; does not reset highlight.
 */
static void KillPopupWin(Object *obj, struct MUI_CycleData *data);

/*
 * Advance current selection and wrap to zero through Cycle_Active. Requires
 * live Cycle object and hook class pointer; msg is unused. Synchronous
 * setter notifications may run; no status or ownership transfer.
 */
void PressedHookFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct MUI_CycleData *data;
    Class *cl = (Class *) hook->h_Data;
    LONG act;

    data = INST_DATA(cl, obj);

    act = data->entries_active + 1;
    if (act >= data->entries_num)
        act = 0;

    set(obj, MUIA_Cycle_Active, act);
}

/**************************************************************************
OM_NEW
**************************************************************************/
/*
 * Create owned image/PageGroup children, retain borrowed entry array, build
 * pages and initialize selection. Return object or zero on superclass
 * failure; page-build failures can leave a successful partial object.
 * Install release hook; caller must keep source strings/array alive for
 * future popup use.
 */
IPTR Cycle__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_CycleData *data;
    struct TagItem *tag, *tags;
    Object *pageobj, *imgobj;

    obj = (Object *) DoSuperNewTags
        (cl, obj, NULL,
        ButtonFrame,
        MUIA_Background, MUII_ButtonBack,
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        MUIA_InnerTop, 1,
        MUIA_InnerBottom, 1,
        MUIA_Group_Horiz, TRUE,
        Child, (IPTR) (imgobj = ImageObject,
            MUIA_InnerLeft, 2,
            MUIA_Image_Spec, (IPTR) "6:17",
            MUIA_Image_FreeVert, TRUE,
            End),
        Child, (IPTR) (pageobj = PageGroup, End),
        TAG_MORE, (IPTR) msg->ops_AttrList);

    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->pageobj = pageobj;
    data->imgobj = imgobj;

    data->pressedhook.h_Entry = HookEntry;
    data->pressedhook.h_SubEntry = (HOOKFUNC) PressedHookFunc;
    data->pressedhook.h_Data = cl;

    data->entries = (const char **)default_entries;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Cycle_Entries:
            data->entries = (const char **)tag->ti_Data;
            break;

        case MUIA_Cycle_Active:
            data->entries_active = tag->ti_Data;
            break;
        }
    }

    UpdateEntries(obj, data);

    if ((data->entries_active >= 0)
        && (data->entries_active < data->entries_num))
    {
        set(pageobj, MUIA_Group_ActivePage, data->entries_active);
    }
    else
    {
        data->entries_active = 0;
    }

#if 1
    DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR) obj, 2, MUIM_CallHook, (IPTR) & data->pressedhook);
#else
    DoMethod(imgobj, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR) obj, 3, MUIM_Set, MUIA_Cycle_Active, MUIV_Cycle_Active_Next);
#endif

    return (IPTR) obj;
}

/**************************************************************************
OM_SET
**************************************************************************/
/*
 * Process Entries and Active in tag order, rebuilding pages or selecting
 * valid indices. Requires writable tags: invalid/unchanged Active tags
 * become TAG_IGNORE. Preserve original ti_Data for notifications, including
 * Next/Prev. Delegate with forwarding disabled only for a pure Cycle tag
 * list; return Group result, not change status.
 */
IPTR Cycle__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_CycleData *data;
    struct TagItem *tag, *tags;
    struct opSet supMsg;
    BOOL noforward = TRUE;
    LONG l;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Cycle_Entries:
            data->entries = (const char **)tag->ti_Data;
            DoMethod(data->pageobj, MUIM_Group_InitChange);
            UpdateEntries(obj, data);
            DoMethod(data->pageobj, MUIM_Group_ExitChange);

            data->entries_active = 0;
            set(data->pageobj, MUIA_Group_ActivePage,
                data->entries_active);
            break;

        case MUIA_Cycle_Active:
            l = (LONG) tag->ti_Data;

            if (l == MUIV_Cycle_Active_Next)
            {
                l = data->entries_active + 1;
                if (l >= data->entries_num)
                    l = 0;
            }
            else if (l == MUIV_Cycle_Active_Prev)
            {
                l = data->entries_active - 1;
                if (l < 0)
                    l = data->entries_num - 1;
            }

            if (l >= 0 && l < data->entries_num && l != data->entries_active)
            {
                data->entries_active = l;
                set(data->pageobj, MUIA_Group_ActivePage,
                    data->entries_active);
            }
            else
            {
                /* Attribute value not changed, don't fire notification */
                tag->ti_Tag = TAG_IGNORE;
            }
            break;

        default:
            noforward = FALSE;
            break;
        }
    }

    if (noforward)
    {
        struct TagItem tags[] = {
            {MUIA_Group_Forward,        FALSE                   },
            {TAG_MORE,                  (IPTR)msg->ops_AttrList }
        };
        supMsg.MethodID = msg->MethodID;
        supMsg.ops_AttrList = tags;
        supMsg.ops_GInfo = msg->ops_GInfo;

        return DoSuperMethodA(cl, obj, (Msg) & supMsg);
    }
    else
    {
        return DoSuperMethodA(cl, obj, (Msg) msg);
    }
}

/**************************************************************************
OM_GET
**************************************************************************/
/*
 * Store current Active through writable opg_Storage and return TRUE;
 * delegate other attributes to Group. There is no local Entries getter.
 * Reading Active does not prove a corresponding child exists when
 * construction produced no entries.
 */
IPTR Cycle__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);
#define STORE *(msg->opg_Storage)

    switch (msg->opg_AttrID)
    {
    case MUIA_Cycle_Active:
        STORE = data->entries_active;
        return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
MUIM_Setup
**************************************************************************/
/*
 * Delegate setup, then register priority-one GUI-mode mouse-button/raw-key
 * handler with parent Window. Requires connected object and valid render
 * context. Return zero on superclass failure, otherwise one; handler-add
 * result is not checked.
 */
IPTR Cycle__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_CycleData *data;

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return 0;

    data = INST_DATA(cl, obj);

    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 1;
    data->ehn.ehn_Flags = MUI_EHF_GUIMODE;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) & data->ehn);

    return 1;
}

/**************************************************************************
MUIM_Cleanup
**************************************************************************/
/*
 * Remove the registered event handler from the live parent Window and
 * return superclass cleanup result. Requires paired Setup and prior Hide
 * for an open popup; does not independently free popup resources or
 * retained entry storage.
 */
IPTR Cycle__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) & data->ehn);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Close popup and release its transient resources, remove/dispose old Text
 * pages, then create copied labels from borrowed entries. Requires live
 * PageGroup and NULL-terminated readable array if non-NULL. Stop on Text
 * allocation failure, retain successful prefix, ignore member-add results.
 * UWORD counter must not wrap; no result or rollback.
 */
static void UpdateEntries(Object *obj, struct MUI_CycleData *data)
{
    struct MinList *childlist = NULL;
    Object *page, *cstate;
    UWORD i;

    /* Ensure list isn't displayed while we update it */
    KillPopupWin(obj, data);

    /* Destroy old entries */
    get(data->pageobj, MUIA_Group_ChildList, &childlist);

    cstate = (Object *) childlist->mlh_Head;
    while ((page = NextObject(&cstate)))
    {
        DoMethod(data->pageobj, OM_REMMEMBER, (IPTR) page);
        DoMethod(page, OM_DISPOSE);
    }

    /* Count the number of entries */
    for (i = 0; data->entries && data->entries[i]; i++)
    {
        page = TextObject,
            MUIA_Text_Contents, (IPTR) data->entries[i],
            MUIA_Text_PreParse, (IPTR) "\033c", End;

        if (!page)
        {
            D(bug("Cycle_New: Could not create page object specified!\n"));
            break;
        }

        DoMethod(data->pageobj, OM_ADDMEMBER, (IPTR) page);
    }
    data->entries_num = i;
}

/*
 * Close existing popup, hide/clean background, and destroy consecutive non-
 * NULL child UserData text pointers, clearing each. Requires owned popup
 * text and a live parent Window when mouse-move events are enabled. Stop at
 * first missing text; remove/re-add handler without mouse motion. No
 * status; does not reset highlight.
 */
static void KillPopupWin(Object *obj, struct MUI_CycleData *data)
{
    struct MinList *childlist = NULL;
    Object *child, *cstate;

    if (data->popwin)
    {
        CloseWindow(data->popwin);
        data->popwin = NULL;
    }

    if (data->popbg)
    {
        zune_imspec_hide(data->popbg);
        zune_imspec_cleanup(data->popbg);
        data->popbg = NULL;
    }

    get(data->pageobj, MUIA_Group_ChildList, &childlist);

    cstate = (Object *) childlist->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        ZText *text = NULL;

        get(child, MUIA_UserData, &text);
        if (!text)
            break;

        zune_text_destroy(text);

        set(child, MUIA_UserData, 0);
    }

    if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
    {
        DoMethod(_win(obj), MUIM_Window_RemEventHandler,
            (IPTR) & data->ehn);
        data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
        DoMethod(_win(obj), MUIM_Window_AddEventHandler,
            (IPTR) & data->ehn);
    }

}

/*
 * Draw requested popup row using borrowed child UserData text and temporary
 * popup RastPort substitution. Requires live setup object and open popup;
 * missing child/text returns without drawing. Caller supplies valid row;
 * negative values are not explicitly rejected. Restore original RastPort
 * before return; no ownership transfer.
 */
static void RenderPopupItem(Object *obj, struct MUI_CycleData *data,
    WORD which)
{
    struct MinList *childlist = NULL;
    struct RastPort *saverp;
    Object *child, *cstate;
    ZText *text = NULL;
    WORD x1, y1, x2, y2, i = which;

    get(data->pageobj, MUIA_Group_ChildList, &childlist);
    cstate = (Object *) childlist->mlh_Head;

    while ((child = NextObject(&cstate)) && i--)
    {
    }

    if (!child)
        return;

    get(child, MUIA_UserData, &text);
    if (!text)
        return;                 /* paranoia */

    saverp = _rp(obj);
    _rp(obj) = data->popwin->RPort;

    x1 = data->popitemoffx;
    x2 = x1 + data->popitemwidth - 1;
    y1 = data->popitemoffy + which * data->popitemheight;
    y2 = y1 + data->popitemheight - 1;

    if (which == data->activepopitem)
    {
        WORD off = 0;

        if (muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_recessed_entries)
        {
            SetAPen(data->popwin->RPort, _pens(obj)[MPEN_SHADOW]);
            RectFill(data->popwin->RPort, x1, y1, x1, y2);
            RectFill(data->popwin->RPort, x1 + 1, y1, x2 - 1, y1);
            SetAPen(data->popwin->RPort, _pens(obj)[MPEN_SHINE]);
            RectFill(data->popwin->RPort, x2, y1, x2, y2);
            RectFill(data->popwin->RPort, x1 + 1, y2, x2 - 1, y2);

            off = 1;
        }

        SetAPen(data->popwin->RPort, _pens(obj)[MPEN_FILL]);
        RectFill(data->popwin->RPort, x1 + off, y1 + off, x2 - off,
            y2 - off);
    }
    else
    {
        if (data->popbg)
        {
            zune_imspec_draw(data->popbg, muiRenderInfo(obj),
                x1, y1, x2 - x1 + 1, y2 - y1 + 1, x1, y1, 0);
        }
        else
        {
            SetAPen(data->popwin->RPort, 0);
            RectFill(data->popwin->RPort, x1, y1, x2, y2);
        }
    }

    SetAPen(data->popwin->RPort, _pens(obj)[MPEN_TEXT]);

    y1 += POPITEM_EXTRAHEIGHT / 2;
    if (muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_recessed_entries)
    {
        y1++;
    }

    zune_text_draw(text, obj, x1, x2, y1);

    _rp(obj) = saverp;

}

/*
 * Build popup text/background and nonactivating screen window using live
 * setup/render context and borrowed entries. Return FALSE for excessive
 * screen size or detected allocation/open failure, TRUE after rendering.
 * Allocation rollback is incomplete, including last-text failure detection
 * and OpenWindow failure. Set highlight -1 on success; no selected-row
 * prehighlight.
 */
static BOOL MakePopupWin(Object *obj, struct MUI_CycleData *data)
{
    const struct ZuneFrameGfx *zframe;
    struct MinList *childlist = NULL;
    struct RastPort *rp, *saverp;
    Object *child, *cstate;
    WORD x, y, winx, winy, winw, winh, i;

    data->popitemwidth = 0;
    data->popitemheight = 0;

    get(data->pageobj, MUIA_Group_ChildList, &childlist);
    cstate = (Object *) childlist->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        set(child, MUIA_UserData, 0);
    }

    data->popitemwidth = _width(data->pageobj);
    data->popitemheight = _height(data->pageobj);

    data->popitemwidth += POPITEM_EXTRAWIDTH;
    data->popitemheight += POPITEM_EXTRAHEIGHT;

    if (muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_recessed_entries)
    {
        data->popitemwidth += 2;
        data->popitemheight += 2;
    }

    zframe =
        zune_zframe_get(obj,
        &muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_PopUp]);

    data->popitemoffx =
        muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_PopUp].innerLeft +
        zframe->ileft;

    data->popitemoffy =
        muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_PopUp].innerTop +
        zframe->itop;

    winw = data->popitemwidth + data->popitemoffx +
        muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_PopUp].innerRight +
        zframe->iright;

    winh = data->popitemheight * data->entries_num + data->popitemoffy +
        muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_PopUp].
        innerBottom + zframe->ibottom;

    if ((winw > _screen(obj)->Width) || (winh > _screen(obj)->Height))
    {
        return FALSE;
    }

    i = 0;
    cstate = (Object *) childlist->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        ZText *text;

        text = zune_text_new("\33c", data->entries[i++], ZTEXT_ARG_NONE, 0);
        if (!text)
            break;

        zune_text_get_bounds(text, obj);
        set(child, MUIA_UserData, (IPTR) text);
    }

    if (i != data->entries_num)
    {
        KillPopupWin(obj, data);
        return FALSE;
    }

    data->popbg = zune_imspec_setup(MUII_PopupBack, muiRenderInfo(obj));
    if (data->popbg)
        zune_imspec_show(data->popbg, obj);

    winx = _window(obj)->LeftEdge + _mleft(data->pageobj) -
        data->popitemoffx - POPITEM_EXTRAWIDTH / 2;

    if (muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_position ==
        CYCLE_MENU_POSITION_BELOW)
    {
        winy = _window(obj)->TopEdge + _bottom(obj) + 1;
    }
    else
    {
        winy =
            _window(obj)->TopEdge + _mtop(data->pageobj) -
            data->popitemoffy - POPITEM_EXTRAHEIGHT / 2 -
            data->entries_active * data->popitemheight;
    }

    data->popwin =
        OpenWindowTags(NULL, WA_CustomScreen, (IPTR) _screen(obj), WA_Left,
        winx, WA_Top, winy, WA_Width, winw, WA_Height, winh, WA_AutoAdjust,
        TRUE, WA_Borderless, TRUE, WA_Activate, FALSE, WA_BackFill,
        (IPTR) LAYERS_NOBACKFILL, TAG_DONE);

    if (!data->popwin)
    {
        return FALSE;
    }

    rp = data->popwin->RPort;

    saverp = _rp(obj);
    _rp(obj) = rp;
    zframe->draw(zframe->customframe, muiRenderInfo(obj), 0, 0, winw, winh,
        0, 0, winw, winh);

    if (data->popbg)
    {
        zune_imspec_draw(data->popbg, muiRenderInfo(obj),
            zframe->ileft, zframe->itop,
            winw - zframe->ileft - zframe->iright,
            winh - zframe->itop - zframe->ibottom,
            zframe->ileft, zframe->itop, 0);
    }
    else
    {
        SetAPen(rp, 0);
        RectFill(rp, zframe->ileft, zframe->itop,
            winw - 1 - zframe->iright, winh - 1 - zframe->ibottom);
    }

    x = data->popitemoffx;
    y = data->popitemoffy + POPITEM_EXTRAHEIGHT / 2;

    if (muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_recessed_entries)
    {
        y++;
    }

    i = 0;
    cstate = (Object *) childlist->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        ZText *text = NULL;

        get(child, MUIA_UserData, &text);

        SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
        if (text)               /* paranoia */
        {
            zune_text_draw(text, obj, x, x + data->popitemwidth - 1, y);
        }

        y += data->popitemheight;
    }

    _rp(obj) = saverp;

    data->activepopitem = -1;

    return TRUE;

}

/**************************************************************************
MUIM_HandleEvent
**************************************************************************/
/*
 * Handle MUI keys and eligible mouse events, updating popup highlight or
 * synchronously setting Active. Requires live object, valid message and
 * setup context; imsg may be NULL for keys. Return Eat for consumed events,
 * otherwise zero. Notifications may reenter; popup Up can decrement -1
 * sentinel to the Prev token.
 */
IPTR Cycle__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);
    BOOL fallthroughtomousemove = FALSE;

    if (msg->muikey != MUIKEY_NONE)
    {
        int old_active =
            data->popwin ? data->activepopitem : data->entries_active;
        int new_active = old_active;
        BOOL eat = FALSE;

        switch (msg->muikey)
        {
        case MUIKEY_WINDOW_CLOSE:
            if (data->popwin)
            {
                KillPopupWin(obj, data);
                eat = TRUE;
            }
            break;

        case MUIKEY_PRESS:
            if (data->entries_num <
                muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_min_entries)
            {
                /* fall through to MUIKEY_DOWN */
            }
            else if (!data->popwin)
            {
                if (MakePopupWin(obj, data))
                {
                    DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                        (IPTR) & data->ehn);
                    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
                    DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                        (IPTR) & data->ehn);
                    eat = TRUE;
                    break;
                }
                else
                {
                    /* fall through to MUIKEY_DOWN */
                }
            }
            else if (data->popwin)
            {
                KillPopupWin(obj, data);
                if (new_active != -1)
                {
                    set(obj, MUIA_Cycle_Active, new_active);
                }
                eat = TRUE;
                break;
            }
            /* no break here, because of fall-throughs above */

        case MUIKEY_DOWN:
            if (new_active < data->entries_num - 1)
            {
                new_active++;
            }
            else if (!data->popwin)
            {
                new_active = 0;
            }

            eat = TRUE;
            break;

        case MUIKEY_UP:
            if (new_active)
            {
                new_active--;
            }
            else if (!data->popwin)
            {
                new_active = data->entries_num - 1;
            }

            eat = TRUE;
            break;

        case MUIKEY_PAGEUP:
        case MUIKEY_TOP:
            new_active = 0;
            eat = TRUE;
            break;

        case MUIKEY_PAGEDOWN:
        case MUIKEY_BOTTOM:
            new_active = data->entries_num - 1;
            eat = TRUE;
            break;
        }

        if (new_active != old_active)
        {
            if (data->popwin)
            {
                data->activepopitem = new_active;

                if (old_active != -1)
                    RenderPopupItem(obj, data, old_active);
                if (new_active != -1)
                    RenderPopupItem(obj, data, new_active);

            }
            else
            {
                set(obj, MUIA_Cycle_Active, new_active);
            }

        }

        if (eat)
            return MUI_EventHandlerRC_Eat;

    }

    if (!msg->imsg ||
        data->entries_num <
        muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_min_entries)
    {
        return 0;
    }

    switch (msg->imsg->Class)
    {
    case IDCMP_MOUSEBUTTONS:
        switch (msg->imsg->Code)
        {
        case SELECTDOWN:
            if (_between(_right(data->imgobj) + 1, msg->imsg->MouseX,
                    _right(obj))
                && _between(_top(obj), msg->imsg->MouseY, _bottom(obj))
                && (muiAreaData(obj)->mad_Flags & MADF_CANDRAW)
                && !data->popwin)
            {
                if (MakePopupWin(obj, data))
                {
                    DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                        (IPTR) & data->ehn);
                    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
                    DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                        (IPTR) & data->ehn);

                    fallthroughtomousemove = TRUE;
                }
                break;
            }
            else if (data->popwin)
            {
                /* fall through to SELECTUP/MENUUP/MIDDLEUP */
            }
            else
            {
                break;
            }
            /* no break here! */

        case SELECTUP:
        case MENUUP:
        case MIDDLEUP:
        default:
            if (data->popwin)
            {
                KillPopupWin(obj, data);
                if ((data->activepopitem != -1) &&
                    ((msg->imsg->Code == SELECTUP)
                        || (msg->imsg->Code == SELECTDOWN)))
                {
                    set(obj, MUIA_Cycle_Active, data->activepopitem);
                }
                return MUI_EventHandlerRC_Eat;
            }
            break;


        }

        if (!fallthroughtomousemove)
        {
            break;
        }

    case IDCMP_MOUSEMOVE:
        if (data->popwin)
        {
            WORD x = data->popwin->MouseX;
            WORD y = data->popwin->MouseY - data->popitemoffy;
            WORD newactive = -1;

            if ((x >= 0) && (y >= 0) &&
                (x < data->popwin->Width)
                && (y < data->popitemheight * data->entries_num))
            {
                newactive = y / data->popitemheight;
            }

            if (newactive != data->activepopitem)
            {
                WORD oldactive = data->activepopitem;

                data->activepopitem = newactive;

                if (oldactive != -1)
                    RenderPopupItem(obj, data, oldactive);
                if (newactive != -1)
                    RenderPopupItem(obj, data, newactive);

            }

            return MUI_EventHandlerRC_Eat;
        }
        break;

    }

    return 0;
}

/**************************************************************************
MUIM_Hide
**************************************************************************/
/*
 * If a popup window exists, close it and free popup resources before
 * delegating Hide. Return superclass result. Failed popup creation with no
 * window is not cleaned by this guard; entry array remains borrowed.
 */
IPTR Cycle__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);

    if (data->popwin)
    {
        KillPopupWin(obj, data);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_Export - to export an object's "contents" to a dataspace object.
**************************************************************************/
/*
 * For nonzero ObjectID, synchronously copy one stack LONG selection into
 * live Dataspace via Add. Return zero regardless of Add result; do nothing
 * for ID zero. Caller owns Dataspace and must keep it alive through return.
 */
IPTR Cycle__MUIM_Export(struct IClass *cl, Object *obj,
    struct MUIP_Export *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);
    ULONG id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
        LONG value = data->entries_active;
        DoMethod(msg->dataspace, MUIM_Dataspace_Add,
            (IPTR) & value, sizeof(value), (IPTR) id);
    }
    return 0;
}

/**************************************************************************
 MUIM_Import - to import an object's "contents" from a dataspace object.
**************************************************************************/
/*
 * For nonzero ObjectID, find a borrowed record in live Dataspace and set
 * Active from its first LONG when present. Requires at least sizeof(LONG)
 * readable bytes; no length check. Return zero even for no record or
 * rejected index; setter can notify synchronously.
 */
IPTR Cycle__MUIM_Import(struct IClass *cl, Object *obj,
    struct MUIP_Import *msg)
{
    ULONG id;
    LONG *s;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
        if ((s = (LONG *) DoMethod(msg->dataspace, MUIM_Dataspace_Find,
            (IPTR) id)))
        {
            set(obj, MUIA_Cycle_Active, *s);
        }
    }
    return 0;
}

/*
 * Dispatch local class overrides and return handler results; delegate other
 * messages to Group. Requires ABI-correct message and live object except
 * construction. Child ownership, event callbacks and any popup resources
 * follow the class lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Cycle_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Cycle__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_SET:
        return Cycle__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Cycle__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Cycle__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Cleanup:
        return Cycle__MUIM_Cleanup(cl, obj, (APTR) msg);
    case MUIM_Hide:
        return Cycle__MUIM_Hide(cl, obj, (APTR) msg);
    case MUIM_HandleEvent:
        return Cycle__MUIM_HandleEvent(cl, obj, (APTR) msg);
    case MUIM_Export:
        return Cycle__MUIM_Export(cl, obj, (APTR) msg);
    case MUIM_Import:
        return Cycle__MUIM_Import(cl, obj, (APTR) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
* Class descriptor.
*/
const struct __MUIBuiltinClass _MUI_Cycle_desc =
{
    MUIC_Cycle,
    MUIC_Group,
    sizeof(struct MUI_CycleData),
    (void *) Cycle_Dispatcher
};

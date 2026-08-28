/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    protocols.c - shared ProtocolAddress infrastructure:
      - MUI list hooks (construct / destruct / display)
      - ProtoAddr_FromInterface / ProtoAddr_ToInterface conversions
      - PAWinClass: common protocol-address configuration window base class
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
#include <intuition/classes.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/alib.h>
#include <utility/hooks.h>
#include <string.h>

#include "protocols.h"
#include "netprefs_intern.h"
#include "locale.h"

#define USE_NET_PROTOICON_COLORS
#define USE_NET_PROTOICON_BODY
#include "net_protoicon.h"

/*--- MUI list hook implementations -----------------------------------------*/

AROS_UFH3S(APTR, protoConstructFunc,
    AROS_UFHA(struct Hook *,          hook,  A0),
    AROS_UFHA(APTR,                   pool,  A2),
    AROS_UFHA(struct ProtocolAddress *, entry, A1))
{
    AROS_USERFUNC_INIT

    struct ProtocolAddress *new;
    if ((new = AllocPooled(pool, sizeof(*new))))
        *new = *entry;
    return new;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, protoDestructFunc,
    AROS_UFHA(struct Hook *,          hook,  A0),
    AROS_UFHA(APTR,                   pool,  A2),
    AROS_UFHA(struct ProtocolAddress *, entry, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, entry, sizeof(struct ProtocolAddress));

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(LONG, protoDisplayFunc,
    AROS_UFHA(struct Hook *,          hook,  A0),
    AROS_UFHA(char **,                array, A2),
    AROS_UFHA(struct ProtocolAddress *, entry, A1))
{
    AROS_USERFUNC_INIT

    if (entry)
    {
        /*
         * Two static buffers are safe here because the display hook is called
         * once per entry before MUI renders the row; no re-entrancy concern.
         * The protocol name and the address text both come from the owning
         * plugin - this hook knows nothing about IPv4/IPv6.
         */
        static char famBuf[16];
        static char addrBuf[IP6BUFLEN + 8];
        struct ProtoHandlerNode *ph = ProtoHandler_ByID(entry->pa_node.ln_Type);

        famBuf[0] = addrBuf[0] = '\0';
        if (ph)
        {
            if (ph->ph_Node.ln_Name)
                strlcpy(famBuf, ph->ph_Node.ln_Name, sizeof(famBuf));
            if (ph->ph_Display)
                ph->ph_Display(entry, addrBuf, sizeof(addrBuf));
        }

        *array++ = famBuf;
        *array   = addrBuf;
    }
    else
    {
        /* Column header row */
        *array++ = (STRPTR)"Protocol";
        *array   = (STRPTR)_(MSG_IP);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

struct Hook proto_constructHook = { {0}, (HOOKFUNC)protoConstructFunc, NULL, NULL };
struct Hook proto_destructHook  = { {0}, (HOOKFUNC)protoDestructFunc,  NULL, NULL };
struct Hook proto_displayHook   = { {0}, (HOOKFUNC)protoDisplayFunc,   NULL, NULL };

/*--- Protocol-object list helpers ------------------------------------------*
 * The core keeps each interface's protocol objects on a List of
 * ProtocolAddress nodes, tagged in ln_Type with the owning plugin's id.  These
 * helpers let both the core and the plugins find/create/copy/free them without
 * anyone interpreting the address itself.
 */

extern struct NetPrefsBase *NetPrefs_GetBase(void);

/* Find the registered handler for a plugin id (ln_Type), or NULL. */
struct ProtoHandlerNode *ProtoHandler_ByID(UBYTE id)
{
    struct NetPrefsBase *npb = NetPrefs_GetBase();
    struct ProtoHandlerNode *ph;

    if (!npb)
        return NULL;
    ForeachNode(&npb->npb_ProtoHandlers, ph)
        if (ph->ph_ID == id)
            return ph;
    return NULL;
}

/* Find an interface's protocol object of the given id, or NULL. */
struct ProtocolAddress *ProtoAddr_Find(struct List *list, UBYTE id)
{
    struct ProtocolAddress *pa;

    ForeachNode(list, pa)
        if (pa->pa_node.ln_Type == id)
            return pa;
    return NULL;
}

/* Find, or allocate and append, an interface's protocol object of an id. */
struct ProtocolAddress *ProtoAddr_FindOrAdd(struct List *list, UBYTE id)
{
    struct ProtocolAddress *pa = ProtoAddr_Find(list, id);

    if (!pa && (pa = AllocVec(sizeof(*pa), MEMF_CLEAR)) != NULL)
    {
        pa->pa_node.ln_Type = id;
        AddTail(list, &pa->pa_node);
    }
    return pa;
}

/* Free every protocol object on a list, leaving it empty. */
void ProtoAddr_FreeList(struct List *list)
{
    struct Node *n;

    while ((n = RemHead(list)) != NULL)
        FreeVec(n);
}

/* Replace dst's protocol objects with deep copies of src's. */
void ProtoAddr_CopyList(struct List *dst, struct List *src)
{
    struct ProtocolAddress *pa;

    ProtoAddr_FreeList(dst);
    ForeachNode(src, pa)
    {
        struct ProtocolAddress *copy = AllocVec(sizeof(*copy), MEMF_ANY);
        if (copy)
        {
            *copy = *pa;                 /* copies ln_Type + all fields */
            AddTail(dst, &copy->pa_node);
        }
    }
}

/*===========================================================================
 * PAWinClass - common protocol-address configuration window base class.
 * Subclassed by Net4WinClass (net4.c) and Net6WinClass (net6.c).
 *
 * Tags accepted at OM_NEW:
 *   MUIA_PAWin_ProtocolName  (STRPTR)  - shown as "Address Protocol: <name>"
 *   MUIA_PAWin_Content       (Object*) - gadget group from subclass
 *   Any MUIA_Window_* tag    - passed through to Window.mui via TAG_MORE
 *
 * Read-only attributes:
 *   MUIA_PAWin_UseButton     (Object*) - the Use ImageButton
 *   MUIA_PAWin_CancelButton  (Object*) - the Cancel ImageButton
 *
 * Methods overridden by subclasses:
 *   MUIM_PAWin_Show(pa)        - populate gadgets from ProtocolAddress
 *   MUIM_PAWin_Apply(pa)       - read gadgets back into ProtocolAddress
 *   MUIM_PAWin_ModeChanged(m)  - handle mode-cycle notification
 *===========================================================================*/

struct MUI_CustomClass *PAWinClass = NULL;

struct PAWin_Data
{
    Object *pwd_UseButton;
    Object *pwd_CancelButton;
};

static IPTR PAWin__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    STRPTR   protoName = (STRPTR)GetTagData(MUIA_PAWin_ProtocolName,
                                            (IPTR)"", msg->ops_AttrList);
    Object  *content   = (Object *)GetTagData(MUIA_PAWin_Content,
                                              (IPTR)NULL, msg->ops_AttrList);
    Object  *useBtn, *cancelBtn;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_Window_Title,       (IPTR)_(MSG_CFG_ADDR),
        MUIA_Window_CloseGadget, FALSE,
        WindowContents, (IPTR)VGroup,
            GroupFrame,
            Child, (IPTR)HGroup,
                Child, (IPTR)HVSpace,
                Child, (IPTR)BodychunkObject,
                    MUIA_Bitmap_SourceColors,   (IPTR)net_protoicon_colors,
                    MUIA_FixWidth,              NET_PROTOICON_WIDTH,
                    MUIA_FixHeight,             NET_PROTOICON_HEIGHT,
                    MUIA_Bitmap_Width,          NET_PROTOICON_WIDTH,
                    MUIA_Bitmap_Height,         NET_PROTOICON_HEIGHT,
                    MUIA_Bodychunk_Depth,       NET_PROTOICON_DEPTH,
                    MUIA_Bodychunk_Body,        (IPTR)net_protoicon_body,
                    MUIA_Bodychunk_Compression, NET_PROTOICON_COMPRESSION,
                    MUIA_Bodychunk_Masking,     NET_PROTOICON_MASKING,
                    MUIA_Bitmap_Transparent,    NET_PROTOICON_TRANSPARENT,
                End,
                Child, (IPTR)HVSpace,
            End,
            Child, (IPTR)ColGroup(2),
                Child, (IPTR)Label2(__(MSG_PROTO_LABEL)),
                Child, (IPTR)TextObject,
                    MUIA_Text_Contents, (IPTR)protoName,
                End,
            End,
            Child, (IPTR)(content != NULL ? content : HVSpace),
            Child, (IPTR)HGroup,
                Child, (IPTR)(useBtn    = ImageButton(_(MSG_BUTTON_USE),
                                "THEME:Images/Gadgets/Use")),
                Child, (IPTR)(cancelBtn = ImageButton(_(MSG_BUTTON_CANCEL),
                                "THEME:Images/Gadgets/Cancel")),
            End,
        End,
        TAG_MORE, (IPTR)msg->ops_AttrList);

    if (!obj)
        return 0;

    struct PAWin_Data *data = INST_DATA(cl, obj);
    data->pwd_UseButton    = useBtn;
    data->pwd_CancelButton = cancelBtn;

    /* Cancel button closes this window */
    DoMethod(cancelBtn, MUIM_Notify, MUIA_Pressed, FALSE,
             obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    return (IPTR)obj;
}

static IPTR PAWin__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct PAWin_Data *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        case MUIA_PAWin_UseButton:
            *msg->opg_Storage = (IPTR)data->pwd_UseButton;
            return TRUE;
        case MUIA_PAWin_CancelButton:
            *msg->opg_Storage = (IPTR)data->pwd_CancelButton;
            return TRUE;
    }
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/* Base implementations — subclasses override these */
static IPTR PAWin__MUIM_PAWin_Show(Class *cl, Object *obj,
    struct MUIP_PAWin_Show *msg)
{
    (void)msg;
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

static IPTR PAWin__MUIM_PAWin_Apply(Class *cl, Object *obj,
    struct MUIP_PAWin_Apply *msg)
{
    (void)msg;
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

static IPTR PAWin__MUIM_PAWin_ModeChanged(Class *cl, Object *obj,
    struct MUIP_PAWin_ModeChanged *msg)
{
    (void)msg;
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

BOOPSI_DISPATCHER(IPTR, PAWin_Dispatch, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:
            return PAWin__OM_NEW(cl, obj, (struct opSet *)msg);
        case OM_GET:
            return PAWin__OM_GET(cl, obj, (struct opGet *)msg);
        case MUIM_PAWin_Show:
            return PAWin__MUIM_PAWin_Show(cl, obj,
                       (struct MUIP_PAWin_Show *)msg);
        case MUIM_PAWin_Apply:
            return PAWin__MUIM_PAWin_Apply(cl, obj,
                       (struct MUIP_PAWin_Apply *)msg);
        case MUIM_PAWin_ModeChanged:
            return PAWin__MUIM_PAWin_ModeChanged(cl, obj,
                       (struct MUIP_PAWin_ModeChanged *)msg);
        default:
            return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

BOOL PAWin_InitClass(void)
{
    if (PAWinClass)
        return TRUE;
    PAWinClass = MUI_CreateCustomClass(NULL, MUIC_Window, NULL,
                     sizeof(struct PAWin_Data), PAWin_Dispatch);
    return PAWinClass != NULL;
}

void PAWin_FreeClass(void)
{
    if (PAWinClass)
    {
        MUI_DeleteCustomClass(PAWinClass);
        PAWinClass = NULL;
    }
}

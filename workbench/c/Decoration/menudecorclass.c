/*
    Copyright  2011-2012, The AROS Development Team.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <clib/alib_protos.h>

#include <intuition/intuition.h>
#include <intuition/extensions.h>
#include <intuition/imageclass.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/exec.h>

#include "menudecorclass.h"
#include "screendecorclass.h"
#include "drawfuncs.h"
#include "config.h"


#define SETIMAGE_MEN(id) md->img_##id=sd->di->img_##id

struct menudecor_data
{
    struct DecorConfig * dc;

    /* Pointers to images used for sys images */
    struct NewImage *img_amigakey;
    struct NewImage *img_menucheck;
    struct NewImage *img_submenu;
};

static IPTR menudecor_getdefsizes(Class *cl, Object *obj, struct mdpGetDefSizeSysImage *msg)
{
    struct menudecor_data *data = INST_DATA(cl, obj);

    struct NewImage *n = NULL;
    BOOL  isset = FALSE;

    switch(msg->mdp_Which)
    {
        case AMIGAKEY:
            n = data->img_amigakey;
            if(n) isset = TRUE;
            break;

        case MENUCHECK:
            n = data->img_menucheck;
            if(n) isset = TRUE;
            break;

        case SUBMENUIMAGE:
            n = data->img_submenu;
            if(n) isset = TRUE;
            break;

        default:
            return FALSE;
    }

    if (!isset) return DoSuperMethodA(cl, obj, (Msg) msg);

    *msg->mdp_Width = n->w;
    *msg->mdp_Height = n->h;
    return TRUE;
}

static IPTR menudecor_getmenuspaces(Class *cl, Object *obj, struct mdpGetMenuSpaces *msg)
{
    struct menudecor_data *data = INST_DATA(cl, obj);

    msg->mdp_InnerLeft =  data->dc->MenuInnerLeft;
    msg->mdp_InnerTop =  data->dc->MenuInnerTop;
    msg->mdp_InnerRight =  data->dc->MenuInnerRight;
    msg->mdp_InnerBottom =  data->dc->MenuInnerBottom;
    msg->mdp_ItemInnerLeft = 1;
    msg->mdp_ItemInnerTop = 2;
    msg->mdp_ItemInnerRight = 2;
    msg->mdp_ItemInnerBottom = 1;
    if ((data->dc->MenuTileLeft + data->dc->MenuTileRight) > (data->dc->MenuInnerLeft + data->dc->MenuInnerRight)) 
        msg->mdp_MinWidth = data->dc->MenuTileLeft + data->dc->MenuTileRight; 
    else 
        msg->mdp_MinWidth = data->dc->MenuInnerLeft + data->dc->MenuInnerRight;

    if ((data->dc->MenuTileTop + data->dc->MenuTileBottom) > (data->dc->MenuInnerTop + data->dc->MenuInnerBottom)) 
        msg->mdp_MinHeight = data->dc->MenuTileTop + data->dc->MenuTileBottom; 
    else 
        msg->mdp_MinHeight = data->dc->MenuInnerTop + data->dc->MenuInnerBottom;

    return TRUE;
}

static IPTR menudecor_draw_sysimage(Class *cl, Object *obj, struct mdpDrawSysImage *msg)
{
    struct ScreenData      *md = (struct ScreenData *) msg->mdp_UserBuffer;
    struct RastPort        *rp = msg->mdp_RPort;
    struct NewImage        *ni = NULL;
    LONG                    left = msg->mdp_X;
    LONG                    top = msg->mdp_Y;
    WORD                    addx = 0;
    WORD                    addy = 0;
    BOOL                    isset = FALSE;

    switch(msg->mdp_Which)
    {
        case AMIGAKEY:
            if (md && md->img_amigakey->ok)
            {
                ni = md->img_amigakey;
                isset = TRUE;
            }
            break;

        case MENUCHECK:
            if (md && md->img_menucheck->ok)
            {
                ni = md->img_menucheck;
                isset = TRUE;
            }
            break;

        case SUBMENUIMAGE:
            if (md && md->img_submenu->ok)
            {
                ni = md->img_submenu;
                isset = TRUE;
            }
            break;

        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    if (!isset || (ni == NULL)) return DoSuperMethodA(cl, obj, (Msg)msg);

    DrawStatefulGadgetImageToRP(rp, ni, IDS_NORMAL, left + addx, top + addy);

    return TRUE;
}

/**************************************************************************************************/

static IPTR menudecor_renderbackground(Class *cl, Object *obj, struct mdpDrawBackground *msg)
{
    struct menudecor_data *data = INST_DATA(cl, obj);
    struct RastPort    *rp = msg->mdp_RPort;
    struct NewImage    *ni;
    struct MenuData    *md = (struct MenuData *) msg->mdp_UserBuffer;
    UWORD               flags = msg->mdp_Flags;

    if ((flags & HIGHITEM) && md->ni)
    {
        ni = NewImageContainer(msg->mdp_ItemWidth, msg->mdp_ItemHeight);
        if (ni)
        {
            DrawPartToImage(md->ni, ni, msg->mdp_ItemLeft, msg->mdp_ItemTop, msg->mdp_ItemWidth, msg->mdp_ItemHeight, 0, 0);
            SetImageTint(ni, 255 - (data->dc->MenuHighlightTint >> 24), data->dc->MenuHighlightTint & 0xffffff);
            PutImageToRP(rp, ni, msg->mdp_ItemLeft, msg->mdp_ItemTop);
        }
    }
    else
    {
        if (md->ni) DrawPartImageToRP(rp, md->ni, msg->mdp_ItemLeft, msg->mdp_ItemTop, msg->mdp_ItemLeft, msg->mdp_ItemTop, msg->mdp_ItemWidth, msg->mdp_ItemHeight);
    }

    return TRUE;
}

static IPTR menudecor_initmenu(Class *cl, Object *obj, struct mdpInitMenu *msg)
{
    struct menudecor_data  *data = INST_DATA(cl, obj);
    struct RastPort        *rp = msg->mdp_RPort;
    struct MenuData        *md = (struct MenuData *) msg->mdp_UserBuffer;
    struct ScreenData      *sd = (struct ScreenData *) msg->mdp_ScreenUserBuffer;

    SETIMAGE_MEN(menu);
    SETIMAGE_MEN(amigakey);
    SETIMAGE_MEN(menucheck);
    SETIMAGE_MEN(submenu);

    if (data->dc->MenuIsTiled)
    {
        md->img_menu_ti = AllocVec(sizeof(struct TileInfo), MEMF_ANY | MEMF_CLEAR);
        md->img_menu_ti->TileLeft   = data->dc->MenuTileLeft;
        md->img_menu_ti->TileRight  = data->dc->MenuTileRight;
        md->img_menu_ti->TileBottom = data->dc->MenuTileBottom;
        md->img_menu_ti->TileTop    = data->dc->MenuTileTop;
    }

    if ((msg->mdp_MenuDecorFlags & MDP_MDF_MENU) && !(msg->mdp_MenuDecorFlags & MDP_MDF_MENUS_UNDERMOUSE))
    {
        /* Special handling for pulled down menu bar */
        LONG height = msg->mdp_Height;

        /* Increase height for rendering if needed */
        if ((data->dc->MenuIsTiled) && (height < (md->img_menu_ti->TileBottom + md->img_menu_ti->TileTop)))
            height = (md->img_menu_ti->TileBottom + md->img_menu_ti->TileTop);

        md->ni = NewImageContainer(msg->mdp_Width, height);
        if (md->ni)
        {
            md->ni->ok = TRUE;
            RenderMenuBarBackground(md->ni, md->img_menu, md->img_menu_ti, 20);

            /* Scale down if needed */
            if (height > msg->mdp_Height)
            {
                struct NewImage * sni = ScaleNewImage(md->ni, msg->mdp_Width, msg->mdp_Height);
                if (sni)
                {
                    DisposeImageContainer(md->ni);
                    md->ni = sni;
                    md->ni->ok = TRUE;
                }
            }
        }
    }
    else
    {
        md->ni = GetImageFromRP(rp, msg->mdp_Left, msg->mdp_Top, msg->mdp_Width, msg->mdp_Height);
        if (md->ni)
        {
            md->ni->ok = TRUE;
            RenderMenuBackground(md->ni, md->img_menu, md->img_menu_ti, 20);
        }
    }


    return TRUE;
}

static IPTR menudecor_exitmenu(Class *cl, Object *obj, struct mdpExitMenu *msg)
{
    struct MenuData     *md = (struct MenuData *) msg->mdp_UserBuffer;

    if (md->ni) DisposeImageContainer(md->ni);
    if (md->map) FreeBitMap(md->map);
    if (md->img_menu_ti) FreeVec(md->img_menu_ti);

    return TRUE;
}

static void DisposeMenuSkinning(struct menudecor_data *data)
{
}

static BOOL InitMenuSkinning(struct menudecor_data *data, struct DecorImages * di, struct DecorConfig * dc) 
{
    if ((!dc) || (!di))
        return FALSE;
        
    data->dc = dc;

    /* Set pointers to gadget images, used only to get gadget sizes as they
       are requested prior to creation of menu object */
    data->img_amigakey  = di->img_amigakey;
    data->img_menucheck = di->img_menucheck;
    data->img_submenu   = di->img_submenu;

    return TRUE;
}

static IPTR menudecor__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct menudecor_data *data;

    D(bug("menudecor__OM_NEW(tags @ 0x%p)\n", msg->ops_AttrList));

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
        data = INST_DATA(cl, obj);

        struct DecorImages * di = (struct DecorImages *) GetTagData(MDA_DecorImages, (IPTR) NULL, msg->ops_AttrList);
        struct DecorConfig * dc = (struct DecorConfig *) GetTagData(MDA_DecorConfig, (IPTR) NULL, msg->ops_AttrList);

        D(bug("menudecor__OM_NEW: DecorImages @ 0x%p\n", di));
        D(bug("menudecor__OM_NEW: DecorConfig @ 0x%p\n", dc));

        if (!InitMenuSkinning(data, di, dc))
        {
            CoerceMethod(cl, obj ,OM_DISPOSE);
            obj = NULL;
        }
    }
    return (IPTR)obj;
}


static IPTR menudecor__OM_DISPOSE(Class *cl, Object *obj, struct opSet *msg)
{
    struct menudecor_data *data = INST_DATA(cl, obj);

    DisposeMenuSkinning(data);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

static IPTR menudecor_dispatcher(struct IClass *cl, Object *obj, Msg msg)
{
    IPTR retval;

    switch(msg->MethodID)
    {
        case OM_NEW:
            retval = menudecor__OM_NEW(cl, obj, (struct opSet *) msg);
            break;

        case OM_DISPOSE:
            retval = menudecor__OM_DISPOSE(cl, obj, (struct opSet *) msg);
            break;

        case MDM_DRAW_SYSIMAGE:
            retval = menudecor_draw_sysimage(cl, obj, (struct mdpDrawSysImage *)msg);
            break;

        case MDM_GETDEFSIZE_SYSIMAGE:
            retval = menudecor_getdefsizes(cl, obj, (struct mdpGetDefSizeSysImage *) msg);
            break;

        case MDM_DRAWBACKGROUND:
            retval = menudecor_renderbackground(cl, obj, (struct mdpDrawBackground *)msg);
            break;

        case MDM_INITMENU:
            retval = menudecor_initmenu(cl, obj, (struct mdpInitMenu *)msg);
            break;

        case MDM_EXITMENU:
            retval = menudecor_exitmenu(cl, obj, (struct mdpExitMenu *)msg);
            break;

        case MDM_GETMENUSPACES:
            retval = menudecor_getmenuspaces(cl, obj, (struct mdpGetMenuSpaces *)msg);
            break;

        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}

struct IClass * MakeMenuDecorClass()
{
    struct IClass * cl = MakeClass(NULL, MENUDECORCLASS, NULL, sizeof(struct menudecor_data), 0);
    if (cl)
    {
        cl->cl_Dispatcher.h_Entry    = HookEntry;
        cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)menudecor_dispatcher;
    }
    
    return cl;
}

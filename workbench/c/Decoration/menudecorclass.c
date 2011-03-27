/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#include <clib/alib_protos.h>

#include <intuition/intuition.h>
#include <intuition/extensions.h>
#include <intuition/imageclass.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <string.h>

#include "menudecorclass.h"
#include "screendecorclass.h"
#include "drawfuncs.h"
#include "config.h"


#define SETIMAGE_MEN(id) md->img_##id=&sd->img_##id

struct menudecor_data
{
    /* img_menu */
    BOOL    tiled;
    LONG    tile_left;
    LONG    tile_top;
    LONG    tile_right;
    LONG    tile_bottom;
    LONG    inner_left;
    LONG    inner_top;
    LONG    inner_right;
    LONG    inner_bottom;

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
            isset = TRUE;
            break;

        case MENUCHECK:
            n = data->img_menucheck;
            isset = TRUE;
            break;

        case SUBMENUIMAGE:
            n = data->img_submenu;
            isset = TRUE;
            break;

        default:
            return FALSE;
    }

    if (!isset || (n==NULL)) return DoSuperMethodA(cl, obj, (Msg) msg);

    *msg->mdp_Width = n->w;
    *msg->mdp_Height = n->h;
    return TRUE;
}

static IPTR menudecor_getmenuspaces(Class *cl, Object *obj, struct mdpGetMenuSpaces *msg)
{
    struct menudecor_data *data = INST_DATA(cl, obj);

    msg->mdp_InnerLeft =  data->inner_left;
    msg->mdp_InnerTop =  data->inner_top;
    msg->mdp_InnerRight =  data->inner_right;
    msg->mdp_InnerBottom =  data->inner_bottom;
    msg->mdp_ItemInnerLeft = 1;
    msg->mdp_ItemInnerTop = 2;
    msg->mdp_ItemInnerRight = 2;
    msg->mdp_ItemInnerBottom = 1;
    if ((data->tile_left + data->tile_right) > (data->inner_left + data->inner_right)) 
        msg->mdp_MinWidth = data->tile_left + data->tile_right; 
    else 
        msg->mdp_MinWidth = data->inner_left + data->inner_right;

    if ((data->tile_top + data->tile_bottom) > (data->inner_top + data->inner_bottom)) 
        msg->mdp_MinHeight = data->tile_top + data->tile_bottom; 
    else 
        msg->mdp_MinHeight = data->inner_top + data->inner_bottom;

    return TRUE;
}

static IPTR menudecor_draw_sysimage(Class *cl, Object *obj, struct mdpDrawSysImage *msg)
{
    struct ScreenData      *md = (struct ScreenData *) msg->mdp_UserBuffer;
    struct RastPort        *rp = msg->mdp_RPort;
    struct NewImage        *ni = NULL;
    LONG                    state = msg->mdp_State;
    LONG                    left = msg->mdp_X;
    LONG                    top = msg->mdp_Y;
    WORD                    addx = 0;
    WORD                    addy = 0;
    BOOL                    isset = FALSE;

    switch(msg->mdp_Which)
    {
        case AMIGAKEY:
            if (md && md->img_amigakey.ok)
            {
                ni = &md->img_amigakey;
                isset = TRUE;
            }
            break;

        case MENUCHECK:
            if (md && md->img_amigakey.ok)
            {
                ni = &md->img_menucheck;
                isset = TRUE;
            }
            break;

        case SUBMENUIMAGE:
            if (md && md->img_submenu.ok)
            {
                ni = &md->img_submenu;
                isset = TRUE;
            }
            break;

        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    if (!isset || (ni == NULL)) return DoSuperMethodA(cl, obj, (Msg)msg);

    DrawAlphaStateImageToRP(2/*NULL*/, rp, ni, state, left+addx, top+addy, FALSE);

    return TRUE;
}

/**************************************************************************************************/

static IPTR menudecor_renderbackground(Class *cl, Object *obj, struct mdpDrawBackground *msg)
{
    struct RastPort    *rp = msg->mdp_RPort;
    struct NewImage    *ni;
    struct MenuData    *md = (struct MenuData *) msg->mdp_UserBuffer;
    UWORD               flags = msg->mdp_Flags;

    if (!msg->mdp_TrueColor)
    {   

        if ((flags & HIGHITEM) && md->map)
        {
        }
        else
        {
            if (md->map)
            {
                BltBitMapRastPort(md->map, msg->mdp_ItemLeft, msg->mdp_ItemTop, rp, msg->mdp_ItemLeft, msg->mdp_ItemTop, msg->mdp_ItemWidth, msg->mdp_ItemHeight, 0xc0);
                return TRUE;
            }
        }
        return FALSE;

    }

    if ((flags & HIGHITEM) && md->ni)
    {
        ni = NewImageContainer(msg->mdp_ItemWidth, msg->mdp_ItemHeight);
        if (ni)
        {
            DrawPartToImage(md->ni, ni, msg->mdp_ItemLeft, msg->mdp_ItemTop, msg->mdp_ItemWidth, msg->mdp_ItemHeight, 0, 0);
            SetImageTint(ni, 60, 0x00888800);
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

    md->img_menu->istiled       = data->tiled;
    md->img_menu->tile_left     = data->tile_left;
    md->img_menu->tile_top      = data->tile_top;
    md->img_menu->tile_right    = data->tile_right;
    md->img_menu->tile_bottom   = data->tile_bottom;
    md->img_menu->inner_left    = data->inner_left;
    md->img_menu->inner_top     = data->inner_top;
    md->img_menu->inner_right   = data->inner_right;
    md->img_menu->inner_bottom  = data->inner_bottom;
    md->truecolor = msg->mdp_TrueColor;

//    if (!msg->mdp_TrueColor) return DoSuperMethodA(cl, obj, (Msg) msg);

    if (md->truecolor)
    {
        md->ni = GetImageFromRP(rp, msg->mdp_Left, msg->mdp_Top, msg->mdp_Width, msg->mdp_Height);
        if (md->ni) {
            md->ni->ok = TRUE;
            RenderBackground(md->ni, md->img_menu, 20);
        }
    }
    else
    {
        md->map = AllocBitMap(msg->mdp_Width, msg->mdp_Height, 1, BMF_CLEAR, rp->BitMap);
        if (md->map) {
            BltBitMap(rp->BitMap, msg->mdp_Left, msg->mdp_Top, md->map, 0, 0, msg->mdp_Width, msg->mdp_Height, 0xc0, 0xff, NULL);

            TileMapToBitmap(md->img_menu, md->map, msg->mdp_Width, msg->mdp_Height);
        }
    }

    return TRUE;
}

static IPTR menudecor_exitmenu(Class *cl, Object *obj, struct mdpExitMenu *msg)
{
    struct MenuData     *md = (struct MenuData *) msg->mdp_UserBuffer;

    if (md->ni) DisposeImageContainer(md->ni);
    if (md->map) FreeBitMap(md->map);

    return TRUE;
}

static void DisposeMenuSkinning(struct menudecor_data *data)
{
}

static BOOL InitMenuSkinning(STRPTR path, struct menudecor_data *data, struct DecorImages * di) 
{

    char    buffer[256];
    char    *line, *v;
    BPTR    file;
    BPTR    lock;
    BPTR    olddir = 0;

    data->tiled = FALSE;
    data->tile_left = 0;
    data->tile_top = 0;
    data->tile_right = 0;
    data->tile_bottom = 0;
    data->inner_left = 0;
    data->inner_top = 0;
    data->inner_right = 0;
    data->inner_bottom = 0;

    
    lock = Lock(path, ACCESS_READ);
    if (lock)
        olddir = CurrentDir(lock);
    else 
        return FALSE;

    file = Open("Menu/Config", MODE_OLDFILE);
    if (file)
    {
        do
        {
            line = FGets(file, buffer, 256);
            if (line)
            {
                if ((v = strstr(line, "TileLeft ")) == line)
                {
                    data->tile_left = GetInt(v);
                    data->tiled = TRUE;
                }
                else  if ((v = strstr(line, "TileTop ")) == line)
                {
                    data->tile_top = GetInt(v);
                    data->tiled = TRUE;
                }
                else  if ((v = strstr(line, "TileRight ")) == line)
                {
                    data->tile_right = GetInt(v);
                    data->tiled = TRUE;
                }
                else  if ((v = strstr(line, "TileBottom ")) == line)
                {
                    data->tile_bottom = GetInt(v);
                    data->tiled = TRUE;
                }
                else if ((v = strstr(line, "InnerLeft ")) == line)
                {
                    data->inner_left = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerTop ")) == line)
                {
                    data->inner_top = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerRight ")) == line)
                {
                    data->inner_right = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerBottom ")) == line)
                {
                    data->inner_bottom = GetInt(v);
                }
            }
        }
        while(line);
        Close(file);
    }

    /* Set pointers to gadget images, used only to get gadget sizes as their
       are requested prior to creation of menu object */
    data->img_amigakey  = di->img_amigakey;
    data->img_menucheck = di->img_menucheck;
    data->img_submenu   = di->img_submenu;

    if (olddir) CurrentDir(olddir);
    UnLock(lock);

    return TRUE;
}

static IPTR menudecor__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct menudecor_data *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
        data = INST_DATA(cl, obj);

        STRPTR path = (STRPTR) GetTagData(MDA_Configuration, (IPTR) "Theme:", msg->ops_AttrList);
        struct DecorImages * di = (struct DecorImages *) GetTagData(MDA_DecorImages, (IPTR) NULL, msg->ops_AttrList);

        if (!InitMenuSkinning(path, data, di))
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

/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

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


#define PUTIMAGE_MEN(id) data->img_##id=data->sd->img_##id
#define SETIMAGE_MEN(id) md->img_##id=&sd->img_##id

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

    DrawAlphaStateImageToRP(NULL, rp, ni, state, left+addx, top+addy, FALSE);

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
    struct RastPort        *rp = msg->mdp_RPort;
    struct MenuData        *md = (struct MenuData *) msg->mdp_UserBuffer;
    struct ScreenData      *sd = (struct ScreenData *) msg->mdp_ScreenUserBuffer;

    SETIMAGE_MEN(menu);
    SETIMAGE_MEN(amigakey);
    SETIMAGE_MEN(menucheck);
    SETIMAGE_MEN(submenu);

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
    struct MenuData      *md = (struct MenuData *) msg->mdp_UserBuffer;

    if (md->ni) DisposeImageContainer(md->ni);
    if (md->map) FreeBitMap(md->map);

    return TRUE;
}

static IPTR menudecor_getmenuspaces(Class *cl, Object *obj, struct mdpGetMenuSpaces *msg)
{
    struct menudecor_data *data = INST_DATA(cl, obj);

    if (data->img_menu)
    {
        msg->mdp_InnerLeft =  data->img_menu->inner_left;
        msg->mdp_InnerTop =  data->img_menu->inner_top;
        msg->mdp_InnerRight =  data->img_menu->inner_right;
        msg->mdp_InnerBottom =  data->img_menu->inner_bottom;
        msg->mdp_ItemInnerLeft = 1;
        msg->mdp_ItemInnerTop = 2;
        msg->mdp_ItemInnerRight = 2;
        msg->mdp_ItemInnerBottom = 1;
        if ((data->img_menu->tile_left + data->img_menu->tile_right) > (data->img_menu->inner_left + data->img_menu->inner_right)) msg->mdp_MinWidth = data->img_menu->tile_left + data->img_menu->tile_right; else msg->mdp_MinWidth = data->img_menu->inner_left + data->img_menu->inner_right;
        if ((data->img_menu->tile_top + data->img_menu->tile_bottom) > (data->img_menu->inner_top + data->img_menu->inner_bottom)) msg->mdp_MinHeight = data->img_menu->tile_top + data->img_menu->tile_bottom; else msg->mdp_MinHeight = data->img_menu->inner_top + data->img_menu->inner_bottom;
    }
    return TRUE;
}

static void DisposeMenuSkinning(struct menudecor_data *data)
{
}

static BOOL InitMenuSkinning(STRPTR path, struct menudecor_data *data) {

    char    buffer[256];
    char    *line, *v;
    BPTR    file;
    BOOL    tiled = FALSE;
    int     tile_left = 0;
    int     tile_top = 0;
    int     tile_right = 0;
    int     tile_bottom = 0;
    int     inner_left = 0;
    int     inner_top = 0;
    int     inner_right = 0;
    int     inner_bottom = 0;
    BPTR    lock;
    BPTR    olddir = 0;
    
    lock = Lock(path, ACCESS_READ);
    if (lock)
    {
        olddir = CurrentDir(lock);
    }
    else return FALSE;

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
                    tile_left = GetInt(v);
                    tiled = TRUE;
                }
                else  if ((v = strstr(line, "TileTop ")) == line)
                {
                    tile_top = GetInt(v);
                    tiled = TRUE;
                }
                else  if ((v = strstr(line, "TileRight ")) == line)
                {
                    tile_right = GetInt(v);
                    tiled = TRUE;
                }
                else  if ((v = strstr(line, "TileBottom ")) == line)
                {
                    tile_bottom = GetInt(v);
                    tiled = TRUE;
                }
                else if ((v = strstr(line, "InnerLeft ")) == line)
                {
                    inner_left = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerTop ")) == line)
                {
                    inner_top = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerRight ")) == line)
                {
                    inner_right = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerBottom ")) == line)
                {
                    inner_bottom = GetInt(v);
                }
            }
        }
        while(line);
        Close(file);
    }

    PUTIMAGE_MEN(menu);
    PUTIMAGE_MEN(amigakey);
    PUTIMAGE_MEN(menucheck);
    PUTIMAGE_MEN(submenu);

    if (data->img_menu)
    {
        data->img_menu->tile_left = tile_left;
        data->img_menu->tile_right = tile_right;
        data->img_menu->tile_top = tile_top;
        data->img_menu->tile_bottom = tile_bottom;
        data->img_menu->inner_left = inner_left;
        data->img_menu->inner_right = inner_right;
        data->img_menu->inner_top = inner_top;
        data->img_menu->inner_bottom = inner_bottom;
        data->img_menu->istiled = tiled;
    }
    if (olddir) CurrentDir(olddir);
    UnLock(lock);
    if (data->img_menu) return TRUE;
    DisposeMenuSkinning(data);
    return FALSE;
}

static IPTR menudecor__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct menudecor_data *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
        data = INST_DATA(cl, obj);

        STRPTR path = (STRPTR) GetTagData(MDA_Configuration, (IPTR) "Theme:", msg->ops_AttrList);
        data->sd = (struct scrdecor_data *) GetTagData(MDA_ScreenData, 0, msg->ops_AttrList);

        if (!InitMenuSkinning(path, data))
        {
            CoerceMethod(cl,obj,OM_DISPOSE);
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

IPTR MenuDecor_Dispatcher(struct IClass *cl, Object *obj, Msg msg)
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


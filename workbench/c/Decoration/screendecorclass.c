/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#include <clib/alib_protos.h>

#include <graphics/rpattr.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <string.h>

#include "screendecorclass.h"
#include "drawfuncs.h"
#include "config.h"

#define SETIMAGE_SCR(id) sd->di.img_##id = CreateNewImageContainerMatchingScreen(data->di.img_##id, truecolor, screen)

struct scrdecor_data
{
    /* This are original images loaded from disk */
    struct DecorImages di;
    struct DecorConfig * dc;





};

static void DisposeScreenSkinning(struct scrdecor_data *data)
{
    DisposeImageContainer(data->di.img_sdepth);
    DisposeImageContainer(data->di.img_sbarlogo);
    DisposeImageContainer(data->di.img_stitlebar);

    DisposeImageContainer(data->di.img_size);
    DisposeImageContainer(data->di.img_close);
    DisposeImageContainer(data->di.img_depth);
    DisposeImageContainer(data->di.img_zoom);
    DisposeImageContainer(data->di.img_mui);
    DisposeImageContainer(data->di.img_popup);
    DisposeImageContainer(data->di.img_snapshot);
    DisposeImageContainer(data->di.img_iconify);
    DisposeImageContainer(data->di.img_lock);
    DisposeImageContainer(data->di.img_up);
    DisposeImageContainer(data->di.img_down);
    DisposeImageContainer(data->di.img_left);
    DisposeImageContainer(data->di.img_right);
    DisposeImageContainer(data->di.img_winbar_normal);
    DisposeImageContainer(data->di.img_border_normal);
    DisposeImageContainer(data->di.img_border_deactivated);
    DisposeImageContainer(data->di.img_verticalcontainer);
    DisposeImageContainer(data->di.img_verticalknob);
    DisposeImageContainer(data->di.img_horizontalcontainer);
    DisposeImageContainer(data->di.img_horizontalknob);

    DisposeImageContainer(data->di.img_menu);
    DisposeImageContainer(data->di.img_menucheck);
    DisposeImageContainer(data->di.img_amigakey);
    DisposeImageContainer(data->di.img_submenu);

    data->di.img_size = NULL;
    data->di.img_close = NULL;
    data->di.img_depth = NULL;
    data->di.img_zoom = NULL;
    data->di.img_mui = NULL;
    data->di.img_popup = NULL;
    data->di.img_snapshot = NULL;
    data->di.img_iconify = NULL;
    data->di.img_lock = NULL;
    data->di.img_up = NULL;
    data->di.img_down = NULL;
    data->di.img_left = NULL;
    data->di.img_right = NULL;
    data->di.img_winbar_normal = NULL;
    data->di.img_border_normal = NULL;
    data->di.img_border_deactivated = NULL;
    data->di.img_verticalcontainer = NULL;
    data->di.img_verticalknob = NULL;
    data->di.img_horizontalcontainer = NULL;
    data->di.img_horizontalknob = NULL;

    data->di.img_sdepth = NULL;
    data->di.img_sbarlogo = NULL;
    data->di.img_stitlebar = NULL;
}

static BOOL InitScreenSkinning(STRPTR path, struct scrdecor_data *data, struct DecorConfig * dc)
{
    
    BPTR    lock;
    BPTR    olddir = 0;
    ULONG   wgsubimagecols = 4; /* Default value of subimage cols in window gadget */

    if (!dc)
        return FALSE;

    data->dc = dc;
    
    if (dc->GadgetsThreeState) wgsubimagecols = 3;

    lock = Lock(path, ACCESS_READ);
    if (lock)
    {
        olddir = CurrentDir(lock);
    }
    else return FALSE;

    data->di.img_sdepth = GetImageFromFile(path, "System/SDepth/Default", 2, 1);
    data->di.img_stitlebar = GetImageFromFile(path, "System/STitlebar/Default", 1, 1);
    data->di.img_sbarlogo = GetImageFromFile(path, "System/SBarLogo/Default", 1, 1);

    data->di.img_size = GetImageFromFile(path, "System/Size/Default", wgsubimagecols, 1);
    data->di.img_close = GetImageFromFile(path, "System/Close/Default", wgsubimagecols, 1);
    data->di.img_depth = GetImageFromFile(path, "System/Depth/Default", wgsubimagecols, 1);
    data->di.img_zoom = GetImageFromFile(path, "System/Zoom/Default", wgsubimagecols, 1);
    data->di.img_mui = GetImageFromFile(path, "System/MUI/Default", wgsubimagecols, 1);
    data->di.img_popup = GetImageFromFile(path, "System/PopUp/Default", wgsubimagecols, 1);
    data->di.img_snapshot = GetImageFromFile(path, "System/Snapshot/Default", wgsubimagecols, 1);
    data->di.img_iconify = GetImageFromFile(path, "System/Iconify/Default", wgsubimagecols, 1);
    data->di.img_lock = GetImageFromFile(path, "System/Lock/Default", wgsubimagecols, 1);
    data->di.img_up = GetImageFromFile(path, "System/ArrowUp/Default", wgsubimagecols, 1);
    data->di.img_down = GetImageFromFile(path, "System/ArrowDown/Default", wgsubimagecols, 1);
    data->di.img_left = GetImageFromFile(path, "System/ArrowLeft/Default", wgsubimagecols, 1);
    data->di.img_right = GetImageFromFile(path, "System/ArrowRight/Default", wgsubimagecols, 1);
    data->di.img_winbar_normal = GetImageFromFile(path, "System/Titlebar/Default", 1, 2);
    data->di.img_border_normal = GetImageFromFile(path, "System/Borders/Default", 1, 1);
    data->di.img_border_deactivated = GetImageFromFile(path, "System/Borders/Default_Deactivated", 1, 1);
    data->di.img_verticalcontainer = GetImageFromFile(path, "System/Container/Vertical", 2, 1);
    data->di.img_verticalknob = GetImageFromFile(path, "System/Knob/Vertical", 3, 1);
    data->di.img_horizontalcontainer = GetImageFromFile(path, "System/Container/Horizontal", 1, 2);
    data->di.img_horizontalknob = GetImageFromFile(path, "System/Knob/Horizontal", 1, 3);

    data->di.img_menu = GetImageFromFile(path, "Menu/Background/Default", 1, 1);
    data->di.img_amigakey = GetImageFromFile(path, "Menu/AmigaKey/Default", 1, 1);
    data->di.img_menucheck = GetImageFromFile(path, "Menu/Checkmark/Default", 1, 1);
    data->di.img_submenu = GetImageFromFile(path, "Menu/SubMenu/Default", 1, 1);

    if (data->di.img_stitlebar)
    {
        data->di.img_stitlebar->tile_left = 8;
        data->di.img_stitlebar->tile_right = 8;
        data->di.img_stitlebar->tile_top = 9;
        data->di.img_stitlebar->tile_bottom = 8;
    }

    if (olddir) CurrentDir(olddir);
    UnLock(lock);

    if (data->di.img_sdepth) return TRUE;
    DisposeScreenSkinning(data);
    return FALSE;
}





static IPTR scrdecor_new(Class *cl, Object *obj, struct opSet *msg)
{
    struct scrdecor_data   *data;
    UWORD                   barh;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);

    if (obj)
    {
        data = INST_DATA(cl, obj);
        STRPTR path = (STRPTR) GetTagData(SDA_Configuration, (IPTR) "Theme:", msg->ops_AttrList);
        struct DecorConfig * dc = (struct DecorConfig *) GetTagData(SDA_DecorConfig, (IPTR) NULL, msg->ops_AttrList);

        if (!InitScreenSkinning(path, data, dc))
        {
            CoerceMethod(cl,obj,OM_DISPOSE);
            obj = NULL;
        }
        else
        {
            barh = data->dc->SBarHeight;

            if (data->di.img_sbarlogo) if (data->di.img_sbarlogo->h > barh) barh = data->di.img_sbarlogo->h;
            if (data->di.img_stitlebar) if (data->di.img_stitlebar->h > barh) barh = data->di.img_stitlebar->h;
        }
    }
    return (IPTR)obj;
}

static IPTR scrdecor_dispose(Class *cl, Object *obj, struct opSet *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);

    DisposeScreenSkinning(data);

    return 1;
}

/**************************************************************************************************/

static IPTR scrdecor_get(Class *cl, Object *obj, struct opGet *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
        case SDA_TrueColorOnly:
            *msg->opg_Storage = TRUE;
            break;

        case SDA_DecorImages:
            *msg->opg_Storage = (IPTR)&data->di;
            break;

        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }
    return 1;
}

/**************************************************************************************************/

static void scr_findtitlearea(struct Screen *scr, LONG *left, LONG *right)
{
    struct Gadget *g;

    *left = 0;
    *right = scr->Width - 1;

    for (g = scr->FirstGadget; g; g = g->NextGadget)
    {
        if (!(g->Flags & GFLG_RELRIGHT))
        {
            if (g->LeftEdge + g->Width > *left) *left = g->LeftEdge + g->Width;
        }
        else
        {
            if (g->LeftEdge + scr->Width - 1 - 1 < *right) *right = g->LeftEdge + scr->Width - 1 - 1;
        }
    }
}

static IPTR scrdecor_draw_screenbar(Class *cl, Object *obj, struct sdpDrawScreenBar *msg)
{
    struct scrdecor_data   *data = INST_DATA(cl, obj);
    struct ScreenData      *sd = (struct ScreenData *) msg->sdp_UserBuffer;
    struct TextExtent       te;
    struct RastPort        *rp = msg->sdp_RPort;
    struct Screen          *scr = msg->sdp_Screen;
    UWORD                  *pens = msg->sdp_Dri->dri_Pens;
    LONG                    left, right, titlelen = 0;
    BOOL                    hastitle = TRUE;
    BOOL		    beeping = scr->Flags & BEEPING;

    if (beeping) {
        SetAPen(rp, pens[BARDETAILPEN]);
        RectFill(rp, 0, 0, scr->Width, sd->img_stitlebar->h);
    } else {
        if (sd->img_stitlebar->ok)
	        WriteTiledImageHorizontal(rp, sd->img_stitlebar, 0, 0, 
	        sd->img_stitlebar->w, 0, 0, scr->Width);
    }
    if (sd->img_sbarlogo->ok)
        WriteTiledImageHorizontal(rp, sd->img_sbarlogo, 0, 0, 
            sd->img_sbarlogo->w, data->dc->SLogoOffset, 
            (scr->BarHeight + 1 - sd->img_sbarlogo->h) / 2, sd->img_sbarlogo->w);

    if (scr->Title == NULL)
        hastitle = FALSE;

    if (hastitle)
    {
        scr_findtitlearea(scr, &left, &right);
        titlelen = strlen(scr->Title);
        titlelen = TextFit(rp, scr->Title, titlelen, &te, NULL, 1, right - data->dc->STitleOffset, data->dc->SBarHeight);
        if (titlelen == 0) hastitle = 0;
    }

    if (hastitle)
    {
        UWORD tx = data->dc->STitleOffset;
        UWORD ty = (scr->BarHeight + 1 - msg->sdp_Dri->dri_Font->tf_YSize) / 2 + rp->TxBaseline;

        SetFont(rp, msg->sdp_Dri->dri_Font);
        SetDrMd(rp, JAM1);
//         Move(rp, data->dc->STitleOffset, (scr->BarHeight + 1 - msg->sdp_Dri->dri_Font->tf_YSize) / 2 + rp->TxBaseline);
//         Text(rp, scr->Title, titlelen);

        if (!sd->truecolor || ((data->dc->STitleOutline == FALSE) && (data->dc->STitleShadow == FALSE)))
        {
	    SetAPen(rp, pens[beeping ? BARBLOCKPEN : BARDETAILPEN]);
            Move(rp, tx, ty);
            Text(rp, scr->Title, titlelen);
        }
        else if (data->dc->STitleOutline)
        {

                SetSoftStyle(rp, FSF_BOLD, AskSoftStyle(rp));
                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->dc->STitleColorShadow, TAG_DONE);

                Move(rp, tx + 1, ty ); Text(rp, scr->Title, titlelen);
                Move(rp, tx + 2, ty ); Text(rp, scr->Title, titlelen);
                Move(rp, tx , ty ); Text(rp, scr->Title, titlelen);
                Move(rp, tx, ty + 1);  Text(rp, scr->Title, titlelen);
                Move(rp, tx, ty + 2);  Text(rp, scr->Title, titlelen);
                Move(rp, tx + 1, ty + 2);  Text(rp, scr->Title, titlelen);
                Move(rp, tx + 2, ty + 1);  Text(rp, scr->Title, titlelen);
                Move(rp, tx + 2, ty + 2);  Text(rp, scr->Title, titlelen);

                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->dc->STitleColorText, TAG_DONE);
                Move(rp, tx + 1, ty + 1);
                Text(rp, scr->Title, titlelen);
                SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));
        }
        else
        {
                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->dc->STitleColorShadow, TAG_DONE);
                Move(rp, tx + 1, ty + 1 );
                Text(rp, scr->Title, titlelen);

                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->dc->STitleColorText, TAG_DONE);
                Move(rp, tx, ty);
                Text(rp, scr->Title, titlelen);

        }
    }
    return TRUE;
}

static IPTR scrdecor_getdefsize_sysimage(Class *cl, Object *obj, struct sdpGetDefSizeSysImage *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);

    if (msg->sdp_Which == SDEPTHIMAGE)
    {
        if (data->di.img_sdepth)
        {
            *msg->sdp_Height = data->di.img_sdepth->h;
            *msg->sdp_Width = data->di.img_sdepth->w >> 1;
        }
        else return DoSuperMethodA(cl, obj, (Msg) msg);
    }
    else return DoSuperMethodA(cl, obj, (Msg) msg);

    return TRUE;
}

static IPTR scrdecor_draw_sysimage(Class *cl, Object *obj, struct sdpDrawSysImage *msg)
{
    struct ScreenData      *sd = (struct ScreenData *) msg->sdp_UserBuffer;

    struct RastPort        *rp = msg->sdp_RPort;
    LONG                    left = msg->sdp_X;
    LONG                    top = msg->sdp_Y;
    LONG                    state = msg->sdp_State;

    if (msg->sdp_Which == SDEPTHIMAGE)
    {
        if (&sd->img_sdepth)
        {
            DrawStatefulGadgetImageToRP(rp, sd->img_sdepth, state, left, top);
        }
        else return DoSuperMethodA(cl, obj, (Msg) msg);
    }
    else return DoSuperMethodA(cl, obj, (Msg) msg);

    return TRUE;
}

static IPTR scrdecor_layoutscrgadgets(Class *cl, Object *obj, struct sdpLayoutScreenGadgets *msg)
{
    struct Gadget          *gadget = msg->sdp_Gadgets;
    struct ScreenData      *sd = (struct ScreenData *) msg->sdp_UserBuffer;

    struct scrdecor_data   *data = INST_DATA(cl, obj);

    while(gadget)
    {
        switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
        {
            case GTYP_SDEPTH:
                gadget->LeftEdge = -gadget->Width;
                gadget->TopEdge = (data->dc->SBarHeight - sd->img_sdepth->h) >> 1;
                gadget->Flags &= ~GFLG_RELWIDTH;
                gadget->Flags |= GFLG_RELRIGHT;
                break;
        }

        if (msg->sdp_Flags & SDF_LSG_MULTIPLE)
        {
            gadget = gadget->NextGadget;
        }
        else
        {
            gadget = NULL;
        }
    }

    return TRUE;
}


static IPTR scrdecor_initscreen(Class *cl, Object *obj, struct sdpInitScreen *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);
    struct ScreenData *sd = (struct ScreenData *)msg->sdp_UserBuffer;
    struct Screen *screen = msg->sdp_Screen;

    sd->truecolor = msg->sdp_TrueColor;

    BOOL truecolor = sd->truecolor;

    msg->sdp_WBorTop = data->dc->BarHeight - 1 - msg->sdp_FontHeight;
    msg->sdp_BarHBorder = 1;
    msg->sdp_BarHeight = data->dc->SBarHeight - 1; //compatiblity issue
    msg->sdp_WBorLeft = data->dc->LeftBorder;
    msg->sdp_WBorRight = data->dc->RightBorder;
    msg->sdp_WBorBottom = data->dc->BottomBorder;

    sd->ActivePen = -1;
    sd->DeactivePen = -1;
    if (!truecolor) {
        sd->ActivePen = ObtainPen(screen->ViewPort.ColorMap, -1, (data->dc->LUTBaseColors_a << 8) & 0xff000000, (data->dc->LUTBaseColors_a << 16) & 0xff000000, (data->dc->LUTBaseColors_a << 24) & 0xff000000, PEN_EXCLUSIVE);
        sd->DeactivePen = ObtainPen(screen->ViewPort.ColorMap, -1, (data->dc->LUTBaseColors_d << 8) & 0xff000000, (data->dc->LUTBaseColors_d << 16) & 0xff000000, (data->dc->LUTBaseColors_d << 24) & 0xff000000, PEN_EXCLUSIVE);
    }

    /* Convert initial images to current screen */
    /* Make sure a structure is always generated even if there is no image
       That was the assumption of previous code :/ */
    SETIMAGE_SCR(sdepth);
    SETIMAGE_SCR(sbarlogo);
    SETIMAGE_SCR(stitlebar);

    SETIMAGE_SCR(size);
    SETIMAGE_SCR(close);
    SETIMAGE_SCR(depth);
    SETIMAGE_SCR(zoom);
    SETIMAGE_SCR(up);
    SETIMAGE_SCR(down);
    SETIMAGE_SCR(left);
    SETIMAGE_SCR(right);
    SETIMAGE_SCR(mui);
    SETIMAGE_SCR(popup);
    SETIMAGE_SCR(snapshot);
    SETIMAGE_SCR(iconify);
    SETIMAGE_SCR(lock);
    SETIMAGE_SCR(winbar_normal);
    SETIMAGE_SCR(border_normal);
    SETIMAGE_SCR(border_deactivated);
    SETIMAGE_SCR(verticalcontainer);
    SETIMAGE_SCR(verticalknob);
    SETIMAGE_SCR(horizontalcontainer);
    SETIMAGE_SCR(horizontalknob);

    SETIMAGE_SCR(menu);
    SETIMAGE_SCR(amigakey);
    SETIMAGE_SCR(menucheck);
    SETIMAGE_SCR(submenu);

    /* Set pointers to converted images */
    sd->img_sdepth      = sd->di.img_sdepth;
    sd->img_sbarlogo    = sd->di.img_sbarlogo;
    sd->img_stitlebar   = sd->di.img_stitlebar;
    
    sd->img_amigakey    = sd->di.img_amigakey;
    sd->img_menucheck   = sd->di.img_menucheck;
    sd->img_submenu     = sd->di.img_submenu;

    return TRUE;
}

static IPTR scrdecor_exitscreen(Class *cl, Object *obj, struct sdpExitScreen *msg)
{
    struct ScreenData *sd = (struct ScreenData *)msg->sdp_UserBuffer;

    DisposeImageContainer(sd->di.img_sdepth);
    DisposeImageContainer(sd->di.img_sbarlogo);
    DisposeImageContainer(sd->di.img_stitlebar);

    DisposeImageContainer(sd->di.img_size);
    DisposeImageContainer(sd->di.img_close);
    DisposeImageContainer(sd->di.img_depth);
    DisposeImageContainer(sd->di.img_zoom);
    DisposeImageContainer(sd->di.img_up);
    DisposeImageContainer(sd->di.img_down);
    DisposeImageContainer(sd->di.img_left);
    DisposeImageContainer(sd->di.img_right);
    DisposeImageContainer(sd->di.img_mui);
    DisposeImageContainer(sd->di.img_popup);
    DisposeImageContainer(sd->di.img_snapshot);
    DisposeImageContainer(sd->di.img_iconify);
    DisposeImageContainer(sd->di.img_lock);
    DisposeImageContainer(sd->di.img_winbar_normal);
    DisposeImageContainer(sd->di.img_border_normal);
    DisposeImageContainer(sd->di.img_border_deactivated);
    DisposeImageContainer(sd->di.img_verticalcontainer);
    DisposeImageContainer(sd->di.img_verticalknob);
    DisposeImageContainer(sd->di.img_horizontalcontainer);
    DisposeImageContainer(sd->di.img_horizontalknob);

    DisposeImageContainer(sd->di.img_menu);
    DisposeImageContainer(sd->di.img_amigakey);
    DisposeImageContainer(sd->di.img_menucheck);
    DisposeImageContainer(sd->di.img_submenu);

    return TRUE;
}

/**************************************************************************************************/

static IPTR scrdecor_dispatcher(struct IClass *cl, Object *obj, Msg msg)
{
    IPTR retval;
  
    switch(msg->MethodID)
    {
        case OM_NEW:
            retval = scrdecor_new(cl, obj, (struct opSet *)msg);
            break;

        case OM_DISPOSE:
            retval = scrdecor_dispose(cl, obj, (struct opSet *)msg);
            break;

        case OM_GET:
            retval = scrdecor_get(cl, obj, (struct opGet *)msg);
            break;

        case SDM_GETDEFSIZE_SYSIMAGE:
            retval = scrdecor_getdefsize_sysimage(cl, obj, (struct sdpGetDefSizeSysImage *)msg);
            break;

        case SDM_DRAW_SCREENBAR:
            retval = scrdecor_draw_screenbar(cl, obj, (struct sdpDrawScreenBar *)msg);
            break;

        case SDM_DRAW_SYSIMAGE:
            retval = scrdecor_draw_sysimage(cl, obj, (struct sdpDrawSysImage *)msg);
            break;

        case SDM_LAYOUT_SCREENGADGETS:
            retval = scrdecor_layoutscrgadgets(cl, obj, (struct sdpLayoutScreenGadgets *)msg);
            break;

        case SDM_INITSCREEN:
            retval = scrdecor_initscreen(cl, obj, (struct sdpInitScreen *)msg);
            break;

       case SDM_EXITSCREEN:
            retval = scrdecor_exitscreen(cl, obj, (struct sdpExitScreen *)msg);
            break;

        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}

struct IClass * MakeScreenDecorClass()
{
    struct IClass * cl = MakeClass(NULL, SCRDECORCLASS, NULL, sizeof(struct scrdecor_data), 0);
    if (cl)
    {
        cl->cl_Dispatcher.h_Entry    = HookEntry;
        cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)scrdecor_dispatcher;
    }
    
    return cl;
}

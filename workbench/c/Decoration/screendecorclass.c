/*
    Copyright  2011-2012, The AROS Development Team.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <clib/alib_protos.h>

#include <graphics/rpattr.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <string.h>

#include <intuition/cghooks.h>

#include "screendecorclass.h"
#include "drawfuncs.h"
#include "config.h"

#define SETIMAGE_SCR(id) sd->di->img_##id = CreateNewImageContainerMatchingScreen(data->di->img_##id, truecolor, screen)

#define CHILDPADDING 1

struct scrdecor_data
{
    struct Screen *Screen;
    /* These are original images loaded from disk */
    struct DecorImages * di;
    struct DecorConfig * dc;
    Object *FirstChild;
};

static void DisposeScreenSkinning(struct scrdecor_data *data)
{
}

static BOOL InitScreenSkinning(struct scrdecor_data *data, struct DecorImages * di, struct DecorConfig * dc)
{
    if ((!dc) || (!di))
        return FALSE;

    data->dc = dc;
    data->di = di;

    if (data->di->img_sdepth) return TRUE;

    DisposeScreenSkinning(data);
    return FALSE;
}





static IPTR scrdecor_new(Class *cl, Object *obj, struct opSet *msg)
{
    struct scrdecor_data   *data;
    UWORD                   barh;

    D(bug("scrdecor_new(tags @ 0x%p)\n", msg->ops_AttrList));
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);

    if (obj)
    {
        data = INST_DATA(cl, obj);

        struct DecorConfig * dc = (struct DecorConfig *) GetTagData(SDA_DecorConfig, (IPTR) NULL, msg->ops_AttrList);
        struct DecorImages * di = (struct DecorImages *) GetTagData(SDA_DecorImages, (IPTR) NULL, msg->ops_AttrList);

        D(bug("scrdecor_new: DecorImages @ 0x%p\n", di));
        D(bug("scrdecor_new: DecorConfig @ 0x%p\n", dc));

        data->FirstChild = NULL;

        if (!InitScreenSkinning(data, di, dc))
        {
            CoerceMethod(cl,obj,OM_DISPOSE);
            obj = NULL;
        }
        else
        {
            barh = data->dc->SBarHeight;

            if (data->di->img_sbarlogo) if (data->di->img_sbarlogo->h > barh) barh = data->di->img_sbarlogo->h;
            if (data->di->img_stitlebar) if (data->di->img_stitlebar->h > barh) barh = data->di->img_stitlebar->h;
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
    switch(msg->opg_AttrID)
    {
        case SDA_TrueColorOnly:
            *msg->opg_Storage = TRUE;
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

static IPTR scrdecor_set(Class *cl, Object *obj, struct opSet *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tstate;
    struct TagItem *tag;

    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case SDA_TitleChild: 
                if (tag->ti_Data)
                {
                    if (!(data->FirstChild))
                    {
                        LONG                    left, right = 0;
                        struct GadgetInfo childgadinf;
                        struct gpLayout childlayoutmsg;
                        D(bug("[screendecor] SDA_TitleChild: 0x%p\n", tag->ti_Data));

                        if ((childgadinf.gi_Screen = LockPubScreen(NULL)) != NULL)
                        {
                            UnlockPubScreen(NULL, childgadinf.gi_Screen);
                            data->FirstChild = (Object *)tag->ti_Data;
                            ((struct Gadget *)(data->FirstChild))->SpecialInfo = childgadinf.gi_Screen;

                            childgadinf.gi_RastPort = childgadinf.gi_Screen->BarLayer->rp;

                            if ((childgadinf.gi_DrInfo = GetScreenDrawInfo(childgadinf.gi_Screen)) != NULL)
                            {
                                childgadinf.gi_Pens.DetailPen = childgadinf.gi_DrInfo->dri_Pens[DETAILPEN];
                                childgadinf.gi_Pens.BlockPen = childgadinf.gi_DrInfo->dri_Pens[BLOCKPEN];
                            }

                            childlayoutmsg.MethodID = GM_LAYOUT;
                            childlayoutmsg.gpl_GInfo = &childgadinf;
                            childlayoutmsg.gpl_Initial = 0;

                            ((struct Gadget *)(data->FirstChild))->TopEdge = 0 + CHILDPADDING;
                            ((struct Gadget *)(data->FirstChild))->Height = childgadinf.gi_Screen->BarHeight - (CHILDPADDING << 1);

                            DoMethodA(data->FirstChild, &childlayoutmsg);

                            scr_findtitlearea((childgadinf.gi_Screen), &left, &right);

                            ((struct Gadget *)(data->FirstChild))->LeftEdge = right - ((struct Gadget *)(data->FirstChild))->Width + 1;

                            childlayoutmsg.MethodID = GM_GOACTIVE;
                            DoMethodA(data->FirstChild, &childlayoutmsg);
                        }
                    }
                }
                break;
            default:
                return DoSuperMethodA(cl, obj, (Msg)msg);
        }
    }
    return 1;
}

static IPTR scrdecor_draw_screenbar(Class *cl, Object *obj, struct sdpDrawScreenBar *msg)
{
    struct scrdecor_data   *data = INST_DATA(cl, obj);
    struct ScreenData      *sd = (struct ScreenData *) msg->sdp_UserBuffer;
    struct TextExtent       te;
    struct RastPort        *rp = msg->sdp_RPort;
    struct Screen          *scr = msg->sdp_Screen;
    struct DrawInfo        *dri = msg->sdp_Dri;
    UWORD                  *pens = dri->dri_Pens;
    LONG                    left, right = 0, titlelen = 0;
    BOOL                    hastitle = TRUE;
    BOOL		    beeping = scr->Flags & BEEPING;

    if (beeping) {
        SetAPen(rp, pens[BARDETAILPEN]);
        RectFill(rp, 0, 0, scr->Width, sd->img_stitlebar->h);
    } else {
        if (sd->img_stitlebar->ok)
            WriteVerticalScaledTiledImageHorizontal(rp, sd->img_stitlebar, 0, 0,
                sd->img_stitlebar->w, 0, 0, data->dc->SBarHeight, scr->Width, scr->BarHeight + 1);
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
        if (data->FirstChild)
        {
            if (((struct Gadget *)(data->FirstChild))->Width > 2) {
                D(bug("[screendecor] draw_screenbar: titlechild width = %d\n", ((struct Gadget *)(data->FirstChild))->Width));
                right = right - ((struct Gadget *)(data->FirstChild))->Width + 1; 
            }
        }
        titlelen = TextFit(rp, scr->Title, titlelen, &te, NULL, 1, right - data->dc->STitleOffset, scr->BarHeight);
        if (titlelen == 0) hastitle = 0;
    }

    if (hastitle)
    {
        UWORD tx = data->dc->STitleOffset;
        UWORD tymax = scr->BarHeight - (dri->dri_Font->tf_YSize - dri->dri_Font->tf_Baseline) - 1;
        UWORD ty = ((scr->BarHeight + dri->dri_Font->tf_Baseline - 1) >> 1);
        if (ty > tymax) ty = tymax;

        SetFont(rp, msg->sdp_Dri->dri_Font);
        SetDrMd(rp, JAM1);

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

    if (data->FirstChild && (((struct Gadget *)(data->FirstChild))->Width > 0)) {
        struct GadgetInfo childgadinf;
        struct gpRender childrendermsg =
        {
            GM_RENDER,
            &childgadinf,
            rp,
            GREDRAW_REDRAW
        };

        childgadinf.gi_Screen = ((struct Gadget *)(data->FirstChild))->SpecialInfo = scr;
        childgadinf.gi_RastPort = rp;
        childgadinf.gi_Pens.DetailPen = pens[DETAILPEN];
        childgadinf.gi_Pens.BlockPen = pens[BLOCKPEN];
        childgadinf.gi_DrInfo = dri;
        childgadinf.gi_Domain.Left = right;
        childgadinf.gi_Domain.Top = 0 + CHILDPADDING;
        childgadinf.gi_Domain.Width = ((struct Gadget *)(data->FirstChild))->Width;
        childgadinf.gi_Domain.Height = sd->img_stitlebar->h - (CHILDPADDING << 1);
        D(bug("[screendecor] draw_screenbar: rendering titlechild @ 0x%p, msg @ 0x%p, info @ 0x%p\n", data->FirstChild, &childrendermsg, &childgadinf));
        DoMethodA(data->FirstChild, &childrendermsg);

    }
    return TRUE;
}

static IPTR scrdecor_getdefsize_sysimage(Class *cl, Object *obj, struct sdpGetDefSizeSysImage *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);

    if (msg->sdp_Which == SDEPTHIMAGE)
    {
        if (data->di && data->di->img_sdepth)
        {
            *msg->sdp_Height = data->di->img_sdepth->h;
            *msg->sdp_Width = data->di->img_sdepth->w >> 1;
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
    LONG                    width = msg->sdp_Width;
    LONG                    height = msg->sdp_Height;
    LONG                    state = msg->sdp_State;

    if (msg->sdp_Which == SDEPTHIMAGE)
    {
        if (&sd->img_sdepth)
        {
            DrawScaledStatefulGadgetImageToRP(rp, sd->img_sdepth, state, left, top, width, height);
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

    /*
     * This depends on openwindow.c adding font height to w->BorderTop.
     * It allows the window title bar to grow when font is bigger than decoration defined height
     */
    if ((msg->sdp_FontHeight + 2) > data->dc->BarHeight)
        msg->sdp_WBorTop = 1;
    else
        msg->sdp_WBorTop = data->dc->BarHeight - 1 - msg->sdp_FontHeight;

    /* Allow scaling title bar above decoration defined height */
    msg->sdp_BarHeight = msg->sdp_FontHeight > (data->dc->SBarHeight - 1) ? msg->sdp_FontHeight : (data->dc->SBarHeight - 1);

    msg->sdp_BarHBorder = 1;
    msg->sdp_WBorLeft = data->dc->LeftBorder;
    msg->sdp_WBorRight = data->dc->RightBorder;
    msg->sdp_WBorBottom = data->dc->BottomBorder;

    sd->ActivePen = -1;
    sd->DeactivePen = -1;
#if 0 // Omitting this gives better colours on low-color screens
    if (!truecolor) {
        sd->ActivePen = ObtainPen(screen->ViewPort.ColorMap, -1, (data->dc->LUTBaseColors_a << 8) & 0xff000000, (data->dc->LUTBaseColors_a << 16) & 0xff000000, (data->dc->LUTBaseColors_a << 24) & 0xff000000, PEN_EXCLUSIVE);
        sd->DeactivePen = ObtainPen(screen->ViewPort.ColorMap, -1, (data->dc->LUTBaseColors_d << 8) & 0xff000000, (data->dc->LUTBaseColors_d << 16) & 0xff000000, (data->dc->LUTBaseColors_d << 24) & 0xff000000, PEN_EXCLUSIVE);
    }
#endif

    /* Convert initial images to current screen */
    /* Make sure a structure is always generated even if there is no image
       That was the assumption of previous code :/ */

    sd->di = NewImages();

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
    sd->img_sdepth      = sd->di->img_sdepth;
    sd->img_sbarlogo    = sd->di->img_sbarlogo;
    sd->img_stitlebar   = sd->di->img_stitlebar;
    
    sd->img_amigakey    = sd->di->img_amigakey;
    sd->img_menucheck   = sd->di->img_menucheck;
    sd->img_submenu     = sd->di->img_submenu;

    return TRUE;
}

static IPTR scrdecor_exitscreen(Class *cl, Object *obj, struct sdpExitScreen *msg)
{
    struct ScreenData *sd = (struct ScreenData *)msg->sdp_UserBuffer;

    FreeImages(sd->di);

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

        case OM_SET:
            retval = scrdecor_set(cl, obj, (struct opSet *)msg);
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

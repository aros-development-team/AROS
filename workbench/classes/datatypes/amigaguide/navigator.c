/*
** $PROJECT: amigaguide.datatype
**
** $VER: navigator.c 50.1 (07.06.03)
**
** $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
**
*/

/* ------------------------------- includes ------------------------------- */

#include <intuition/imageclass.h>
#include <graphics/gfxmacros.h>

/* no debugging output for navigation bar */
#undef DEBUG
#include "classbase.h"

#include "navigator.h"

#undef INSTDATA
#define INSTDATA  struct NavigatorData *data = INST_DATA(cl,obj)


/* ----------------------------- definitions ------------------------------ */

struct NavigatorData
{
   struct NavigatorButton *nd_Buttons;
   Object *nd_Target;
   struct Region *nd_Region;
   UWORD nd_DomainWidth;
   UWORD nd_MaxWidth;
   WORD nd_Width;
   WORD nd_XDistance;
   UBYTE nd_NumButtons;
   UBYTE nd_Height;
   BYTE nd_Pressed;
   BYTE nd_Selected;

   BOOL nd_Updating;
};

#define LBUTTON_UP         (IECODE_LBUTTON | IECODE_UP_PREFIX)
#define LBUTTON_DOWN       (IECODE_LBUTTON)

/* number of pixel to add to max text width */
#define INNERSPACE         10

/* --------------------------- public interface --------------------------- */

static ULONG navclass_dispatcher(Class *cl, Object *obj, Msg msg);

static ClassCall
ULONG dispatcher REGARGS((REG(a0,Class *cl),
                          REG(a2,Object *obj),
                          REG(a1,Msg msg)))
{
   MREG(a0, Class *, cl);
   MREG(a2, Object *, obj);
   MREG(a1, Msg, msg);

   return navclass_dispatcher(cl, obj, msg);
}
#ifdef __MORPHOS__
static struct EmulLibEntry GATEhook = {
    TRAP_LIB, 0, (void (*)(void)) dispatcher
};
#endif


Class *MakeNavigatorClass(struct ClassBase *cb)
{
   Class *cl;

   if((cl = MakeClass(NULL,"gadgetclass",NULL,sizeof(struct NavigatorData),0)) != NULL)
   {
#ifdef __MORPHOS__
      cl->cl_Dispatcher.h_Entry = (HOOKFUNC) &GATEhook;
#else
      cl->cl_Dispatcher.h_Entry = (HOOKFUNC) dispatcher;
#endif
      cl->cl_UserData = (ULONG) cb;
   }

   return cl;
}
BOOL FreeNavigatorClass(struct ClassBase *cb, Class *cl)
{
   return FreeClass(cl);
}

/* ---------------------------- implementation ---------------------------- */

static
struct Region *installregion(Class *cl, Object *obj, struct GadgetInfo *ginfo)
{
   CLASSBASE;
   INSTDATA;
   struct Rectangle rect;

   if((data->nd_Region = NewRegion()) != NULL)
   {
      rect.MinX = CAST_GAD(obj)->LeftEdge;
      rect.MinY = CAST_GAD(obj)->TopEdge;
      rect.MaxX = rect.MinX + data->nd_DomainWidth;
      rect.MaxY = rect.MinY + CAST_GAD(obj)->Height;

      return InstallClipRegionSafe(cb,ginfo, data->nd_Region, &rect);
   }

   return NULL;
}
static
void uninstallregion(Class *cl, Object *obj,
		     struct GadgetInfo *ginfo, struct Region *oldreg)
{
   CLASSBASE;
   struct NavigatorData *data = INST_DATA(cl,obj);

   UnInstallClipRegionSafe(cb, ginfo, oldreg);

   if(data->nd_Region != NULL)
      DisposeRegion(data->nd_Region);
}

static
void draw_button(Class *cl, Object *obj, struct GadgetInfo *ginfo,
		 struct RastPort *rp, ULONG idx)
{
   CLASSBASE;
   INSTDATA;

   struct TextExtent textext;
   UWORD *pens = ginfo->gi_DrInfo->dri_Pens;
   struct NavigatorButton *button = &data->nd_Buttons[idx];

   ULONG x = (data->nd_Width + data->nd_XDistance) * idx + CAST_GAD(obj)->LeftEdge;
   ULONG y = CAST_GAD(obj)->TopEdge;

   ULONG txtoffset;
   ULONG status;
   ULONG chrs;

   chrs = TextFit(rp,button->nb_Text,strlen(button->nb_Text),&textext,NULL,1,data->nd_Width-4,ginfo->gi_DrInfo->dri_Font->tf_YSize+1);
   txtoffset = (data->nd_Width - textext.te_Width) / 2;

   if(button->nb_Flags & NBF_DISABLED)
      status = IDS_DISABLED;
   else if(button->nb_Flags & NBF_SELECTED)
      status = IDS_SELECTED;
   else
      status = IDS_NORMAL;

   DrawImageState(rp, CAST_GAD(obj)->GadgetRender,x,y,status,ginfo->gi_DrInfo);

   SetABPenDrMd(rp,pens[TEXTPEN],0,JAM1);
   Move(rp,x + txtoffset,y + ginfo->gi_DrInfo->dri_Font->tf_Baseline + 1);
   Text(rp,button->nb_Text,chrs);

   if(status == IDS_DISABLED)
   {
      const UWORD pattern[2] = {0x1111,0x4444};
      SetAfPt(rp,(UWORD *) &pattern[0],1);
      RectFill(rp,x+2,y+1,x+data->nd_Width-2,y+rp->TxHeight+1);
      SetAfPt(rp,NULL,0);
   }

   button->nb_Flags &= ~(NBF_NEEDRENDERING);
}
static
BOOL gi_draw_button(Class *cl, Object *obj,
		    struct GadgetInfo *ginfo,
		    ULONG idx)
{
   CLASSBASE;
   struct RastPort *rp;

   if((rp = ObtainGIRPort(ginfo)) != NULL)
   {
      struct Region *oldreg;

      oldreg = installregion(cl,obj,ginfo);

      draw_button(cl,obj,ginfo,rp,idx);

      uninstallregion(cl,obj,ginfo,oldreg);

      ReleaseGIRPort(rp);
   }

   return (BOOL) (rp != NULL);
}

static
ULONG om_new(Class *cl, Object *obj, struct opSet *msg)
{
   CLASSBASE;
   INSTDATA;
   ULONG rv = 0;

   CAST_GAD(obj)->Flags |= GFLG_RELSPECIAL;

   data->nd_Target     = (Object *) GetTagData(NA_Target,(ULONG) NULL,CAST_SET(msg)->ops_AttrList);
   data->nd_XDistance  = GetTagData(NA_Distance,10,CAST_SET(msg)->ops_AttrList);
   data->nd_MaxWidth   = 0;
   data->nd_Pressed    = -1;
   data->nd_Selected   = -1;

   if((data->nd_Buttons    = (struct NavigatorButton *)
       GetTagData(NA_Buttons,(ULONG) NULL,CAST_SET(msg)->ops_AttrList)) != NULL)
   {
      if((CAST_GAD(obj)->GadgetRender = NewObject(NULL,"frameiclass",
						IA_FrameType,FRAME_BUTTON,
						TAG_DONE)) != NULL)
      {
	 rv = (ULONG) obj;
      }
   }

   return rv;
}

static
ULONG om_get(Class *cl, Object *obj, struct opGet *msg)
{
   INSTDATA;

   switch(msg->opg_AttrID)
   {
   case NVA_Selected:
      *msg->opg_Storage = data->nd_Selected;
      break;
   default:
      return DoSuperMethodA(cl,obj,(Msg) msg);
   }
   return 1;
}
static
ULONG gm_input(Class *cl, Object *obj, struct gpInput *msg)
{
   struct InputEvent *ie = msg->gpi_IEvent;
   ULONG rv;

   if(ie->ie_Class == IECLASS_RAWMOUSE)
   {
      INSTDATA;

      LONG width  = data->nd_Width + data->nd_XDistance;
      LONG mx     = msg->gpi_Mouse.X;
      LONG my     = msg->gpi_Mouse.Y;
      LONG bt     = mx / ((width) ? width : 1);
      LONG btx    = data->nd_Pressed * width;

      struct NavigatorButton *button = &data->nd_Buttons[bt];
      BOOL domethod = FALSE;

      switch(ie->ie_Code)
      {
      case LBUTTON_UP:
	 if(data->nd_Selected != -1)
	 {
	    button->nb_Flags &= ~NBF_SELECTED;

	    gi_draw_button(cl,obj,msg->gpi_GInfo,data->nd_Pressed);

	    domethod = TRUE;
	    data->nd_Selected = -1;
#ifndef __AROS__ /* debug output requires sysbase */
	    DB(("should do action for button : %ld\n",data->nd_Pressed));
#endif
	 }
	 rv = GMR_NOREUSE;
	 break;
      case LBUTTON_DOWN:
	 data->nd_Pressed = bt;
	 btx = bt * width;
      default:
	 if(my > 0   && my < data->nd_Height &&
	    mx > btx && mx < btx + data->nd_Width &&
	    bt < data->nd_NumButtons)
	 {
	    if(bt != data->nd_Selected && !(data->nd_Buttons[bt].nb_Flags & NBF_DISABLED))
	    {
	       button->nb_Flags |= NBF_SELECTED;
	       gi_draw_button(cl,obj,msg->gpi_GInfo,bt);
	       data->nd_Selected = bt;
	    }
	 } else if(data->nd_Selected != -1)
	 {
	    data->nd_Buttons[data->nd_Pressed].nb_Flags &= ~NBF_SELECTED;
	    gi_draw_button(cl,obj,msg->gpi_GInfo,data->nd_Pressed);
	    data->nd_Selected = -1;
	 }
	 rv = GMR_MEACTIVE;
      }

      if(domethod)
      {
	 NotifyAttrs(obj,msg->gpi_GInfo,0,
		     NA_Command, button->nb_Command,
		     TAG_DONE);
      }
   } else
      rv = DoSuperMethodA(cl,obj,(Msg) msg);

   return rv;
}
static
ULONG gm_layout(Class *cl, Object *obj, struct gpLayout *msg)
{
   CLASSBASE;
   INSTDATA;

   struct GadgetInfo *ginfo = msg->gpl_GInfo;
   struct IBox *domain;

   if(GetAttr(DTA_Domain, data->nd_Target, (ULONG *) &domain))
      data->nd_DomainWidth = domain->Width - 1;

   /* change left edge according to master object */
   CAST_GAD(obj)->TopEdge  = CAST_GAD(data->nd_Target)->TopEdge;
   CAST_GAD(obj)->LeftEdge = CAST_GAD(data->nd_Target)->LeftEdge;

   DB(("Domain Width : %lx , %ld\n", (ULONG) data->nd_Target, data->nd_DomainWidth));

   if(msg->gpl_Initial)
   {
      struct RastPort trp;
      struct NavigatorButton *buttons = data->nd_Buttons;
      struct TextFont *font = ginfo->gi_DrInfo->dri_Font;
      ULONG width;
      ULONG max = 0;

      InitRastPort(&trp);
      SetFont(&trp,font);

      CAST_GAD(obj)->Height = font->tf_YSize + 3;
      data->nd_Height  = font->tf_YSize + 2;
      data->nd_NumButtons = 0;

      while(buttons->nb_Text != NULL)
      {
	 width = TextLength(&trp,buttons->nb_Text,strlen(buttons->nb_Text));
	 if(width > max)
	    max = width;
	 buttons++;
	 data->nd_NumButtons++;
      }

      data->nd_MaxWidth = max + INNERSPACE;
      data->nd_Width    = data->nd_MaxWidth;
   }

   data->nd_XDistance = 2;
   data->nd_Width =  (data->nd_DomainWidth - (2 * data->nd_NumButtons)) /
		    ((data->nd_NumButtons == 0) ? 1 : data->nd_NumButtons);
   if(data->nd_Width < 20)
      data->nd_Width = 20;

   SetAttrs(CAST_GAD(obj)->GadgetRender,
	    IA_Width,  data->nd_Width,
	    IA_Height, data->nd_Height,
	    TAG_DONE);

   return 0;
}
static
ULONG gm_render(Class *cl, Object *obj, struct gpRender *msg)
{
   CLASSBASE;
   INSTDATA;

   struct GadgetInfo *ginfo = msg->gpr_GInfo;
   struct Region *oldreg;
   ULONG idx = 0;

   oldreg = installregion(cl,obj,ginfo);

   SetFont(msg->gpr_RPort, ginfo->gi_DrInfo->dri_Font);

   while(idx < data->nd_NumButtons)
   {
      if(msg->gpr_Redraw == GREDRAW_REDRAW || (data->nd_Buttons[idx].nb_Flags & NBF_NEEDRENDERING))
	 draw_button(cl,obj,ginfo,msg->gpr_RPort,idx);
      idx++;
   }

   uninstallregion(cl,obj,ginfo,oldreg);

   return 0;
}
static
ULONG nvm_changestatus(Class *cl,Object *obj,struct npChangeStatus *msg)
{
   INSTDATA;
   struct NavigatorButton *button;
   ULONG idx = 0;
   LONG rv = 0;
   LONG i;

   for(i = 0; i < msg->np_NumCommands ; i++)
   {
#ifndef __AROS__ /* debug output requires sysbase */   
      DB(("change status for %ld\n", msg->np_Commands[i].ns_Command));
#endif

      button = data->nd_Buttons;
      while(button->nb_Text != NULL)
      {
	 if(button->nb_Command == msg->np_Commands[i].ns_Command)
	 {
	    BOOL render = FALSE;
	    LONG status = msg->np_Commands[i].ns_Status;

#ifndef __AROS__ /* debug output requires sysbase */   
	    DB(("%ld[%ld] flags : %ld , change to %ld\n",
		msg->np_Commands[i].ns_Command, i, button->nb_Flags, status));
#endif

	    if((status & NVS_MASK) == NVS_ENABLE)
	    {
	       render = (button->nb_Flags & NBF_DISABLED);
	       button->nb_Flags &= ~NBF_DISABLED;
	    } else if((status & NVS_MASK) == NVS_DISABLE)
	    {
	       render = !(button->nb_Flags & NBF_DISABLED);
	       button->nb_Flags |= NBF_DISABLED;
	    }

#ifndef __AROS__ /* debug output requires sysbase */   
	    DB(("render : %ld flags now : %ld\n",
		render, button->nb_Flags));
#endif

	    if(render != FALSE)
	    {
	       if((status & NVF_RENDER))
	       {
		  if(!gi_draw_button(cl,obj,msg->np_GInfo,idx))
		     ++rv;
	       } else
	       {
		  button->nb_Flags |= NBF_NEEDRENDERING;
		  ++rv;
	       }

#ifndef __AROS__ /* debug output requires sysbase */   
	       DB(("nav button need rendering : \"%s\"%s rendered !\n",
		   button->nb_Text, (rv == 1) ? "" : " not"));
#endif
	    }
	    break;
	 }
	 idx++;
	 button++;
      }
   }

   return rv;
}
static
ULONG nvm_changed(Class *cl, Object *obj, Msg msg)
{
   INSTDATA;
   struct NavigatorButton *button = data->nd_Buttons;
   LONG rv = 0;

   while(button->nb_Text != NULL)
   {
      if(button->nb_Flags & NBF_NEEDRENDERING)
	 rv++;
      button++;
   }

   return rv;
}

static
ULONG navclass_dispatcher(Class *cl, Object *obj, Msg msg)
{
   CLASSBASE;
   LONG rv = 0;

   switch(msg->MethodID)
   {
   case OM_NEW:
      if((rv = DoSuperMethodA(cl,obj,msg)) != 0)
      {
	 Object *newobj = (Object *) rv;

	 if((rv = om_new(cl, newobj, CAST_SET(msg))) == (ULONG) NULL)
	    CoerceMethod(cl, newobj, OM_DISPOSE);
      }
      break;
   case OM_DISPOSE:
      DisposeObject(CAST_GAD(obj)->GadgetRender);
      rv = DoSuperMethodA(cl, obj, msg);
      break;
   case OM_GET:
      rv = om_get(cl, obj, (struct opGet *) msg);
      break;
   case GM_GOACTIVE:
   case GM_HANDLEINPUT:
      rv = gm_input(cl, obj, (struct gpInput *) msg);
      break;
   case GM_LAYOUT:
      rv = gm_layout(cl, obj, (struct gpLayout *) msg);
      break;
   case GM_RENDER:
      rv = gm_render(cl, obj, (struct gpRender *) msg);
      break;
   case NVM_CHANGESTATUS:
      rv = nvm_changestatus(cl, obj, (struct npChangeStatus *) msg);
      break;
   case NVM_CHANGED:
      rv = nvm_changed(cl, obj, msg);
      break;
   default:
      rv = DoSuperMethodA(cl, obj, msg);
   }

   return rv;
}


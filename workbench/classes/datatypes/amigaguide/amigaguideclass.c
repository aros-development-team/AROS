/*
** $PROJECT: amigaguide.datatype
**
** $VER: amigaguideclass.c 50.1 (13.06.03)
**
** $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
**
*/

/* ------------------------------- includes ------------------------------- */

#include "classbase.h"
#include <libraries/diskfont.h>

/* ------------------------------ prototypes ------------------------------ */

ULONG AmigaGuide__OM_NEW(Class *cl, Object *obj, struct opSet *msg);
ULONG AmigaGuide__OM_DISPOSE(Class *cl, Object *obj, Msg msg);
static ULONG om_set(Class *cl, Object *obj, struct opSet *msg);
static ULONG om_update(Class *cl, Object *obj, struct opSet *msg);

ULONG AmigaGuide__GM_RENDER(Class *cl, Object *obj, struct gpRender *msg);
ULONG AmigaGuide__GM_LAYOUT(Class *cl, Object *obj, struct gpLayout *msg);
ULONG AmigaGuide__GM_HANDLEINPUT(Class *cl, Object *obj, struct gpInput *msg);

ULONG AmigaGuide__DTM_ASYNCLAYOUT(Class *cl, Object *obj, struct gpLayout *msg);
ULONG AmigaGuide__DTM_TRIGGER(Class *cl, Object *obj, struct dtTrigger *msg);


static void ActivateAGObject(Class *cl, Object *obj, struct GadgetInfo *ginfo,
			     struct AmigaGuideObject *agobj);

/* ------------------------------ constants ------------------------------- */

static const struct NavigatorButton navbuttons[] =
{
   {"Contents", STM_CONTENTS, NBF_DISABLED},
   {"Index",    STM_INDEX, NBF_DISABLED},
   {"Help",     STM_HELP, 0},
   {"Retrace",  STM_RETRACE, NBF_DISABLED},
   {"< Browse", STM_BROWSE_PREV, NBF_DISABLED},
   {"> Browse", STM_BROWSE_NEXT, NBF_DISABLED},
   {NULL, 0, 0}
};

static const struct DTMethod triggermethods[] =
{
   {"Contents", "CONTENTS", STM_CONTENTS},
   {"Index",    "INDEX",    STM_INDEX},
   {"Help",     "HELP",     STM_HELP},
   {"Retrace",  "RETRACE",  STM_RETRACE},
   {"< Browse", "PREVIOUS", STM_BROWSE_PREV},
   {"> Browse", "NEXT",     STM_BROWSE_NEXT},
   {"Next Field", NULL,     STM_NEXT_FIELD},
   {"Prev Field", NULL,     STM_PREV_FIELD},
   {"Activate Field", NULL, STM_ACTIVATE_FIELD},
   {NULL, NULL, 0}
};

static const struct TagItem navmap[] =
{
   {NA_Command, AGNA_Command},
   {TAG_END,0}
};

static const ULONG supported_methods[] =
{
   DTM_COPY,
   DTM_SELECT,
   DTM_CLEARSELECTED,
   DTM_WRITE,
   DTM_PRINT,
   DTM_TRIGGER,
   DTM_GOTO,
   ~0,
};

/* -------------------------- user library init --------------------------- */

BOOL UserClassBaseOpen(struct ClassBase *cb)
{
   cb->cb_Navigator = MakeNavigatorClass(cb);
   cb->cb_NodeClass = MakeNodeClass(cb);
   if(cb->cb_Navigator != NULL &&
      cb->cb_NodeClass != NULL)
   {
      return TRUE;
   }

   return FALSE;
}
#ifdef __AROS__
BOOL UserClassBaseClose(struct ClassBase *cb)
#else
void UserClassBaseClose(struct ClassBase *cb)
#endif
{
   if(cb->cb_Navigator != NULL)
      if(FreeNavigatorClass(cb, cb->cb_Navigator))
         cb->cb_Navigator = NULL;
   if(cb->cb_NodeClass != NULL)
      if(FreeNodeClass(cb, cb->cb_NodeClass))
         cb->cb_NodeClass = NULL;
    
#ifdef __AROS__
    return TRUE;
#endif
}

#ifdef __AROS__
#include <aros/symbolsets.h>

ADD2INITLIB(UserClassBaseOpen, -1);
ADD2EXPUNGELIB(UserClassBaseClose, -1);
#endif /* __AROS__ */

/* ------------------------------ functions ------------------------------- */

static
void MakeNavGadget(Class *cl, Object *obj, struct IBox *domain)
{
   CLASSBASE;
   INSTDATA;

   if(data->ag_Gadget != NULL)
      return;

   data->ag_Gadget =
      NewObject(cb->cb_Navigator,NULL,
	        ((CAST_GAD(obj)->Flags & GFLG_RELRIGHT)  == GFLG_RELRIGHT)  ? GA_RelRight  : GA_Left, domain->Left,
	        ((CAST_GAD(obj)->Flags & GFLG_RELBOTTOM) == GFLG_RELBOTTOM) ? GA_RelBottom : GA_Top, domain->Top,
	        GA_RelWidth, CAST_GAD(obj)->Width,
	        GA_Height, 10,
	        NA_Buttons, (ULONG) data->ag_Buttons,
	        NA_Target, (ULONG) obj,
	        ICA_MAP, (ULONG) navmap,
	        ICA_TARGET, (ULONG) obj,
	        TAG_DONE);
}

/****** amigaguide.datatype/OM_NEW *******************************************

    NAME
	OM_NEW -- create an amigaguide datatype object

    INPUT
        struct opSet
        {
    	    ULONG              MethodID;
    	    struct TagItem    *ops_AttrList;
    	    struct GadgetInfo *ops_GInfo;
        };

    FUNCTION
        This method creates and initializes an amigaguide datatype object.
        Currently only DTST_FILE source type is supported. All tags for
        OM_SET method can be passed to the method. Additionally the following
        tags are only used during initialization.

    TAGS
        DTA_NodeName -- (STRPTR) name of the node to load initially.

        DTA_ARexxPortName -- (STRPTR) base name for the ARexx port.

    SEE ALSO
        datatypes.library/NewDTObjectA(), OM_DISPOSE, OM_SET, --arexx--

******************************************************************************
*
*/


ULONG AmigaGuide__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
   ULONG rv;
   CLASSBASE;
   INSTDATA;
   Object *newobj;

   STRPTR nodename = "main";
   ULONG sourcetype;
   BPTR handle;

   struct TagItem *tstate = msg->ops_AttrList;
   struct TagItem *tag;

   rv = DoSuperMethodA(cl, obj, msg);
   if (rv == 0)
	return rv;
    else
    {
	obj = (Object *)rv;
        data = (struct AmigaGuideData *)INST_DATA(cl, obj);
    }

   /* initialize the instance data */
   data->ag_Pool = CreatePool(MEMF_CLEAR | MEMF_ANY, AG_PUDDLE_SIZE, AG_PUDDLE_SIZE);

   /* prepare lists */
   NewList(&data->ag_Files);
   NewList(&data->ag_Visited);

   InitSemaphore(&data->ag_ASyncLayout);

   data->ag_ARexxPortName = "AMIGAGUIDE";

   /* lets use this object for the actual one until one node
      could be loaded. Therefore we don't need to check some
      conditions if a node is loaded or not. */
   data->ag_Actual = obj;
   data->ag_InitialNode = CopyAGString(cl, obj, nodename);
   data->ag_Creator = FindTask(NULL);

   /* process attributes that only belongs to this class
      and intialization phase. */
   while((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
   {
      switch(tag->ti_Tag)
      {
      case DTA_NodeName:
	 nodename = (STRPTR) tag->ti_Data;
	 break;
      case DTA_ARexxPortName:
         data->ag_ARexxPortName = (STRPTR) tag->ti_Data;
         break;
      }
   }

   data->ag_File = AllocAGFile(cl, obj);
   if(data->ag_File == NULL)
      rv = (ULONG) NULL;
   else
   {
      /* set attributes provided by the user */
      om_set(cl, obj, msg);

      /* now get the source type only DTST_FILE is supported.
         Then start scanning the AmigaGuide file */
      if(GetDTAttrs(obj,
   		    DTA_SourceType, (ULONG) &sourcetype,
   		    DTA_Handle, (ULONG) &handle,
   		    TAG_DONE) != 2)
      {
         rv = (ULONG) NULL;
      } else
      {
         switch(sourcetype)
         {
	     char s[1024];
         case DTST_FILE:
   	    data->ag_File->agf_Handle = handle;
            data->ag_File->agf_Lock = DupLockFromFH(handle);
	     NameFromFH(handle, s, 1024);
	     DB(("Scanning file %s\n", s));
   	    ScanFile(cl, obj, data->ag_File);

            if(data->ag_File->agf_OnOpen != NULL)
               if(SendRexxCommand(cl, obj, data->ag_File->agf_OnOpen, AGRX_RX) != RC_OK)
                  rv = (ULONG) NULL;
   	    break;
         default:
            DB(("sourcetype unsupported : %ld\n",sourcetype));
   	    rv = (ULONG) NULL;
         }
      }

      /* now create navigation bar */
      if(rv != (ULONG) NULL)
      {
         struct IBox domain;

         if(GetDTDomain(cl, obj, &domain))
         {
   	    data->ag_Buttons = AllocAGVec(cl, obj, sizeof(navbuttons));
   	    if(data->ag_Buttons != NULL)
   	    {
   	       memcpy(data->ag_Buttons, navbuttons, sizeof(navbuttons));

               MakeNavGadget(cl, obj, &domain);
   	    }
         }

         /* create external process for this object */
         if(CreateAGProcess(cl, obj) != data->ag_Process)
            rv = (ULONG) NULL;
      }
   }

   if (rv == (ULONG)NULL)
	CoerceMethod(cl, obj, OM_DISPOSE);

   return rv;
}


/****** amigaguide.datatype/OM_DISPOSE ***************************************

    NAME
	OM_DISPOSE -- dispose an amigaguide datatype object

    INPUT
        none

    FUNCTION
        This method disposes an amigaguide datatype object. It deallocates
        any resource obtained by the amigaguide object.

    SEE ALSO
        datatypes.library/DisposeDTObject(), OM_NEW

******************************************************************************
*
*/


ULONG AmigaGuide__OM_DISPOSE(Class *cl,Object *obj,Msg msg)
{
   CLASSBASE;
   INSTDATA;
   ULONG rv;
   struct AmigaGuideObject *ago;
   struct AmigaGuideFile *agf;

   DeleteAGProcess(cl, obj);

   /* close and unlock files, memory will be free's by DeletePool() */
   while((agf = (struct AmigaGuideFile *) RemHead(&data->ag_Files)) != NULL)
   {
      if(agf->agf_Handle != NULL)
         if(agf->agf_Flags.CloseHandle)
            Close(agf->agf_Handle);

      if(agf->agf_Lock != NULL)
         UnLock(agf->agf_Lock);
   }

   while((ago = (struct AmigaGuideObject *) RemHead(&data->ag_Visited)) != NULL)
   {
      FreeAGObject(cl, obj, ago);
   }

   /* cleanup navigator gadget */
   if(data->ag_Gadget != NULL)
      DisposeObject(data->ag_Gadget);

   /* cleanup our data here */
   if(data->ag_Pool != NULL)
      DeletePool(data->ag_Pool);

   rv = DoSuperMethodA(cl,obj,msg);

   return rv;
}

/****** amigaguide.datatype/OM_GET *******************************************

    NAME
	OM_GET -- gets an amigaguide datatype object attribute

    INPUT
        struct opGet
        {
    	    ULONG  MethodID;
    	    ULONG  opg_AttrID;
    	    ULONG *opg_Storage;
        };

    FUNCTION
        This method gets an amigaguide datatype object attribute. See the
        following TAGS section for directly supported (overwritten) attributes
        by the datatype. All attributes of super classes are supported as
        well.

    TAGS
        DTA_Title -- (STRPTR) name of the amigaguide document.

        DTA_NominalHoriz -- (ULONG) nominal width of the document in pixel.
            See @width AmigaGuide command.

        DTA_NominalVert -- (ULONG) nominal height of the document in pixel.
            See @height AmigaGuide command.

        DTA_Methods -- (ULONG *) array of supported datatype methods.

    SEE ALSO
        datatypes.library/GetDTAttrsA(), OM_SET

******************************************************************************
*
*/


ULONG AmigaGuide__OM_GET(Class *cl,Object *obj,struct opGet *msg)
{
   ULONG rv;
   INSTDATA;

   rv = 1;
   switch(msg->opg_AttrID)
   {
   case DTA_Title:
      *msg->opg_Storage = (ULONG) data->ag_File->agf_Name;
      break;
   case DTA_NominalHoriz:
      {
         WORD x=0,y=0;
         GetFontDimension(cl, obj, data->ag_File->agf_Font, &x, &y);
         *msg->opg_Storage = data->ag_File->agf_Width*x;
      }
      break;
   case DTA_NominalVert:
      {
         WORD x=0,y=0;
         GetFontDimension(cl, obj, data->ag_File->agf_Font, &x, &y);
         *msg->opg_Storage = data->ag_File->agf_Height*y;
      }
      break;
   case DTA_ARexxPortName:
      *msg->opg_Storage = (ULONG) data->ag_RexxName;
      break;
   case DTA_TriggerMethods:
      *msg->opg_Storage = (ULONG) triggermethods;
      break;

   case AGNA_Contents:
      if(data->ag_File->agf_TOC != NULL)
	 *msg->opg_Storage = (ULONG) data->ag_File->agf_TOC;
      else
	 *msg->opg_Storage = (ULONG) "main";
      break;
   case AGNA_Help:
      if(data->ag_File->agf_Help != NULL)
	 *msg->opg_Storage = (ULONG) data->ag_File->agf_Help;
      else
	 *msg->opg_Storage = (ULONG) "sys/amigaguide.guide";
      break;
   case AGNA_Index:
      *msg->opg_Storage = (ULONG) data->ag_File->agf_Index;
      break;
   case DTA_Methods:
      if(data->ag_Actual == obj)
         *msg->opg_Storage = (ULONG) supported_methods;
      else
         /* get methods from sub object. */
         if(DoMethodA(data->ag_Actual, (Msg) msg) == 0)
            rv = 0;
      break;
   default:
      rv = DoSuperMethodA(cl,obj,(Msg) msg);
   }

   return rv;
}

static
ULONG om_update(Class *cl, Object *obj, struct opSet *msg)
{
   CLASSBASE;
   ULONG rv = 0;

   struct TagItem *tstate = msg->ops_AttrList;
   struct TagItem *tag;

   /* process attributes that only belongs to this class */
   while((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
   {
      switch(tag->ti_Tag)
      {
      case AGNA_Command:
	 DoTrigger(cl, obj, msg->ops_GInfo, tag->ti_Data, NULL);
	 break;
      }
   }

   return rv;
}

/****** amigaguide.datatype/OM_SET *******************************************

    NAME
	OM_SET -- sets an amigaguide datatype object attribute

    INPUT
        struct opSet
        {
    	    ULONG              MethodID;
    	    struct TagItem    *ops_AttrList;
    	    struct GadgetInfo *ops_GInfo;
        };

    FUNCTION
        This method sets the passed amigaguide datatype object attributes. See
        the following TAGS section for directly supported (overwritten)
        attributes by the datatype. All attributes of super classes are
        supported as well.

    TAGS
        AGA_Secure, AGDTA_Secure -- (BOOL) if TRUE no program or ARexx script
            will be launched by the amigaguide datatype. Defaults to FALSE.

    SEE ALSO
        datatypes.library/SetDTAttrsA(), OM_GET

******************************************************************************
*
*/


static
ULONG om_set(Class *cl,Object *obj,struct opSet *msg)
{
   CLASSBASE;
   INSTDATA;
   ULONG rv = 0;

   struct TagItem *tstate = msg->ops_AttrList;
   struct TagItem *tag;

   /* process attributes that only belongs to this class */
   while((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
   {
      switch(tag->ti_Tag)
      {
      case AGDTA_Secure:
      case AGA_Secure:
         data->ag_Flags.Secure = tag->ti_Data ? TRUE : FALSE;
         break;
      case AGDTA_HelpGroup:
         /* TODO: help group set attr? */
         break;
      case DTA_Sync:
	 /* now redraw the object */
	 rv = 1;
	 break;
      case ICA_TARGET:
         data->ag_ICTarget = (Object *) tag->ti_Data;
         break;
      case ICA_MAP:
         data->ag_ICMap = (struct TagItem *) tag->ti_Data;
         break;
      }

      DA(msg->MethodID == OM_SET, ("OM_SET: %lx=0x%lx\n", tag->ti_Tag, tag->ti_Data));
      DA(msg->MethodID == OM_UPDATE, ("OM_UPDATE: %lx=0x%lx\n", tag->ti_Tag, tag->ti_Data));
   }

   if(data->ag_Flags.Redraw)
      rv++;

   if(data->ag_Actual != obj)
   {
      rv += DoMethodA(data->ag_Actual, (Msg) msg);
   }
   return rv;
}



ULONG AmigaGuide__GM_LAYOUT(Class *cl, Object *obj, struct gpLayout *msg)
{
   CLASSBASE;
   INSTDATA;
   ULONG rc;

   /* remember window pointer to be used by ARexx commands */
   if(data->ag_Window == NULL)
      data->ag_Window = msg->gpl_GInfo->gi_Window;

   rc = DoSuperMethodA(cl,obj,(Msg) msg);

   DB(("inital layout %ld\n", msg->gpl_Initial));

   if(data->ag_Gadget == NULL)
   {
      struct IBox domain;
      if(GetDTDomain(cl, obj, &domain))
         MakeNavGadget(cl, obj, &domain);
   }

   if(data->ag_Gadget != NULL)
      DoMethodA(CAST_OBJ(data->ag_Gadget),(Msg) msg);

   if(GetDTDomain(cl, obj, &data->ag_SubObjDomain))
   {
      if(data->ag_Gadget != NULL)
	 data->ag_NavHeight = CAST_GAD(data->ag_Gadget)->Height + AG_NAV_DISTANCE;

      data->ag_SubObjDomain.Top    += data->ag_NavHeight;
      data->ag_SubObjDomain.Height -= data->ag_NavHeight;
   }

   if(data->ag_Actual != obj)
   {
      rc += DoMethodA(data->ag_Actual, (Msg) msg);
      /* adjust initial layout domain attributes */
      if(msg->gpl_Initial)
      {
         SetAttrs(data->ag_Actual,
                  GA_Top, CAST_GAD(obj)->TopEdge + data->ag_NavHeight,
                  GA_RelHeight, CAST_GAD(obj)->Height - data->ag_NavHeight,
                  TAG_DONE);
      }
   }

   if(!data->ag_Flags.InAsyncLayout)
   {
      /* if we aren't in our external process send layout msg. */
      if(data->ag_Process != (struct Process *) FindTask(NULL))
      {
         struct DTSpecialInfo *si= CAST_GAD(obj)->SpecialInfo;

         ObtainSemaphore(&data->ag_ASyncLayout);
         if(si->si_Flags & DTSIF_LAYOUT)
         {
            si->si_Flags |= DTSIF_NEWSIZE;
            /* terminate layout process */
            Signal(&data->ag_Process->pr_Task, SIGBREAKF_CTRL_C);
         } else
         {
            if(SendAGLayout(cl, obj, msg))
               si->si_Flags |= DTSIF_LAYOUT;
         }
         ReleaseSemaphore(&data->ag_ASyncLayout);
      } else
      {
         struct gpLayout layout = *msg;

         layout.MethodID = DTM_ASYNCLAYOUT;
         DoMethodA(obj, (Msg) &layout);

         if(data->ag_Flags.GotoLine)
         {
            /* set possible new vertical top position (goto line). */
            SetAttrs(obj, DTA_TopVert, data->ag_ActualObject->ago_TopVert, TAG_DONE);
            /* and also tell others... */
            NotifyAttrs(obj, msg->gpl_GInfo, 0,
   	                GA_ID,       CAST_GAD(obj)->GadgetID,
                        DTA_TopVert, data->ag_ActualObject->ago_TopVert,
                        TAG_DONE);
            data->ag_Flags.GotoLine = FALSE;
         }
      }
   }

   return rc;
}


ULONG AmigaGuide__GM_RENDER(Class *cl, Object *obj, struct gpRender *msg)
{
   CLASSBASE;
   INSTDATA;
   struct DTSpecialInfo *si= CAST_GAD(obj)->SpecialInfo;
   ULONG rv = 0;

   /* do not render in busy state */
   if(si->si_Flags & DTSIF_LAYOUT)
   {
      DB(("gm_render(): in layout\n"));
      data->ag_Flags.Redraw = TRUE;
      return rv;
   }

   if(!AttemptSemaphore(&si->si_Lock))
   {
      DB(("gm_render(): lock failed\n"));
   } else
   {
      /* first draw navigator gadget */
      Object *nav = CAST_OBJ(data->ag_Gadget);
      if(nav != NULL)
      {
	 ULONG drawmode = msg->gpr_Redraw;
	 ULONG draw = data->ag_Flags.Redraw || drawmode == GREDRAW_REDRAW;

	 if(drawmode == GREDRAW_UPDATE)
	    if(DoMethod(nav, NVM_CHANGED))
	       draw = TRUE;

	 if(draw)
	 {
	    struct IBox *domain = &data->ag_SubObjDomain;
	    struct RastPort *rp = msg->gpr_RPort;
	    struct GadgetInfo *ginfo = msg->gpr_GInfo;
	    WORD y = domain->Top - AG_NAV_DISTANCE;
            struct gpRender render;

            render.MethodID  = GM_RENDER;
            render.gpr_GInfo = ginfo;
            render.gpr_RPort = rp;
            render.gpr_Redraw= drawmode;

	    DoMethodA(CAST_OBJ(nav), (Msg) &render);

	    SetAPen(rp, ginfo->gi_DrInfo->dri_Pens[SHINEPEN]);
	    Move(rp, domain->Left                    , y);
	    Draw(rp, domain->Left + domain->Width - 1, y);
	    SetAPen(rp, ginfo->gi_DrInfo->dri_Pens[SHADOWPEN]);
	    Move(rp, domain->Left                    , ++y);
	    Draw(rp, domain->Left + domain->Width - 1, y);
	 }
      }

      /* now lets draw the actual displayed object */
      if(data->ag_Actual != obj)
      {
	 rv = DoMethodA(data->ag_Actual, (Msg) msg);
      }

      rv = 1;

      data->ag_Flags.Redraw = FALSE;
      ReleaseSemaphore(&si->si_Lock);
   }

   return rv;
}

/****** amigaguide.datatype/GM_HANDLEINPUT ***********************************

    NAME
	GM_HANDLEINPUT -- handles input events

    INPUT
        struct gpInput
        {
        	ULONG              MethodID;
        	struct GadgetInfo *gpi_GInfo;
        	struct InputEvent *gpi_IEvent;
        	LONG              *gpi_Termination;
        	struct
        	{
        		WORD X;
        		WORD Y;
        	}                  gpi_Mouse;
        	struct TabletData *gpi_TabletData;
        };

    FUNCTION
        This method handles incoming input events. Checks if the event resides
        in the navigation bar and executes any action if so. Otherwise it
        forwards the incoming events to its viewed node or datatype object.
        In case of an AmigaGuide node it handles any mouse event to enable
        link activation through the mouse.

    SEE ALSO

******************************************************************************
*
*/


ULONG AmigaGuide__GM_HANDLEINPUT(Class *cl, Object *obj, struct gpInput *msg)
{
   CLASSBASE;
   INSTDATA;
   struct DTSpecialInfo *si= CAST_GAD(obj)->SpecialInfo;
   ULONG rv = GMR_MEACTIVE;

   /* do not handle input in busy state */
   if(si->si_Flags & DTSIF_LAYOUT)
   {
      rv = GMR_NOREUSE;
   } else if(!AttemptSemaphore(&si->si_Lock))
   {
      rv = GMR_NOREUSE;
   } else
   {
      struct InputEvent *ie = msg->gpi_IEvent;
      struct Gadget *nav = (struct Gadget *) data->ag_Gadget;

      switch(ie->ie_Class)
      {
      case IECLASS_RAWMOUSE:
	 if(nav != NULL && ((msg->gpi_Mouse.Y < nav->Height &&
			     !data->ag_Flags.InDocInput) ||
			    data->ag_Flags.InNavInput))
	 {
	    /* check if gadget is even active. This is needed to deactivate the gadget, if
	     * the user makes a great step with the mouse, so that msg->gpi_Mouse.Y > nav->Height !
	     */
	    rv = DoMethodA(CAST_OBJ(nav),(Msg) msg);

	    data->ag_Flags.InNavInput = (rv == GMR_MEACTIVE) ? TRUE : FALSE;
	 } else if(data->ag_Actual != obj)
	 {
            /* adjust mouse y coordinate so the sub-object doesn't need to
             * know about the navigation bar height.
             */
            msg->gpi_Mouse.Y -= (nav->Height + AG_NAV_DISTANCE);

	    rv = DoMethodA(data->ag_Actual, (Msg) msg);

	    data->ag_Flags.InDocInput = (rv == GMR_MEACTIVE) ? TRUE : FALSE;
	 }
	 break;
      }
      ReleaseSemaphore(&si->si_Lock);
   }

   return rv;
}


ULONG AmigaGuide__DTM_REMOVEDTOBJECT(Class *cl, Object *obj, Msg msg)
{
   INSTDATA;

   /* clear window pointer which is used in the external process
      using DoDTMethod() to get a valid GInfo structure. */
   data->ag_Window = NULL;

#if 0
   CLASSBASE;
   if(data->ag_Gadget != NULL)
   {
      DisposeObject(data->ag_Gadget);
      data->ag_Gadget = NULL;
   }
#endif

   return DoSuperMethodA(cl, obj, msg);
}


ULONG AmigaGuide__DTM_ASYNCLAYOUT(Class *cl, Object *obj, struct gpLayout *msg)
{
   CLASSBASE;
   INSTDATA;
   struct DTSpecialInfo *si = (struct DTSpecialInfo *) CAST_GAD(obj)->SpecialInfo;
   struct DTSpecialInfo *sosi = (struct DTSpecialInfo *) CAST_GAD(data->ag_Actual)->SpecialInfo;
   struct IBox domain;
   LONG topv,totv,unitv;
   LONG toph,toth,unith;
   STRPTR title = "amigaguide.datatype";

   GetDTDomain(cl, obj, &domain);

   ObtainSemaphore(&si->si_Lock);

   /* indicate sub object that we are in layout */
   sosi->si_Flags |= DTSIF_LAYOUT;

   /* try to load initial node */
   if(data->ag_Actual == obj)
   {
      data->ag_Flags.InAsyncLayout = TRUE;
      GotoObject(cl, obj, msg->gpl_GInfo, data->ag_InitialNode, 0);
      data->ag_Flags.InAsyncLayout = FALSE;
   }

   /* layout the actual sub object. */
   if(data->ag_Actual != obj)
   {
      BOOL initial = msg->gpl_Initial;
      ULONG mid = msg->MethodID;

      /* if ag_Flags.InitialLayout is set to TRUE, the sub-object
       * needs an initial layout! */
      if(data->ag_Flags.InitialLayout)
      {
 	  msg->gpl_Initial = TRUE;
	 data->ag_Flags.InitialLayout = FALSE;
      }

      /* we are in asynclayout, so make really sure, overwriting proclayout id! */
      msg->MethodID = DTM_ASYNCLAYOUT;
      DoMethodA(data->ag_Actual, (Msg) msg);
      msg->MethodID = mid;

      if(!initial)
	 msg->gpl_Initial = FALSE;
   }

   if(GetDTAttrs(data->ag_Actual,
		 DTA_TopVert,    (ULONG) &topv,
		 DTA_TotalVert,  (ULONG) &totv,
		 DTA_TopHoriz,   (ULONG) &toph,
		 DTA_TotalHoriz, (ULONG) &toth,
		 DTA_VertUnit,   (ULONG) &unitv,
		 DTA_HorizUnit,  (ULONG) &unith,
		 TAG_DONE) == 6)
   {
      unitv = (unitv == 0) ? 1 : unitv;
      unith = (unith == 0) ? 1 : unith;

      if(GetDTAttrs(data->ag_Actual, DTA_Title, (ULONG) &title, TAG_DONE) == 0)
         if(GetDTAttrs(data->ag_Actual, DTA_Name, (ULONG) &title, TAG_DONE) == 0)
            title = "unknown";
      /* these values must be set explicitly
       * possible error in datatypesclass ?
       */
      si->si_VisVert  = (domain.Height - data->ag_NavHeight) / unitv;
      si->si_VisHoriz = domain.Width  / unith;

      DB(("topvert %ld, visvert %ld, totvert %ld\n",
          topv, si->si_VisVert, totv));
      if(msg->gpl_Initial)
      {
	 /* fix for a different version of the datatypesclass, normally
	  * this should be done by the datatypesclass itself */
	 si->si_TopVert  = -1;
	 si->si_TopHoriz = -1;
      } else
      {
#if 1
	 /* TODO: if wrapping is on this value maybe wrong after an layout */
	 /* remember last vertical top value */
	 si->si_TopVert = topv;
         si->si_TopHoriz = toph;
#endif
      }

#if 1
      si->si_TotVert  = totv;
      si->si_TotHoriz = toth;
#endif

      DB(("before async notify...\n"));

      NotifyAttrs(obj, msg->gpl_GInfo, 0,
		       GA_ID,          CAST_GAD(obj)->GadgetID,

		       DTA_TopVert,    topv,
		       DTA_TotalVert,  totv,
		       DTA_VertUnit,   unitv,
		       DTA_VisibleVert,si->si_VisVert,

		       DTA_TopHoriz,   toph,
		       DTA_TotalHoriz, toth,
		       DTA_HorizUnit,  unith,
		       DTA_VisibleHoriz,si->si_VisHoriz,

		       DTA_Title,      title,

		       DTA_Busy,       FALSE,
		       DTA_Sync,       TRUE,
		       TAG_DONE);
   }

   /* layout done... */
   sosi->si_Flags &= ~DTSIF_LAYOUT;

   ReleaseSemaphore(&si->si_Lock);

   return totv;
}

/****** amigaguide.datatype/DTM_TRIGGER **************************************

    NAME
	DTM_TRIGGER -- triggers some functions on the AmigaGuide object.

    INPUT
        struct dtTrigger
        {
        	ULONG                MethodID;
        	struct GadgetInfo   *dtt_GInfo;
        	ULONG                dtt_Function;
        	APTR                 dtt_Data;
        };

    FUNCTION
        This method triggers the following functions specified via the
        dtt_Function field of the message. NOTE that you have to mask out
        the real STM_#? function from the field by applying the function
        mask (dtt_Function & STMG_METHOD_MASK):

        STM_RETRACE -- Go to the previous viewed node. Same as pressing the
            "Retrace" button

        STM_HELP -- Go to the help node. Same as pressing the "Help" button

        STM_INDEX -- Go to the index node. Same as pressing the "Index" button

        STM_CONTENTS -- Go to the contents/main node. Same as pressing the
            "Contents" button

        STM_BROWSE_PREV -- Go to the previous logical node in the database.
            Same as pressing the "< Browse" button

        STM_BROWSE_NEXT -- Go to the next logical node in the database. Same
            as pressing the "> Browse" button

        STM_COMMAND -- Execute the command found in the dtt_Data field.
            Currently only null-terminated strings are supported. To make this
            function save it only operates on data found in dtt_Data if
            (dtt_Function & STMF_DATA_MASK) == STMD_STRPTR. The following
            commands are supported:

            link <nodename> -- Goto to the named node.
            system <command> -- Execute the given command.
            rxs <arexx-string> -- Executes the given ARexx string.
            rx <arexx-script> -- Executes the given ARexx macro script.

        STM_PREV_FIELD -- if the current node has links select previous link.

        STM_NEXT_FIELD -- if the current node has links select next link.

        STM_ACTIVATE_FIELD -- if a link is selected activate it, by sending
            a STM_COMMAND trigger method.

    SEE ALSO
        DTM_GOTO

******************************************************************************
*
*/



ULONG AmigaGuide__DTM_TRIGGER(Class *cl, Object *obj, struct dtTrigger *msg)
{
   CLASSBASE;
   INSTDATA;
   ULONG rv = 0;

   /* make sure any trigger command is only executed in our external
    * process context!
    */
   if(data->ag_Process != (struct Process *) FindTask(NULL))
   {
      SendAGTrigger(cl, obj, msg);
      return 0;
   }

   DB(("do trigger command (proc=\"%s\") function=%lx\n",
       FindTask(NULL)->tc_Node.ln_Name, msg->dtt_Function));

   switch(msg->dtt_Function & STMF_METHOD_MASK)
   {
   case STM_RETRACE:
      if(data->ag_ActualObject->ago_Node.ln_Succ->ln_Succ != NULL)
      {
	 struct AmigaGuideObject *old = (struct AmigaGuideObject *) RemHead(&data->ag_Visited);
	 DB(("old is %lx (%s)\n", old, old->ago_Node.ln_Name));
         data->ag_Flags.GotoLine = TRUE;
	 ActivateAGObject(cl, obj, msg->dtt_GInfo,
			  (struct AmigaGuideObject *) data->ag_Visited.lh_Head);
	 FreeAGObject(cl, obj, old);
      }
      break;
   case STM_COMMAND:
      {
         ULONG cmdtype = CMDTYPE_UNKNOWN;
         STRPTR dest = "";
         STRPTR destend = NULL;
         LONG line = 0;
         UBYTE chr = '"';

         DB(("got trigger command\n"));
         switch(msg->dtt_Function & STMF_DATA_MASK)
         {
         case STMD_STRPTR: {
            STRPTR type, typeend;
            STRPTR ptr = (STRPTR) msg->dtt_Data;

            DB(("string is %s\n", ptr));
            type = ptr;
            while(*ptr != '\0' && *ptr != ' ' && *ptr != '\t')
               ++ptr;
            typeend = ptr;
            ptr = eatws(ptr);
            if(*ptr != '"')
            {
               dest = ptr;
               while(*ptr != '\0' && *ptr != ' ' && *ptr != '\t')
                  ++ptr;
               destend = ptr;
               chr = *destend;
               ptr = eatws(ptr);
               /* get line number to goto. */
               if(*ptr != '\0')
                  StrToLong(ptr, &line);
               *destend = '\0';
            } else
            {
               dest = ++ptr;
               while(*ptr != '\0' && *ptr != '"')
                  ptr++;
               destend = ptr;
               chr = *destend;
               if(*ptr == '"')
               {
                  ptr = eatws(ptr+1);
                  /* get line number to goto. */
                  if(*ptr != '\0')
                     StrToLong(ptr, &line);
               }
               *destend = '\0';
            }

            if(!Strnicmp(type, "link", typeend-type))
               cmdtype = CMDTYPE_LINK;
            else if(!Strnicmp(type, "system", typeend-type))
               cmdtype = CMDTYPE_SYSTEM;
            else if(!Strnicmp(type, "rxs", typeend-type) && (typeend-type) == 3)
               cmdtype = CMDTYPE_RXS;
            else if(!Strnicmp(type, "rx", typeend-type))
               cmdtype = CMDTYPE_RX;
            } break;
         }

         DB(("cmdtype %ld (dest=\"%s\") %ld\n", cmdtype, dest, line));
         if(*dest != '\0')
            switch(cmdtype)
            {
            case CMDTYPE_LINK:
               rv = GotoObject(cl, obj, msg->dtt_GInfo, dest, line);
               break;
            case CMDTYPE_SYSTEM:
               SystemCommand(cl, obj, dest);
               break;
            case CMDTYPE_RXS:
               SendRexxCommand(cl, obj, dest, AGRX_RXS);
               break;
            case CMDTYPE_RX:
               SendRexxCommand(cl, obj, dest, AGRX_RX);
               break;
            default:
               DB(("unknown command\n"));
            }
         if(destend != NULL)
            *destend = chr;
      }
      break;
   case STM_HELP:
      rv = GotoObjectTag(cl, obj, msg->dtt_GInfo, AGNA_Help);
      break;
   case STM_CONTENTS:
      rv = GotoObjectTag(cl, obj, msg->dtt_GInfo, AGNA_Contents);
      break;
   case STM_INDEX:
      rv = GotoObjectTag(cl, obj, msg->dtt_GInfo, AGNA_Index);
      break;
   case STM_BROWSE_PREV:
      rv = GotoObjectTag(cl, obj, msg->dtt_GInfo, AGNA_Previous);
      /* no object found try previous node in linked list */
      if(rv == 0)
      {
	 if(data->ag_ActualObject->ago_AGNode->agn_Node.ln_Pred->ln_Pred != NULL)
	    rv = GotoObject(cl, obj, msg->dtt_GInfo, data->ag_ActualObject->ago_AGNode->agn_Node.ln_Pred->ln_Name, 0);
      }
      break;
   case STM_BROWSE_NEXT:
      rv = GotoObjectTag(cl, obj, msg->dtt_GInfo, AGNA_Next);
      /* no object found try next node in linked list */
      if(rv == 0)
      {
	 if(data->ag_ActualObject->ago_AGNode->agn_Node.ln_Succ->ln_Succ != NULL)
	    rv = GotoObject(cl, obj, msg->dtt_GInfo, data->ag_ActualObject->ago_AGNode->agn_Node.ln_Succ->ln_Name, 0);
      }
      break;
   default:
      rv = DoMethodA(data->ag_Actual, (Msg) msg);
   }

   return rv;
}

static
void UpdateNavigator(Class *cl, Object *obj, struct GadgetInfo *ginfo)
{
   CLASSBASE;
   INSTDATA;
   struct AmigaGuideObject *agobj = data->ag_ActualObject;

   struct npChangeStatus chg;
   struct NavigatorStatus cmds[6];
   STRPTR toc = NULL;
   STRPTR index = NULL;
   STRPTR prev = NULL;
   STRPTR next = NULL;
   STRPTR help = NULL;

   GetDTAttrs(data->ag_Actual,
	      AGNA_Contents, (ULONG) &toc,
	      AGNA_Index,    (ULONG) &index,
	      AGNA_Previous, (ULONG) &prev,
	      AGNA_Next,     (ULONG) &next,
	      AGNA_Help,     (ULONG) &help,
	      TAG_DONE);

   if(help == NULL)
   {
      BOOL nodetype;
      BPTR lock = GetFileLock(cl, obj, "sys/amigaguide.guide", &nodetype);
      if(lock != NULL)
      {
         UnLock(lock);
         help = "sys/amigaguide.guide";
         SetAttrs(data->ag_Actual,
                  AGNA_Help, (ULONG) help,
                  TAG_DONE);
      }
   }
   cmds[0].ns_Command = STM_CONTENTS;
   cmds[0].ns_Status  = (toc == NULL && !Stricmp(agobj->ago_Node.ln_Name, "main")) ? NVS_DISABLE : NVS_ENABLE;
   cmds[1].ns_Command = STM_INDEX;
   cmds[1].ns_Status  = (index == NULL || !Stricmp(agobj->ago_Node.ln_Name, index)) ? NVS_DISABLE : NVS_ENABLE;
   cmds[2].ns_Command = STM_RETRACE;
   cmds[2].ns_Status  = (agobj->ago_Node.ln_Succ->ln_Succ == NULL) ? NVS_DISABLE : NVS_ENABLE;
   cmds[3].ns_Command = STM_BROWSE_PREV;
   cmds[3].ns_Status  = (prev == NULL && (agobj->ago_AGNode == NULL || agobj->ago_AGNode->agn_Node.ln_Pred->ln_Pred == NULL)) ? NVS_DISABLE : NVS_ENABLE;
   cmds[4].ns_Command = STM_BROWSE_NEXT;
   cmds[4].ns_Status  = (next == NULL && (agobj->ago_AGNode == NULL || agobj->ago_AGNode->agn_Node.ln_Succ->ln_Succ == NULL)) ? NVS_DISABLE : NVS_ENABLE;
   cmds[5].ns_Command = STM_HELP;
   cmds[5].ns_Status  = (help == NULL) ? NVS_DISABLE : NVS_ENABLE;

   chg.MethodID = NVM_CHANGESTATUS;
   chg.np_GInfo = ginfo;
   chg.np_NumCommands = 6;
   chg.np_Commands = cmds;

   if(DoMethodA(CAST_OBJ(data->ag_Gadget), (Msg) &chg))
   {
      NotifyAttrs(obj, ginfo, 0,
		  GA_ID,    CAST_GAD(obj)->GadgetID,
		  DTA_Sync, TRUE,
		  TAG_DONE);
   }
}

static
void ActivateAGObject(Class *cl, Object *obj, struct GadgetInfo *ginfo,
		      struct AmigaGuideObject *agobj)
{
   CLASSBASE;
   INSTDATA;
   struct gpLayout layout;
   struct IBox *domain;

   data->ag_Actual = agobj->ago_Object;
   data->ag_ActualObject = agobj;

   /* if the current object is an amigaguide node set the appropriate
      file structure into the global data section to find nodes within
      this AmigaGuide file again. */
   if(agobj->ago_AGNode != NULL)
      data->ag_File = agobj->ago_AGNode->agn_File;

   /* adjust state of navigation bar */
   UpdateNavigator(cl, obj, ginfo);

   /* clear object domain */
   {
      struct RastPort *rp;

      domain = &data->ag_SubObjDomain;
      rp = ObtainGIRPort(ginfo);
      if(rp != NULL)
      {
         EraseRect(rp, domain->Left, domain->Top,
                       domain->Left+domain->Width-1,
                       domain->Top+domain->Height-1);
         ReleaseGIRPort(rp);
      }
   }

   /* now lets layout our and its sub object. */
   layout.MethodID    = GM_LAYOUT;
   layout.gpl_GInfo   = ginfo;
   layout.gpl_Initial = FALSE;
   DoMethodA(obj, (Msg) &layout);
}


/****** amigaguide.datatype/DTM_GOTO *****************************************

    NAME
	DTM_GOTO -- goto the given node/object.

    INPUT
        struct dtGoto
        {
        	ULONG                MethodID;
        	struct GadgetInfo   *dtg_GInfo;
        	STRPTR               dtg_NodeName;
        	struct TagItem      *dtg_AttrList;
        };

    FUNCTION
        This method tries to load and view the node or object specified in the
        dtg_NodeName field. This field can also contain a file name for a file
        to be viewed within the AmigaGuide object. And this also may have an
        additionally added node name. For example if you want to open an
        external AmigaGuide file called index.guide and you want to open the
        node named "B" for index entries starting with the letter "B" you can
        just pass index.guide/B to the method via the dtg_NodeName field. Also
        the tags listed in the TAGS section are passed to the newly created
        datatype object.

    TAGS
        DTA_TopVert -- vertical (line) position to go to.

    RETURN
        1 -- if object could be loaded
        0 -- if object could not be found
        -1 -- if some error occured during loading

    SEE ALSO
        DTM_TRIGGER

******************************************************************************
*
*/



ULONG AmigaGuide__DTM_GOTO(Class *cl, Object *obj, struct dtGoto *msg)
{
   CLASSBASE;
   INSTDATA;
   ULONG rv = 0;
   struct DataType *dt;
   struct AmigaGuideNode *agnode;
   struct AmigaGuideObject *agobj = NULL;
   Object *dtobj = NULL;
   LONG err = 0;

   UBYTE fontname[MAXFONTNAME];
   struct TextAttr ta;
   struct TextAttr *textattr = NULL;
   BOOL aginternal = TRUE;

   if(msg->dtg_NodeName == NULL || *msg->dtg_NodeName == '\0')
   {
      NotifyAttrs(obj, msg->dtg_GInfo, 0,
		  GA_ID, CAST_GAD(obj)->GadgetID,
		  DTA_Busy, FALSE,
		  DTA_ErrorString, "goto empty node name",
		  DTA_ErrorNumber, ERROR_REQUIRED_ARG_MISSING,
		  DTA_ErrorLevel, 1,
		  TAG_DONE);
      return -1;
   }

   if(data->ag_File->agf_Font != NULL)
   {
      ta.ta_Name = fontname;
      DB(("parse font %s\n", data->ag_File->agf_Font));
      ParseFontLine(cl, obj, data->ag_File->agf_Font, &ta);
      textattr = &ta;
   }

   DB(("try loading object \"%s\"\n", msg->dtg_NodeName));

   /* currently protected input.device task */
   if(FindTask(NULL)->tc_Node.ln_Type != NT_PROCESS)
   {
      DB(("goto object \"%s\" within input task\n", msg->dtg_NodeName));
      return 1;
   }

   /* TODO: check this work around...
    * don't notify loading title if its the main and initial node. There
    * are some asynchron notification problems. Sometimes this notification
    * is send to MultiView after the complete async layout method has
    * completed including all notifications (DTA_Busy == FALSE). Which is
    * then overwritten by this notification...
    */
   if(!data->ag_Flags.InAsyncLayout)
   {
      mysprintf(cb, data->ag_Message, sizeof(data->ag_Message),
                "Loading %s ...", msg->dtg_NodeName);

      NotifyAttrs(obj, msg->dtg_GInfo, 0,
   	          GA_ID, CAST_GAD(obj)->GadgetID,
   	          DTA_Busy, TRUE,
   	          DTA_Title, data->ag_Message,
   	          TAG_DONE);
   }
   agnode = GetAGNode(cl, obj, data->ag_File, msg->dtg_NodeName);
   if(agnode == NULL)
   {
      STRPTR file = CopyAGString(cl, obj, msg->dtg_NodeName);
      aginternal = FALSE;
      if(file != NULL)
      {
         BOOL nodetype = FALSE;
         BPTR lock = GetFileLock(cl, obj, file, &nodetype);

         DB(("node \"%s\" not found (lock=%lx)\n", file, lock));

         if(lock != NULL)
         {
            if((dt = ObtainDataTypeA(DTST_FILE, (APTR) lock, NULL)) != NULL)
            {
               if(nodetype == TRUE && !Strnicmp(dt->dtn_Header->dth_Name, "AmigaGuide", 10))
               {
                  struct AmigaGuideFile *agf = (struct AmigaGuideFile *) data->ag_Files.lh_Head;

                  /* search current list of opened AG files */
                  while(agf->agf_Node.ln_Succ != NULL)
                  {
                     if(SameLock(agf->agf_Lock, lock) == LOCK_SAME)
                     {
                        /* found a previously opened AG file, so try to find node */
                        agnode = GetAGNode(cl, obj, agf, FilePart(msg->dtg_NodeName));
                        if(agnode != NULL)
                        {
                           /* set active AG file */
                           data->ag_File = agf;
                           break;
                        }
                     }
                     agf = (struct AmigaGuideFile *) agf->agf_Node.ln_Succ;
                  }

                  /* no ag file and node found so load it now */
                  if(agnode == NULL)
                  {
                     DB(("found amigaguide file\n"));
                     agf = AllocAGFile(cl, obj);
                     if(agf != NULL)
                     {
                        if((agf->agf_Handle = OpenFromLock(lock)) != NULL)
                        {
                           agf->agf_Lock = DupLockFromFH(agf->agf_Handle);
                           agf->agf_Flags.CloseHandle = TRUE;
                           lock = NULL;
                           ScanFile(cl, obj, agf);
                           data->ag_File = agf;
                           agnode = GetAGNode(cl, obj, data->ag_File, FilePart(msg->dtg_NodeName));
                        }
                     }
                  }
               } else
               {
                  STRPTR filename = AllocAGMem(cl, obj, 1024);
                  agobj = AllocAGObject(cl, obj);
                  if(agobj != NULL && filename != NULL)
                  {
                     NameFromLock(lock, filename, 1024);
                     DB(("try to open file \"%s\"\n", file));
                     DTL(msg->dtg_AttrList);
                     if(msg->dtg_AttrList != NULL)
                        agobj->ago_TopVert = GetTagData(DTA_TopVert, 0, msg->dtg_AttrList);
            	     agobj->ago_Object =
            	     dtobj = NewDTObject(filename,
                   	                GA_Immediate, TRUE,
                    		        GA_RelVerify, TRUE,
         	       		        GA_Top,           data->ag_SubObjDomain.Top,
         	          	        GA_Left,          data->ag_SubObjDomain.Left,
         		     	        GA_RelWidth,      CAST_GAD(obj)->Width,
         		                GA_RelHeight,     CAST_GAD(obj)->Height - data->ag_NavHeight,
         		                GA_ID,            CAST_GAD(obj)->GadgetID,
                                        ICA_TARGET,       (ULONG) data->ag_ICTarget,
                                        ICA_MAP,          (ULONG) data->ag_ICMap,
                                        DTA_TopVert,      agobj->ago_TopVert,
         			        (textattr != NULL) ? DTA_TextAttr : TAG_IGNORE, (ULONG) textattr,
         			        TAG_DONE);
                     data->ag_Flags.InitialLayout = TRUE;
                  } else
                  {
               	     rv = 0;
                  }

                  if(filename != NULL)
                     FreeAGMem(cl, obj, filename, 1024);
               }
               ReleaseDataType(dt);
            }
            UnLock(lock);
         }
         FreeAGVec(cl ,obj, file);
      }
   }

   if(agnode != NULL)
   {
      /* GEORGFIX: data->ag_ActualObject could be NULL here so added
         "data->ag_ActualObject" to checks (to make sure it is != NULL) */
	 
      if(data->ag_ActualObject && data->ag_ActualObject->ago_AGNode != NULL && aginternal &&
         !Stricmp(agnode->agn_Node.ln_Name, data->ag_ActualObject->ago_AGNode->agn_Node.ln_Name))
      {
         DB(("link within same node \"%s\"\n", agnode->agn_Node.ln_Name));
         agobj = AllocAGObject(cl, obj);
         if(agobj != NULL)
         {
            dtobj = agobj->ago_Object = data->ag_Actual;
            agobj->ago_AGNode = data->ag_ActualObject->ago_AGNode;
            agobj->ago_NoDispose = TRUE;
         }
      } else
      {
         agobj = AllocAGObjectNode(cl, obj, data->ag_File, agnode);
         if(agobj != NULL)
         {
   	    char buf[64];
            LONG i=0;
            do
            {
               mysprintf(cb, buf, sizeof(buf), "T:%s_%ld", FilePart(msg->dtg_NodeName), ++i);
               agobj->ago_TmpHandle = Open(buf, MODE_NEWFILE);
            } while(agobj->ago_TmpHandle == NULL && i<100);
            if(agobj->ago_TmpHandle != NULL)
            {
               Write(agobj->ago_TmpHandle, agobj->ago_Buffer, agobj->ago_BufferLen);
               Close(agobj->ago_TmpHandle);
               agobj->ago_TmpHandle = Lock(buf, SHARED_LOCK);
            }

            if(agnode->agn_Font != NULL)
            {
               ta.ta_Name = fontname;
               ParseFontLine(cl, obj, agnode->agn_Font, &ta);
               textattr = &ta;
            }

            dt = ObtainDataTypeA(DTST_FILE, (APTR) agobj->ago_TmpHandle, NULL);
#if 0
           /* TODO: Currently MorphOS datatypes.library doesn't support DTST_MEMORY... */
     			        DTA_SourceType,   DTST_MEMORY,
   			        DTA_SourceAddress,agobj->ago_Buffer,
   			        DTA_SourceSize,   agobj->ago_BufferLen,
   			        TAG_DONE);
#endif
            DA(dt == NULL, ("can't determine datatype\n"));

            if(dt != NULL)
            {
               agobj->ago_Object =
               dtobj = NewObject(cb->cb_NodeClass, NULL,
                                 GA_Top,           data->ag_SubObjDomain.Top,
   	                         GA_Left,          data->ag_SubObjDomain.Left,
   		                 GA_RelWidth,      CAST_GAD(obj)->Width,
   		                 GA_RelHeight,     CAST_GAD(obj)->Height - data->ag_NavHeight,
   		                 GA_ID,            CAST_GAD(obj)->GadgetID,
   		                 DTA_DataType,     (ULONG) dt,
   		                 DTA_Name,         (ULONG) buf,
   		                 DTA_SourceType,   DTST_FILE,
   		                 DTA_Handle,       (ULONG) agobj->ago_TmpHandle,
                                 ICA_TARGET,       (ULONG) data->ag_ICTarget,
                                 ICA_MAP,          (ULONG) data->ag_ICMap,
                                 /* special attributes for amigagudienode class */
                                 AGNA_RootObject,  (ULONG) obj,
                                 AGNA_AGFile,      (ULONG) data->ag_File,

   		                 (textattr != NULL) ? DTA_TextAttr : TAG_IGNORE, (ULONG) textattr,
   		                 (msg->dtg_AttrList != NULL) ? TAG_MORE : TAG_DONE, (ULONG) msg->dtg_AttrList);
               if(dtobj != NULL)
               {
                  agobj->ago_TmpHandle = NULL;
                  data->ag_Flags.InitialLayout = TRUE;
               }
               ReleaseDataType(dt);
            }
         }
      }
   }

   if(dtobj == NULL)
   {
      err = IoErr();
      DB(("couldn't create dtobj %ld for node=\"%s\"\n", err, msg->dtg_NodeName));
      if(agobj != NULL)
         FreeAGObject(cl, obj, agobj);
   } else
   {
      AddHead(&data->ag_Visited, &agobj->ago_Node);
      if(msg->dtg_AttrList != NULL)
         agobj->ago_TopVert = GetTagData(DTA_TopVert, 0, msg->dtg_AttrList);
      DB(("top vert is %ld\n", agobj->ago_TopVert));
      ActivateAGObject(cl, obj, msg->dtg_GInfo, agobj);
      rv = 1;
   }

   if(err != 0)
   {
      DB(("error occured %ld for node=\"%s\"\n", err, msg->dtg_NodeName));

      NotifyAttrs(obj, msg->dtg_GInfo, 0,
		  GA_ID, CAST_GAD(obj)->GadgetID,
		  DTA_Busy, FALSE,
		  DTA_ErrorString, msg->dtg_NodeName,
		  DTA_ErrorNumber, err,
		  DTA_ErrorLevel, 1,
		  TAG_DONE);
      rv = -1;
   }

   return rv;
}

#ifdef __AROS__
IPTR AmigaGuide__OM_UPDATE(Class *cl, Object *obj, struct opSet *msg)
{
    ULONG rv;
    
    /* avoid update loops */
    if(DoMethod(obj, ICM_CHECKLOOP))
	return (IPTR)0;

    rv = om_update(cl, obj, msg);
    rv += om_set(cl, obj, msg);

    rv += (ULONG)DoSuperMethodA(cl, obj, (Msg) msg);

    DB(("set returned %ld\n", rv));
    /* this class is derived from the gadgetclass, check
     * if the gadget needs a refresh : */
    if(rv != 0 && (OCLASS (obj) == cl))
    {
	struct RastPort *rp;
	if((rp = ObtainGIRPort(CAST_SET(msg)->ops_GInfo)) != NULL)
	{
            struct gpRender render;
            render.MethodID  = GM_RENDER;
            render.gpr_GInfo = CAST_SET(msg)->ops_GInfo;
            render.gpr_RPort = rp;
            render.gpr_Redraw= GREDRAW_UPDATE;
	    DoMethodA(obj, (Msg) &render);

	    ReleaseGIRPort(rp);
	}
    }

    return (IPTR)rv;
}

IPTR AmigaGuide__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    ULONG rv = om_set(cl, obj, CAST_SET(msg));

    rv += (ULONG)DoSuperMethodA(cl, obj, (Msg) msg);

    DB(("set returned %ld\n", rv));
    /* this class is derived from the gadgetclass, check
     * if the gadget needs a refresh : */
    if(rv != 0 && (OCLASS (obj) == cl))
    {
	struct RastPort *rp;
	if((rp = ObtainGIRPort(CAST_SET(msg)->ops_GInfo)) != NULL)
	{
            struct gpRender render;
            render.MethodID  = GM_RENDER;
            render.gpr_GInfo = CAST_SET(msg)->ops_GInfo;
            render.gpr_RPort = rp;
            render.gpr_Redraw= GREDRAW_UPDATE;
	    DoMethodA(obj, (Msg) &render);

	    ReleaseGIRPort(rp);
	}
    }
    
    return (IPTR)rv;
}

IPTR AmigaGuide__GM_GOACTIVE(Class *cl, Object *obj, struct gpInput *msg)
{
    return AmigaGuide__GM_HANDLEINPUT(cl, obj, msg);
}

IPTR AmigaGuide__DTM_PROCLAYOUT(Class *cl, Object *obj, struct gpLayout *msg)
{
    DoSuperMethodA(cl, obj, (Msg)msg);
    
    return AmigaGuide__DTM_ASYNCLAYOUT(cl, obj, msg);
}
#endif /* __AROS__ */

IPTR class_dispatcher(Class *cl, Object *obj, Msg msg)
{
   CLASSBASE;
   IPTR rv = 0;

   switch(msg->MethodID)
   {
   case OM_NEW:
      rv = AmigaGuide__OM_NEW(cl, obj, CAST_SET(msg));
      break;
   case OM_DISPOSE:
      rv = AmigaGuide__OM_DISPOSE(cl, obj, msg);
      break;

   case OM_GET:
      rv = AmigaGuide__OM_GET(cl,obj,(struct opGet *) msg);
      break;

   case OM_UPDATE:
      /* avoid update loops */
      if(DoMethod(obj, ICM_CHECKLOOP))
	 break;

      rv = om_update(cl, obj, CAST_SET(msg));
   case OM_SET:
      rv += om_set(cl, obj, CAST_SET(msg));

      rv += DoSuperMethodA(cl, obj, (Msg) msg);

      DB(("set returned %ld\n", rv));
      /* this class is derived from the gadgetclass, check
       * if the gadget needs a refresh : */
      if(rv != 0 && (OCLASS (obj) == cl))
      {
	 struct RastPort *rp;
	 if((rp = ObtainGIRPort(CAST_SET(msg)->ops_GInfo)) != NULL)
	 {
            struct gpRender render;
            render.MethodID  = GM_RENDER;
            render.gpr_GInfo = CAST_SET(msg)->ops_GInfo;
            render.gpr_RPort = rp;
            render.gpr_Redraw= GREDRAW_UPDATE;
	    DoMethodA(obj, (Msg) &render);

	    ReleaseGIRPort(rp);
	 }
      }
      break;
   case GM_LAYOUT:
      rv = AmigaGuide__GM_LAYOUT(cl, obj, CAST_GPL(msg));
      break;
   case GM_RENDER:
      rv = AmigaGuide__GM_RENDER(cl, obj, (struct gpRender *) msg);
      break;
   case GM_GOACTIVE:
   case GM_HANDLEINPUT:
      rv = AmigaGuide__GM_HANDLEINPUT(cl, obj, (struct gpInput *) msg);
      break;
   case DTM_REMOVEDTOBJECT:
      rv = AmigaGuide__DTM_REMOVEDTOBJECT(cl, obj, msg);
      break;
   case DTM_PROCLAYOUT:
      rv = DoSuperMethodA(cl, obj, msg);
      /* fall through */
   case DTM_ASYNCLAYOUT:
      rv = AmigaGuide__DTM_ASYNCLAYOUT(cl, obj, CAST_GPL(msg));
      break;
   case DTM_TRIGGER:
      rv = AmigaGuide__DTM_TRIGGER(cl, obj, (struct dtTrigger *) msg);
      break;
   case DTM_GOTO:
      rv = AmigaGuide__DTM_GOTO(cl, obj, (struct dtGoto *) msg);
      break;
   default:
      rv = DoSuperMethodA(cl,obj,msg);
   }

   return rv;
}


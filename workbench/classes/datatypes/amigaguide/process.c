/*
** $PROJECT: amigaguide.datatype
**
** $VER: process.c 50.1 (14.06.03)
**
** $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
**
*/

/* ------------------------------- includes ------------------------------- */

#include "classbase.h"

#include <dos/dostags.h>

/* ---------------------------- arexx autodoc ----------------------------- */

/****** amigaguide.datatype/--arexx-- ****************************************

    NAME
	ARexx interface -- amigaguide.datatype ARexx commands

    FUNCTION
	The amigaguide.datatype provides an own ARexx port for each object.
	It's name is the port name passed via DTA_ARexxPortName plus a
	suffix of '.1' or if this port exists '.2' and so on. If no
        DTA_ARexxPortName is passed "AMIGAGUIDE" is used. For example if you
        start MultiView with an amigaguide object, MultiView passes
        "MULTIVIEW.1" for DTA_ARexxPortName to the amigaguide object. The
        amigaguide.datatype tries to open a port named "MULTIVIEW.1.1".

    AREXX COMMANDS

        BEEP -- DisplayBeep().

        CLOSE -- Close the current database.

        CONTENTS -- Go to the contents node in the database. Same as pressing
            the "Contents" button.

        GETNODECOUNT -- Returns the number of nodes in the database using the
            RESULT variable.

        HELP -- Go to the help node in the database. Same as pressing the
            "Help" button.

        INDEX -- Go to the index node in the database. Same as pressing the
            "Index" button.

        LINK <node name> -- Go to the named node.

        NEXT -- Go to the next physical node in the database. Same as pressing
            the "Browse >" button.

        PREVIOUS -- Go to the previous physical node in the database. Same as
          pressing the "Browse <" button.  New for V40.

        PRINT -- Print the current node. Doesn't return until complete.

        QUIT -- Close the current database.

        RETRACE -- Go to the previous viewed node in the database. Same as
            pressing the "Retrace" button.

        RX <script> -- Launch the specified ARexx script.

    SEE ALSO

******************************************************************************
*
*/

/* -------------------------- AG privat message --------------------------- */

struct AmigaGuideDTMsg
{
   struct Message agm_Msg;
   ULONG agm_Type;

   union
   {
      struct
      {
         ULONG Trigger;
         APTR Data;
      } Trigger;
      struct
      {
         BOOL Initial;
      } Layout;
   } agm_Data;

   struct GadgetInfo agm_GInfo;
};

/* --------------------------- static functions --------------------------- */

static
void DoRexxTrigger(Class *cl, Object *obj, ULONG function, APTR trigger_data)
{
   CLASSBASE;
   INSTDATA;

   if(data->ag_Window != NULL)
   {
      struct dtTrigger dtt;

      dtt.MethodID = DTM_TRIGGER;
      dtt.dtt_GInfo = NULL;
      dtt.dtt_Function = function;
      dtt.dtt_Data = trigger_data;

      DoDTMethodA(obj, data->ag_Window, NULL,  (Msg) &dtt);
   }
}

static
ULONG agdtm_rexxcmd(Class *cl, Object *obj, struct RexxMsg *msg)
{
   CLASSBASE;
   INSTDATA;
   STRPTR cmd;

   DB(("command received : %s\n",msg->rm_Args[0]));

   cmd = CopyString(cl, obj, msg->rm_Args[0]);
   if(cmd != NULL)
   {
      STRPTR ptr = cmd;
      STRPTR cmdend;
      UBYTE chr ;
      while(*ptr != '\0' && *ptr != ' ' && *ptr != '\t')
         ++ptr;
      cmdend = ptr;
      chr = *ptr;
      if(*ptr != '\0')
         *ptr = '\0';

      if(!Stricmp(cmd, "PRINT"))
      {
         /* TODO implement printing ARexx command. */
      } else if(!Stricmp(cmd, "QUIT") || !Stricmp(cmd, "CLOSE"))
      {
         Signal(data->ag_Creator, SIGBREAKF_CTRL_C);
      } else if(!Stricmp(cmd, "RETRACE"))
      {
         DoRexxTrigger(cl, obj, STM_RETRACE, NULL);
      } else if(!Stricmp(cmd, "PREVIOUS"))
      {
         DoRexxTrigger(cl, obj, STM_BROWSE_PREV, NULL);
      } else if(!Stricmp(cmd, "NEXT"))
      {
         DoRexxTrigger(cl, obj, STM_BROWSE_NEXT, NULL);
      } else if(!Stricmp(cmd, "INDEX"))
      {
         DoRexxTrigger(cl, obj, STM_INDEX, NULL);
      } else if(!Stricmp(cmd, "CONTENTS"))
      {
         DoRexxTrigger(cl, obj, STM_CONTENTS, NULL);
      } else if(!Stricmp(cmd, "HELP"))
      {
         DoRexxTrigger(cl, obj, STM_HELP, NULL);
      } else if(!Stricmp(cmd, "LINK") ||
                !Stricmp(cmd, "RXS") || !Stricmp(cmd, "RX"))
      {
         *cmdend = chr;
         DoRexxTrigger(cl, obj, STM_COMMAND | STMD_STRPTR, cmd);
      } else if(!Stricmp(cmd, "GETNODECOUNT"))
      {
         UBYTE numbuf[32];
         mysprintf(cb, numbuf, sizeof(numbuf), "%ld", data->ag_File->agf_NodeCount);
	 msg->rm_Result2 = (ULONG) CreateArgstring(numbuf,strlen(numbuf));
      } else if(!Stricmp(cmd, "BEEP"))
      {
         DisplayBeep(NULL);
      }

      FreeAGVec(cl, obj, cmd);
   }

   return 0;
}

struct ProcMessage
{
   struct Message pm_Message;
   struct ClassBase *pm_ClassBase;
   Class *pm_Class;
   Object *pm_Object;
   struct Task *pm_Parent;
};

#undef SysBase
#ifdef __AROS__
AROS_UFH3S(void, asyncmethodfunc,
    AROS_UFHA(STRPTR, argptr_unused, A0),
    AROS_UFHA(ULONG, argsize_unused, D0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
#else
static
RegCall GetA4 void asyncmethodfunc(void)
{
   struct Library *SysBase = *((struct Library **) 4L);
#endif
   struct Process *proc = (struct Process *) FindTask(NULL);
   struct ClassBase *cb = NULL;
   struct ProcMessage *pmsg;
   struct AmigaGuideData *data;
   struct MsgPort *port,*rexxport;
   Class *cl = NULL;
   Object *obj = NULL;
   ULONG procmask, rexxmask;
   ULONG rcvd;


   WaitPort(&proc->pr_MsgPort);
   pmsg   = (struct ProcMessage *) GetMsg(&proc->pr_MsgPort);
   cb = pmsg->pm_ClassBase;
   cl = pmsg->pm_Class;
   obj = pmsg->pm_Object;
   data = INST_DATA(cl, obj);
   data->ag_Process  = proc;
   data->ag_ProcPort = port = CreateMsgPort();
   data->ag_RexxPort = rexxport = CreateMsgPort();
   if(rexxport != NULL)
   {
      LONG i=0;
      Forbid();
      do
      {
         mysprintf(cb, data->ag_RexxName, sizeof(data->ag_RexxName),
                   "%s.%ld", data->ag_ARexxPortName, ++i);
      } while(FindPort(data->ag_RexxName) != NULL);

      rexxport->mp_Node.ln_Name = data->ag_RexxName;
      AddPort(rexxport);
      Permit();
      DB(("arexx port is %s\n", data->ag_RexxName));
   }

   Signal(pmsg->pm_Parent, SIGBREAKF_CTRL_F);

   if(port != NULL)
   {
      procmask = (1<<port->mp_SigBit);
      if(rexxport != NULL)
         rexxmask = (1<<rexxport->mp_SigBit);
      do
      {
	 rcvd = Wait(SIGBREAKF_CTRL_F | procmask | rexxmask);
	 if(rcvd & SIGBREAKF_CTRL_F)
	    break;

	 if(rcvd & procmask)
	 {
	    struct AmigaGuideDTMsg *msg;

	    while((msg = (struct AmigaGuideDTMsg *) GetMsg(port)) != NULL)
	    {
	       switch(msg->agm_Type)
	       {
	       case AGMT_TRIGGER:
		  DoTrigger(cl, obj,
                            &msg->agm_GInfo, msg->agm_Data.Trigger.Trigger,
                            msg->agm_Data.Trigger.Data);
		  break;
               case AGMT_LAYOUT:
                  {
                     struct DTSpecialInfo *si = (struct DTSpecialInfo *) CAST_GAD(obj)->SpecialInfo;
                     INSTDATA;
                     struct gpLayout layout;

                     layout.MethodID    = DTM_ASYNCLAYOUT;
                     layout.gpl_GInfo   = &msg->agm_GInfo;
                     layout.gpl_Initial = msg->agm_Data.Layout.Initial;

                     ObtainSemaphore(&si->si_Lock);

                     do
                     {
                        si->si_Flags &= ~DTSIF_NEWSIZE;
                        DoMethodA(obj, (Msg) &layout);
                     } while(si->si_Flags & DTSIF_NEWSIZE);
                     si->si_Flags &= ~DTSIF_LAYOUT;

                     if(data->ag_Flags.GotoLine)
                     {
                        /* set possible new vertical top position. */
                        SetAttrs(obj, DTA_TopVert, data->ag_ActualObject->ago_TopVert, TAG_DONE);
                        /* and also tell others... */
                        NotifyAttrs(obj, &msg->agm_GInfo, 0,
               	                    GA_ID,       CAST_GAD(obj)->GadgetID,
                                    DTA_TopVert, data->ag_ActualObject->ago_TopVert,
                                    TAG_DONE);
                        data->ag_Flags.GotoLine = FALSE;
                     }
                     ReleaseSemaphore(&si->si_Lock);
                  }
                  break;
	       }
	       FreeAGMem(cl, obj, msg, sizeof(struct AmigaGuideDTMsg));
	    }
	 }

         if(rcvd & rexxmask)
         {
            struct RexxMsg *rxmsg;

            while((rxmsg = (struct RexxMsg *) GetMsg(rexxport)) != NULL)
            {
	       if(rxmsg->rm_Node.mn_Node.ln_Type == NT_REPLYMSG)
	       {
	          DB(("command returned : %s\n",rxmsg->rm_Args[0]));
	       	  if(rxmsg->rm_Stdout != NULL)
	             Close(rxmsg->rm_Stdout);

	          /* delete commandstring */
		  DeleteArgstring(rxmsg->rm_Args[0]);

		  DeleteRexxMsg(rxmsg);

		  --data->ag_RexxOutstanding;
	       } else
	       {
      	          struct DTSpecialInfo *si = CAST_GAD(obj)->SpecialInfo;

                  ObtainSemaphore(&si->si_Lock);
	          agdtm_rexxcmd(cl, obj, rxmsg);
      	          ReleaseSemaphore(&si->si_Lock);

	          ReplyMsg((struct Message *) rxmsg);
	       }
            }
         }
      } while(TRUE);
   }

   Forbid();
   data->ag_RexxPort = NULL;
   if(rexxport != NULL)
   {
      RemPort(rexxport);

      while(data->ag_RexxOutstanding > 0)
      {
         struct RexxMsg *rxmsg;

         WaitPort(rexxport);
         while((rxmsg = (struct RexxMsg *) GetMsg(rexxport)) != NULL)
         {
            if(rxmsg->rm_Node.mn_Node.ln_Type == NT_REPLYMSG)
            {
	       if(rxmsg->rm_Stdout != NULL)
	          Close(rxmsg->rm_Stdout);

	       /* delete commandstring */
	       DeleteArgstring(rxmsg->rm_Args[0]);

	       DeleteRexxMsg(rxmsg);

	       --data->ag_RexxOutstanding;
            } else
            {
	       ReplyMsg((struct Message *) rxmsg);
            }
         }
      }
      DeleteMsgPort(rexxport);
   }
   data->ag_ProcPort = NULL;
   if(data->ag_Parent != NULL)
      Signal(data->ag_Parent, SIGBREAKF_CTRL_F);
   Permit();
   DeleteMsgPort(port);

   return;
   
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif       
}
#define SysBase ((struct ExecBase *)cb->cb_SysBase)

/* --------------------------- public functions --------------------------- */

BOOL SendAGTrigger(Class *cl, Object *obj, struct dtTrigger *dtt)
{
   CLASSBASE;
   INSTDATA;
   struct AmigaGuideDTMsg *msg;

   msg = AllocAGMem(cl, obj, sizeof(struct AmigaGuideDTMsg));
   if(msg != NULL)
   {
      msg->agm_Msg.mn_Node.ln_Type = NT_MESSAGE;
      msg->agm_Type    = AGMT_TRIGGER;
      msg->agm_Data.Trigger.Trigger = dtt->dtt_Function;
      msg->agm_Data.Trigger.Data    = dtt->dtt_Data;
      msg->agm_GInfo   = *dtt->dtt_GInfo;
      PutMsg(data->ag_ProcPort, &msg->agm_Msg);
   }

   return (msg != NULL) ? TRUE : FALSE;
}

BOOL SendAGLayout(Class *cl, Object *obj, struct gpLayout *gpl)
{
   CLASSBASE;
   INSTDATA;
   struct AmigaGuideDTMsg *msg;

   msg = AllocAGMem(cl, obj, sizeof(struct AmigaGuideDTMsg));
   if(msg != NULL)
   {
      msg->agm_Msg.mn_Node.ln_Type = NT_MESSAGE;
      msg->agm_Type    = AGMT_LAYOUT;
      msg->agm_GInfo   = *gpl->gpl_GInfo;
      msg->agm_Data.Layout.Initial = gpl->gpl_Initial;
      PutMsg(data->ag_ProcPort, &msg->agm_Msg);
   }

   return (msg != NULL) ? TRUE : FALSE;
}


struct Process *CreateAGProcess(Class *cl, Object *obj)
{
   CLASSBASE;
   struct ProcMessage msg;
#ifndef __AROS__
   struct EmulLibEntry GATE_ProcFunc = {TRAP_LIB, 0, (void (*)(void)) asyncmethodfunc};
#endif
   struct Process *proc;

   msg.pm_Message.mn_Node.ln_Type = NT_MESSAGE;
   msg.pm_ClassBase = cb;
   msg.pm_Class = cl;
   msg.pm_Object = obj;
   msg.pm_Parent = FindTask(NULL);

#ifdef __AROS__
   if((proc = CreateNewProcTags(NP_Entry, (IPTR)AROS_ASMSYMNAME(asyncmethodfunc),
				NP_StackSize, 2*8192,
				NP_Name, (ULONG) "AmigaGuide process",
				TAG_DONE)) != NULL)
#else
   if((proc = CreateNewProcTags(NP_Entry, (ULONG) &GATE_ProcFunc,
				NP_StackSize, 2*8192,
				NP_Name, (ULONG) "AmigaGuide process",
				TAG_DONE)) != NULL)
#endif
   {
      PutMsg(&proc->pr_MsgPort, &msg.pm_Message);
      Wait(SIGBREAKF_CTRL_F);
   }

   return proc;
}

void DeleteAGProcess(Class *cl, Object *obj)
{
   CLASSBASE;
   INSTDATA;

   if(data->ag_Process != NULL)
   {
      data->ag_Parent = FindTask(NULL);
      /* simple handshaking using CTRL-F signal between AG and current process */
      Signal(&data->ag_Process->pr_Task, SIGBREAKF_CTRL_F);
      Wait(SIGBREAKF_CTRL_F);
   }
}


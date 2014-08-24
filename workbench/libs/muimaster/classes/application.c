/*
    Copyright � 1999, David Le Corfec.
    Copyright � 2002-2013, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>
#include <devices/timer.h>
#include <dos/dostags.h>
#include <dos/datetime.h>
#include <utility/date.h>
#include <prefs/prefhdr.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <clib/alib_protos.h>
#include <libraries/commodities.h>
#include <rexx/errors.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/commodities.h>
#include <proto/muimaster.h>
#include <proto/iffparse.h>
#include <proto/rexxsyslib.h>
#include <proto/workbench.h>
#include <proto/icon.h>

//#define MYDEBUG 1
#include "debug.h"
#include "prefs.h"

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

#include <string.h>

extern struct Library *MUIMasterBase;


struct TrackingNode
{
    struct MinNode tn_Node;
    Object *tn_Application;
};

struct MUI_ApplicationData
{
    struct MUI_GlobalInfo   app_GlobalInfo;
    APTR                    app_WindowFamily; /* delegates window list */
    struct MinList          app_IHList;
    struct MinList          app_MethodQueue;
    struct SignalSemaphore  app_MethodSemaphore;
    struct MinList          app_ReturnIDQueue;
    struct Hook             *app_BrokerHook;
    struct MsgPort          *app_BrokerPort;
    struct MsgPort          *app_TimerPort;
    struct timerequest      *app_TimerReq;
    struct Task             *app_Task;
    CxObj                   *app_Broker;
    Object                  *app_Menustrip;
    Object                  *app_AboutWin;
    APTR                    app_RIDMemChunk;
    STRPTR                  app_Author;
    STRPTR                  app_Base;
    STRPTR                  app_Copyright;
    STRPTR                  app_Description;
    STRPTR                  app_HelpFile;
    STRPTR                  app_Title;
    STRPTR                  app_Version;
    BOOL                    app_VersionAllocated;
    STRPTR                  app_Version_Number;
    STRPTR                  app_Version_Date;
    STRPTR                  app_Version_Extra;
    WORD                    app_SleepCount; // attribute nests
    ULONG                   app_TimerOutstanding;
    ULONG                   app_MenuAction; /* Remember last action */
    BOOL                    app_ForceQuit;
    BOOL                    app_Iconified;
    BOOL                    app_SingleTask;
    BOOL                    app_Active;
    BYTE                    app_BrokerPri;
    struct TrackingNode     app_TrackingNode;
    BOOL                    app_is_TNode_in_list;
    ULONG searchwinid;
    LONG winposused;       //dont add other vars before windowpos all is save
                           //together
    struct windowpos        winpos[MAXWINS];
    struct MsgPort          *app_RexxPort;
    struct RexxMsg          *app_RexxMsg;
    struct Hook             *app_RexxHook;
    struct MUI_Command      *app_Commands;
    STRPTR                  app_RexxString;
    BOOL                    app_UseRexx;
    struct MsgPort          *app_AppPort; /* Port for AppIcon/AppMenu/AppWindow */
    struct AppIcon          *app_AppIcon;
    struct DiskObject       *app_DiskObject; /* This is only pointer to
                                              * client-managed object */
    struct DiskObject       *app_DefaultDiskObject; /* This is complete
                                                     * object managed by
                                                     * the class */
};

struct timerequest_ext
{
    struct timerequest treq;
    struct MUI_InputHandlerNode *ihn;
};

/*
 * Application class is the master class for all
 * MUI applications. It serves as a kind of anchor
 * for all input, either coming from the user or
 * somewhere from the system, e.g. commodities
 * or ARexx messages. (hemm forget theses last 2 for Zune :)
 *
 * An application can have any number of sub windows,
 * these windows are the children of the application.
 * (FYI, it delegates child handling to a Family object).
 */

/*
MUIA_Application_Active [ISG]             done
MUIA_Application_Author [I.G]             done
MUIA_Application_Base [I.G]               done
MUIA_Application_Broker [..G]             done
MUIA_Application_BrokerHook [ISG]         done
MUIA_Application_BrokerPort [..G]         done
MUIA_Application_BrokerPri [I.G]          done
MUIA_Application_Commands [ISG]           needs Arexx
MUIA_Application_Copyright [I.G]          done
MUIA_Application_Description [I.G]        done
MUIA_Application_DiskObject [ISG]         done
MUIA_Application_DoubleStart [..G]        not triggered yet (todo)
MUIA_Application_DropObject [IS.]         todo
MUIA_Application_ForceQuit [..G]          not triggered yet
MUIA_Application_HelpFile [ISG]           unused/dummy
MUIA_Application_Iconified [.SG]          done
MUIA_Application_Menu [I.G]               unimplemented (OBSOLETE)
MUIA_Application_MenuAction [..G]         done
MUIA_Application_MenuHelp [..G]           todo (ditto)
MUIA_Application_Menustrip [I..]          done
MUIA_Application_RexxHook [ISG]           needs Arexx
MUIA_Application_RexxMsg [..G]            needs Arexx
MUIA_Application_RexxString [.S.]         needs Arexx
MUIA_Application_SingleTask [I..]         done
MUIA_Application_Sleep [.S.]              todo
MUIA_Application_Title [I.G]              done
MUIA_Application_UseCommodities [I..]     done
MUIA_Application_UseRexx [I..]            done
MUIA_Application_Version [I.G]            done
MUIA_Application_Window [I..]             done
MUIA_Application_WindowList [..G]         done

OM_ADDMEMBER                              done
OM_REMMEMBER                              done
MUIM_Application_AboutMUI                 todo
MUIM_Application_AddInputHandler          done ?
MUIM_Application_CheckRefresh             todo (implementable ?)
MUIM_Application_GetMenuCheck             OBSOLETE
MUIM_Application_GetMenuState             OBSOLETE
MUIM_Application_Input                    OBSOLETE
MUIM_Application_InputBuffered            todo
MUIM_Application_Load
MUIM_Application_NewInput                 done
MUIM_Application_OpenConfigWindow
MUIM_Application_PushMethod
MUIM_Application_RemInputHandler          done ?
MUIM_Application_ReturnID                 done
MUIM_Application_Save
MUIM_Application_SetConfigItem
MUIM_Application_SetMenuCheck
MUIM_Application_SetMenuState
MUIM_Application_ShowHelp

Notify.mui/MUIM_FindUData                 done
Notify.mui/MUIM_GetUData                  done
Notify.mui/MUIM_SetUData                  done
Notify.mui/MUIM_SetUDataOnce              done
 */

static const int __version = 1;
static const int __revision = 1;


/*
 * MethodQueueNode
 */
struct MQNode
{
    struct MinNode    mq_Node;
    Object           *mq_Dest;
    LONG              mq_Count;
    IPTR             *mq_Msg;
};

/*
 * FilePrefHeader
 */
struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

#define ID_MUIO MAKE_ID('M','U','I','O')

/*
 * Allocates an MethodQueue Method
 */
static struct MQNode *CreateMQNode(LONG count)
{
    struct MQNode *mq;

    mq = (struct MQNode *)mui_alloc(sizeof(struct MQNode) +
        (count * sizeof(IPTR)));
    if (!mq)
        return NULL;

    mq->mq_Count = count;
    mq->mq_Msg = (IPTR *) (((char *)mq) + sizeof(struct MQNode));
    return mq;
}

/*
 * Free an IQ Method got from CreateIQMethod()
 */
static void DeleteMQNode(struct MQNode *mq)
{
    mui_free(mq);
}


/*
 * Queue of Return IDs
 */
struct RIDNode
{
    struct MinNode  rid_Node;
    ULONG           rid_Value;
};


static struct RIDNode *CreateRIDNode(struct MUI_ApplicationData *data,
    ULONG retid)
{
    struct RIDNode *rid;

    if ((rid = mui_alloc_struct(struct RIDNode)))
    {
        rid->rid_Value = retid;
    }
    return rid;
}


static void DeleteRIDNode(struct MUI_ApplicationData *data,
    struct RIDNode *rid)
{
    mui_free(rid);
}


/**************************************************************************
 Process a pushed method.
**************************************************************************/
static BOOL application_do_pushed_method(struct MUI_ApplicationData *data)
{
    struct MQNode *mq;

    ObtainSemaphore(&data->app_MethodSemaphore);

    if ((mq = (struct MQNode *)RemHead((struct List *)&data->app_MethodQueue)))
    {
        ReleaseSemaphore(&data->app_MethodSemaphore);

        DoMethodA(mq->mq_Dest, (Msg) mq->mq_Msg);
        DeleteMQNode(mq);
        return TRUE;
    }
    ReleaseSemaphore(&data->app_MethodSemaphore);
    return FALSE;
}

static Object *find_application_by_base(struct IClass *cl, Object *obj,
    STRPTR base)
{
    struct TrackingNode *tn;
    Object *retval = NULL;

    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    ForeachNode(&MUIMB(MUIMasterBase)->Applications, tn)
    {
        STRPTR tn_base = "";

        get(tn->tn_Application, MUIA_Application_Base, &tn_base);

        if (tn_base && (strcmp(base, tn_base)) == 0)
        {
            retval = tn->tn_Application;
            break;
        }
    }
    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    return retval;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Application__OM_NEW(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct MUI_ApplicationData *data;
    struct TagItem *tags, *tag;
    BOOL bad_childs = FALSE;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    /* init input handler list */
    NewList((struct List *)&(data->app_IHList));

    /* init input queue */
    NewList((struct List *)&(data->app_MethodQueue));

    /* init return ids queue */
    NewList((struct List *)&(data->app_ReturnIDQueue));

    /* window list */
    data->app_WindowFamily = MUI_NewObjectA(MUIC_Family, NULL);
    if (!data->app_WindowFamily)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    data->app_GlobalInfo.mgi_ApplicationObject = obj;
    if (!(data->app_GlobalInfo.mgi_WindowsPort = CreateMsgPort()))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    data->app_Task = FindTask(NULL);

    /* Parse prefs */
    data->app_SingleTask =
        (BOOL) GetTagData(MUIA_Application_SingleTask, FALSE,
        msg->ops_AttrList);
    data->app_Base =
        (STRPTR) GetTagData(MUIA_Application_Base, (IPTR) "UNNAMED",
        msg->ops_AttrList);
    if (!data->app_Base || strpbrk(data->app_Base, ":/()#?*,"))
    {
        data->app_Base = NULL;  /* don't remove */
        CoerceMethod(cl, obj, OM_DISPOSE);

        return 0;
    }

    if (!data->app_SingleTask)
    {
        /* must append .1, .2, ... to the base name */
        LONG i;
        char portname[255];

        for (i = 1; i < 1000; i++)
        {
            snprintf(portname, 255, "%s.%d", data->app_Base, (int)i);
            if (!find_application_by_base(cl, obj, portname))
                break;
        }
        data->app_Base = StrDup(portname);
    }
    else
    {
        Object *other_app;

        ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
        if ((other_app = find_application_by_base(cl, obj, data->app_Base)))
        {
            //FIXME "Is calling MUIM_Application_PushMethod on an alien
            //application object safe?"
            DoMethod(other_app, MUIM_Application_PushMethod,
                (IPTR) other_app, 3, MUIM_Set, MUIA_Application_DoubleStart,
                TRUE);
            data->app_Base = NULL;
        }
        ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

        data->app_Base = StrDup(data->app_Base);
    }

    if (!data->app_Base)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    data->app_GlobalInfo.mgi_Configdata =
        MUI_NewObject(MUIC_Configdata, MUIA_Configdata_Application, obj,
        TAG_DONE);
    if (!data->app_GlobalInfo.mgi_Configdata)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }
    get(data->app_GlobalInfo.mgi_Configdata, MUIA_Configdata_ZunePrefs,
        &data->app_GlobalInfo.mgi_Prefs);

//    D(bug("muimaster.library/application.c: Message Port created at 0x%lx\n",
//    data->app_GlobalInfo.mgi_WindowPort));

    /* Setup timer stuff */
    if (!(data->app_TimerPort = CreateMsgPort()))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (!(data->app_TimerReq =
            (struct timerequest *)CreateIORequest(data->app_TimerPort,
                sizeof(struct timerequest))))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (OpenDevice(TIMERNAME, UNIT_VBLANK,
            (struct IORequest *)data->app_TimerReq, 0))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    InitSemaphore(&data->app_MethodSemaphore);

    muiNotifyData(obj)->mnd_GlobalInfo = &data->app_GlobalInfo;

    /* parse initial taglist */

    data->app_Active = 1;
    data->app_Title = "Unnamed";
    data->app_Version = "Unnamed 0.0";
    data->app_Description = "?";

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Application_Author:
            data->app_Author = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Base:
            /* moved before config parsing */
            break;

        case MUIA_Application_Copyright:
            data->app_Copyright = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Description:
            data->app_Description = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_HelpFile:
            data->app_HelpFile = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_SingleTask:
            /* moved before config parsing */
            break;

        case MUIA_Application_Title:
            data->app_Title = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Version:
            data->app_Version = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Version_Number:
            data->app_Version_Number = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Version_Date:
            data->app_Version_Date = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Version_Extra:
            data->app_Version_Extra = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Window:
            if (tag->ti_Data)
                DoMethod(obj, OM_ADDMEMBER, tag->ti_Data);
            else
                bad_childs = TRUE;
            break;

        case MUIA_Application_Menustrip:
            data->app_Menustrip = (Object *) tag->ti_Data;
            break;

        case MUIA_Application_BrokerPri:
            data->app_BrokerPri = (BYTE) tag->ti_Data;
            break;

        case MUIA_Application_BrokerHook:
            data->app_BrokerHook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Application_Active:
            data->app_Active = tag->ti_Data ? TRUE : FALSE;
            break;

        case MUIA_Application_UsedClasses:
            {
                STRPTR *list = (STRPTR *) tag->ti_Data;
                if (!list)
                    break;
                while (*list)
                {
                    struct IClass *icl = MUI_GetClass(*list);
                    if (icl)
                        MUI_FreeClass(icl);
                    ++list;
                }
            }
            break;

        case MUIA_Application_UseRexx:
            data->app_UseRexx = tag->ti_Data ? TRUE : FALSE;
            break;

        case MUIA_Application_Commands:
            data->app_Commands = (struct MUI_Command *)tag->ti_Data;
            break;

        case MUIA_Application_RexxHook:
            data->app_RexxHook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Application_DiskObject:
            data->app_DiskObject = (struct DiskObject *)tag->ti_Data;
            break;

        }
    }

    /* create MUIA_Application_Version if NULL */
    if (data->app_Version == NULL
        && data->app_Title != NULL && data->app_Version_Number != NULL)
    {
        STRPTR result = NULL;
        ULONG length = 0;

        /* Calculate length */
        length = strlen("$VER: ") + strlen(data->app_Title) + 1 /* space */
            + strlen(data->app_Version_Number) + 1 /* NULL */ ;

        if (data->app_Version_Date != NULL)
        {
            length += 1 /* space */  + 1        /* ( */
                + strlen(data->app_Version_Date) + 1 /* ) */ ;
        }

        if (data->app_Version_Extra != NULL)
        {
            length += 1 /* space */  + 1        /* [ */
                + strlen(data->app_Version_Extra) + 1 /* ] */ ;
        }

        /* Allocate memory */
        result = AllocVec(length, MEMF_ANY);

        if (result != NULL)
        {
            result[0] = '\0';

            /* Format string */
            strlcat(result, "$VER: ", length);
            strlcat(result, data->app_Title, length);
            strlcat(result, " ", length);
            strlcat(result, data->app_Version_Number, length);

            if (data->app_Version_Date != NULL)
            {
                strlcat(result, " (", length);
                strlcat(result, data->app_Version_Date, length);
                strlcat(result, ")", length);
            }

            if (data->app_Version_Extra != NULL)
            {
                strlcat(result, " [", length);
                strlcat(result, data->app_Version_Extra, length);
                strlcat(result, "]", length);
            }

            data->app_Version = result;
            data->app_VersionAllocated = TRUE;
        }

    }

    if (bad_childs)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (CxBase
        && GetTagData(MUIA_Application_UseCommodities, TRUE,
            msg->ops_AttrList))
    {
        data->app_BrokerPort = CreateMsgPort();

        if (data->app_BrokerPort)
        {
            struct NewBroker nb;

            nb.nb_Version = NB_VERSION;
            nb.nb_Name =
                data->app_Title ? data->app_Title : (STRPTR) "Unnamed";
            nb.nb_Title =
                data->app_Version ? data->app_Version : (STRPTR) "Unnamed";
            nb.nb_Descr =
                data->app_Description ? data->
                app_Description : (STRPTR) "?";
            nb.nb_Unique = 0;
            nb.nb_Flags = COF_SHOW_HIDE;
            nb.nb_Pri = data->app_BrokerPri;
            nb.nb_Port = data->app_BrokerPort;
            nb.nb_ReservedChannel = 0;

            if (strncmp(nb.nb_Title, "$VER: ", 6) == 0)
                nb.nb_Title += 6;

            data->app_Broker = CxBroker(&nb, 0);

            if (data->app_Broker)
            {
                if (data->app_Active)
                    ActivateCxObj(data->app_Broker, 1);
            }
        }
    }

    if (data->app_UseRexx)
    {
        data->app_RexxPort = CreateMsgPort();
        if (data->app_RexxPort)
        {
            data->app_RexxPort->mp_Node.ln_Name = StrDup(data->app_Base);
            if (data->app_RexxPort->mp_Node.ln_Name != NULL)
            {
                D(bug("[MUI] %s is using REXX!\n",
                        data->app_RexxPort->mp_Node.ln_Name));
                char *i;
                for (i = data->app_RexxPort->mp_Node.ln_Name; *i != '\0';
                    i++)
                {
                    *i = toupper(*i);
                }
                AddPort(data->app_RexxPort);
            }
            else
            {
                DeleteMsgPort(data->app_RexxPort);
                data->app_RexxPort = NULL;
            }
        }
    }

    if (data->app_Menustrip)
        DoMethod(data->app_Menustrip, MUIM_ConnectParent, (IPTR) obj);

    data->app_AppPort = CreateMsgPort();
    data->app_GlobalInfo.mgi_AppPort = data->app_AppPort;
    if (data->app_AppPort == NULL)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    data->app_TrackingNode.tn_Application = obj;
    AddTail((struct List *)&MUIMB(MUIMasterBase)->Applications,
        (struct Node *)&data->app_TrackingNode);
    data->app_is_TNode_in_list = TRUE;
    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    return (IPTR) obj;
}



/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Application__OM_DISPOSE(struct IClass *cl, Object *obj,
    Msg msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct RIDNode *rid;

    long positionmode;
    if (data->app_Base)
    {
        char filename[255];
        positionmode = data->app_GlobalInfo.mgi_Prefs->window_position;
        if (positionmode >= 1)
        {
            snprintf(filename, 255, "ENV:zune/%s.prefs", data->app_Base);
            DoMethod(data->app_GlobalInfo.mgi_Configdata,
                MUIM_Configdata_Save, (IPTR) filename);
        }
        if (positionmode == 2)
        {
            snprintf(filename, 255, "ENVARC:zune/%s.prefs", data->app_Base);
            DoMethod(data->app_GlobalInfo.mgi_Configdata,
                MUIM_Configdata_Save, (IPTR) filename);
        }
    }

    if (data->app_is_TNode_in_list)
    {
        ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
        Remove((struct Node *)&data->app_TrackingNode);
        ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    }

    if (data->app_WindowFamily)
    {
        struct MinList *children = NULL;
        Object *cstate;
        Object *child;

        /* special loop because the next object may have been removed/freed by
         *  the previous. so restart from listhead each time.
         */
        while (1)
        {
            get(data->app_WindowFamily, MUIA_Family_List, &children);
            if (children == NULL)
                break;

            cstate = (Object *) children->mlh_Head;
            if ((child = NextObject(&cstate)))
            {
                D(bug("Application_Dispose(%p) : OM_REMMEMBER(%p)\n", obj,
                        child));
                DoMethod(obj, OM_REMMEMBER, (IPTR) child);
                D(bug("Application_Dispose(%p) : MUI_DisposeObject(%p)\n",
                        obj, child));
                MUI_DisposeObject(child);

            }
            else
            {
                break;
            }
        }
        MUI_DisposeObject(data->app_WindowFamily);
    }

    if (data->app_Menustrip)
        MUI_DisposeObject(data->app_Menustrip);

    if (data->app_VersionAllocated && data->app_Version != NULL)
    {
        FreeVec(data->app_Version);
    }

    /* free commodities stuff */

    if (data->app_Broker)
    {
        DeleteCxObjAll(data->app_Broker);
    }

    if (data->app_BrokerPort)
    {
        struct Message *msg;

        while ((msg = GetMsg(data->app_BrokerPort)))
        {
            ReplyMsg(msg);
        }

        DeleteMsgPort(data->app_BrokerPort);
    }

    /* free timer stuff */
    if (data->app_TimerReq)
    {
        if (data->app_TimerReq->tr_node.io_Device)
        {
            while (data->app_TimerOutstanding)
            {
                if (Wait(1L << data->app_TimerPort->
                        mp_SigBit | 4096) & 4096)
                    break;
                data->app_TimerOutstanding--;
            }
            CloseDevice((struct IORequest *)data->app_TimerReq);
        }
        DeleteIORequest((struct IORequest *)data->app_TimerReq);
    }
    DeleteMsgPort(data->app_TimerPort);

    if (data->app_RexxPort)
    {
        struct Message *msg;
        while ((msg = GetMsg(data->app_RexxPort)))
        {
            ReplyMsg(msg);
        }
        RemPort(data->app_RexxPort);

        FreeVec(data->app_RexxPort->mp_Node.ln_Name);
        DeleteMsgPort(data->app_RexxPort);
    }

    if (data->app_AppIcon)
        RemoveAppIcon(data->app_AppIcon);

    if (data->app_DefaultDiskObject)
        FreeDiskObject(data->app_DefaultDiskObject);

    if (data->app_AppPort)
    {
        struct Message *msg;
        while ((msg = GetMsg(data->app_AppPort)))
            ReplyMsg(msg);

        DeleteMsgPort(data->app_AppPort);
    }

    if (data->app_GlobalInfo.mgi_Configdata)
        MUI_DisposeObject(data->app_GlobalInfo.mgi_Configdata);

    DeleteMsgPort(data->app_GlobalInfo.mgi_WindowsPort);

    FreeVec(data->app_Base);

    /* free returnid stuff */

    while ((rid =
            (struct RIDNode *)RemHead((struct List *)&data->
                app_ReturnIDQueue)))
    {
        DeleteRIDNode(data, rid);
    }

    return DoSuperMethodA(cl, obj, msg);
}


/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Application__OM_SET(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    /* There are many ways to find out what tag items provided by set()
     ** we do know. The best way should be using NextTagItem() and simply
     ** browsing through the list.
     */
    while ((tag = NextTagItem(&tags)) != NULL)
    {
        IPTR *addr;
        switch (tag->ti_Tag)
        {

        case MUIA_Application_SearchWinId:
            data->searchwinid = tag->ti_Data;
            break;

        case MUIA_Application_CopyWinPosToApp:
            addr = (IPTR *) tag->ti_Data;
            CopyMem((CONST_APTR) tag->ti_Data, &data->winposused, *(addr));
            break;

        case MUIA_Application_SetWinPos:
            {
                struct windowpos *winp;
                winp = (struct windowpos *)tag->ti_Data;
                //kprintf("SetWinPos %d %d %d %d %d\n", winp->id, winp->x1,
                //    winp->y1, winp->w1, winp->h1);
                int i;
                for (i = 0; i < MAXWINS - 1; i++)
                {
                    if (data->winpos[i].w1)
                    {
                        if (winp->id == data->winpos[i].id)
                        {
                            //existing entry is overwritten
                            data->winpos[i].x1 = winp->x1;
                            data->winpos[i].y1 = winp->y1;
                            data->winpos[i].w1 = winp->w1;
                            data->winpos[i].h1 = winp->h1;
                            data->winpos[i].x2 = winp->x2;
                            data->winpos[i].y2 = winp->y2;
                            data->winpos[i].w2 = winp->w2;
                            data->winpos[i].h2 = winp->h2;
                            break;
                        }
                    }
                    else
                    {
                        // a new entry is added
                        data->winpos[i].id = winp->id;
                        data->winpos[i].x1 = winp->x1;
                        data->winpos[i].y1 = winp->y1;
                        data->winpos[i].w1 = winp->w1;
                        data->winpos[i].h1 = winp->h1;
                        data->winpos[i].x2 = winp->x2;
                        data->winpos[i].y2 = winp->y2;
                        data->winpos[i].w2 = winp->w2;
                        data->winpos[i].h2 = winp->h2;
                        break;
                    }
                }
            }
            break;

        case MUIA_Application_Configdata:
            DoMethod(obj, MUIM_Application_PushMethod, (IPTR) obj, 2,
                MUIM_Application_SetConfigdata, tag->ti_Data);
            break;

        case MUIA_Application_HelpFile:
            data->app_HelpFile = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Iconified:
            {
                BOOL do_iconify = tag->ti_Data == 1;
                if (data->app_Iconified != do_iconify)
                {
                    data->app_Iconified = do_iconify;

                    nnset(obj, MUIA_ShowMe, !data->app_Iconified);

                    /* Inform workbench.library */
                    if (data->app_Iconified)
                    {
                        STRPTR appname =
                            data->app_Title ? data->
                            app_Title : (STRPTR) "Unnamed";
                        struct DiskObject *dobj =
                            (struct DiskObject *)XGET(obj,
                            MUIA_Application_DiskObject);

                        if (dobj == NULL)
                        {
                            /* Get default AppIcon in ENV:SYS or ENVARC:SYS */
                            dobj = GetDefDiskObject(WBAPPICON);
                            if (dobj)
                                data->app_DefaultDiskObject = dobj;
                            else
                            {
                                /* First default: ENV:SYS/def_MUI.info */
                                dobj = GetDiskObject("ENV:SYS/def_MUI");
                                if (dobj)
                                    data->app_DefaultDiskObject = dobj;
                                else
                                {
                                    /* Second default: ENV:SYS/def_Zune.info */
                                    dobj =
                                        GetDiskObject("ENV:SYS/def_Zune");
                                    if (dobj)
                                        data->app_DefaultDiskObject = dobj;
                                    else
                                    {
                                        /* Third default: default tool icon */
                                        dobj = GetDefDiskObject(WBTOOL);
                                        if (dobj)
                                            data->app_DefaultDiskObject =
                                                dobj;
                                    }
                                }
                            }
                        }

                        if (dobj == NULL)
                            break;

                        dobj->do_CurrentX = NO_ICON_POSITION;
                        dobj->do_CurrentY = NO_ICON_POSITION;

                        data->app_AppIcon =
                            AddAppIconA(0L, 0L, appname, data->app_AppPort,
                            BNULL, dobj, NULL);
                    }
                    else
                    {
                        if (data->app_AppIcon)
                        {
                            RemoveAppIcon(data->app_AppIcon);
                            data->app_AppIcon = NULL;
                        }
                        if (data->app_DefaultDiskObject)
                        {
                            FreeDiskObject(data->app_DefaultDiskObject);
                            data->app_DefaultDiskObject = NULL;
                        }
                    }
                }
            }
            break;

        case MUIA_ShowMe:
            {
                /* Ok ok, you think this stinks? Well, think of it as
                   an attribute belonging to an interface which
                   MUIC_Application, together with MUIC_Area and a few
                   others implement. It makes sense now, yes? */
                struct List *wlist = NULL;
                APTR wstate;
                Object *curwin = NULL;
                Object *lastwin = NULL;

                /* MUIA_ShowMe can cause MUIM_Setup/MUIM_Cleanup to be issued.
                 * On the other hand it is allowed to add/remove other
                 * application windows in MUIM_Setup/MUIM_Cleanup.
                 * This means after processing a window from internal list,
                 * the list needs to be re-read and iteration started again,
                 * because wstate can become invalid.
                 * Note: The code below assumes that the window won't remove
                 * itself from the list.
                 */

                while (1)
                {
                    get(data->app_WindowFamily, MUIA_Family_List, &wlist);
                    wstate = (Object *) wlist->lh_Head;
                    while ((curwin = NextObject(&wstate)))
                    {
                        if (lastwin == NULL)
                            break;
                        if (curwin == lastwin)
                        {
                            curwin = NextObject(&wstate);
                            break;
                        }
                    }

                    /* This is the window to be processed */
                    if (curwin)
                    {
                        set(curwin, MUIA_ShowMe, tag->ti_Data);
                        lastwin = curwin;
                    }
                    else
                    {
                        /* No more windows */
                        break;
                    }
                }
            }
            break;

        case MUIA_Application_Sleep:
            {
                struct List *wlist = NULL;
                APTR wstate;
                Object *curwin;

                if (tag->ti_Data)
                {
                    data->app_SleepCount++;
                    if (data->app_SleepCount == 1)
                    {
                        get(obj, MUIA_Application_WindowList, &wlist);
                        if (wlist)
                        {
                            wstate = wlist->lh_Head;
                            while ((curwin = NextObject(&wstate)))
                            {
                                set(curwin, MUIA_Window_Sleep, TRUE);
                            }
                        }
                    }
                }
                else
                {
                    data->app_SleepCount--;
                    if (data->app_SleepCount == 0)
                    {
                        get(obj, MUIA_Application_WindowList, &wlist);
                        if (wlist)
                        {
                            wstate = wlist->lh_Head;
                            while ((curwin = NextObject(&wstate)))
                            {
                                set(curwin, MUIA_Window_Sleep, FALSE);
                            }
                        }
                    }
                }
            }
            break;

        case MUIA_Application_MenuAction:
            data->app_MenuAction = tag->ti_Data;
            break;

        case MUIA_Application_BrokerHook:
            data->app_BrokerHook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Application_Active:
            data->app_Active = tag->ti_Data ? TRUE : FALSE;
            if (data->app_Broker)
            {
                ActivateCxObj(data->app_Broker, data->app_Active);
            }
            break;

        case MUIA_Application_Commands:
            data->app_Commands = (struct MUI_Command *)tag->ti_Data;
            break;

        case MUIA_Application_RexxString:
            data->app_RexxString = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_RexxHook:
            data->app_RexxHook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Application_DiskObject:
            data->app_DiskObject = (struct DiskObject *)tag->ti_Data;
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/*
 * OM_GET
 */
static IPTR Application__OM_GET(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Application_GetWinPosAddr:
        STORE = (IPTR) & data->winposused;
        return TRUE;

    case MUIA_Application_GetWinPosSize:
        {
            int i;
            for (i = 0; i < MAXWINS - 1; i++)
            {
                if (!data->winpos[i].w1)
                {
                    i *= sizeof(struct windowpos);
                    i += sizeof(long);
                    data->winposused = i;
                    STORE = i;
                    return (TRUE);
                }
            }
            STORE = 0;
            return TRUE;
        }

    case MUIA_Application_GetWinPos:
        {
            int i;
            if (data->searchwinid)
            {
                for (i = 0; i < MAXWINS - 1; i++)
                {
                    if (data->winpos[i].w1)
                    {
                        if (data->searchwinid == data->winpos[i].id)
                        {
                            STORE = (IPTR) & data->winpos[i].id;
                            return 1;
                        }
                    }
                    else
                        break;
                }
            }
            STORE = 0;
            return 1;
        }
        return TRUE;

    case MUIA_Version:
        STORE = __version;
        return TRUE;

    case MUIA_Revision:
        STORE = __revision;
        return TRUE;

    case MUIA_Application_Author:
        STORE = (IPTR) data->app_Author;
        return TRUE;

    case MUIA_Application_Base:
        STORE = (IPTR) data->app_Base;
        return TRUE;

    case MUIA_Application_Copyright:
        STORE = (IPTR) data->app_Copyright;
        return TRUE;

    case MUIA_Application_Description:
        STORE = (IPTR) data->app_Description;
        return TRUE;

    case MUIA_Application_DoubleStart:
        return TRUE;

    case MUIA_Application_ForceQuit:
        STORE = (IPTR) data->app_ForceQuit;
        return TRUE;

    case MUIA_Application_HelpFile:
        STORE = (IPTR) data->app_HelpFile;
        return TRUE;

    case MUIA_Application_Iconified:
        STORE = (IPTR) data->app_Iconified;
        return TRUE;

    case MUIA_Application_Title:
        STORE = (IPTR) data->app_Title;
        return TRUE;

    case MUIA_Application_Version:
        STORE = (IPTR) data->app_Version;
        return TRUE;

    case MUIA_Application_Version_Number:
        STORE = (IPTR) data->app_Version_Number;
        return TRUE;

    case MUIA_Application_Version_Date:
        STORE = (IPTR) data->app_Version_Date;
        return TRUE;

    case MUIA_Application_Version_Extra:
        STORE = (IPTR) data->app_Version_Extra;
        return TRUE;

    case MUIA_Application_WindowList:
        return GetAttr(MUIA_Family_List, data->app_WindowFamily,
            msg->opg_Storage);

    case MUIA_Application_Menustrip:
        STORE = (IPTR) data->app_Menustrip;
        return TRUE;

    case MUIA_Application_MenuAction:
        STORE = (IPTR) data->app_MenuAction;
        return TRUE;

    case MUIA_Application_BrokerPort:
        STORE = (IPTR) data->app_BrokerPort;
        return TRUE;

    case MUIA_Application_BrokerPri:
        STORE = (IPTR) data->app_BrokerPri;
        return TRUE;

    case MUIA_Application_BrokerHook:
        STORE = (IPTR) data->app_BrokerHook;
        return TRUE;

    case MUIA_Application_Broker:
        STORE = (IPTR) data->app_Broker;
        return TRUE;

    case MUIA_Application_Active:
        STORE = data->app_Active;
        return TRUE;

    case MUIA_Application_Commands:
        STORE = (IPTR) data->app_Commands;
        return TRUE;

    case MUIA_Application_RexxMsg:
        STORE = (IPTR) data->app_RexxMsg;
        return TRUE;

    case MUIA_Application_RexxHook:
        STORE = (IPTR) data->app_RexxHook;
        return TRUE;

    case MUIA_Application_DiskObject:
        STORE = (IPTR) data->app_DiskObject;
        return TRUE;
    }

    /* our handler didn't understand the attribute, we simply pass
     ** it to our superclass now
     */
    return (DoSuperMethodA(cl, obj, (Msg) msg));
#undef STORE
}


/**************************************************************************
 OM_ADDMEMBER
**************************************************************************/
static IPTR Application__OM_ADDMEMBER(struct IClass *cl, Object *obj,
    struct opMember *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    D(bug("Application_AddMember: Adding 0x%lx to window member list\n",
            msg->opam_Object));

    DoMethodA(data->app_WindowFamily, (Msg) msg);
    /* Application knows its GlobalInfo, so we can inform window */
    DoMethod(msg->opam_Object, MUIM_ConnectParent, (IPTR) obj);
    return TRUE;
}


/**************************************************************************
 OM_REMMEMBER
**************************************************************************/
static IPTR Application__OM_REMMEMBER(struct IClass *cl, Object *obj,
    struct opMember *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    D(bug("Application_RemMember: Removing 0x%lx from window member list\n",
            msg->opam_Object));

    DoMethod(msg->opam_Object, MUIM_DisconnectParent);
    DoMethodA(data->app_WindowFamily, (Msg) msg);

    return TRUE;
}



/**************************************************************************
 MUIM_Application_AddInputHandler
**************************************************************************/
static IPTR Application__MUIM_AddInputHandler(struct IClass *cl,
    Object *obj, struct MUIP_Application_AddInputHandler *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (msg->ihnode->ihn_Flags & MUIIHNF_TIMER)
    {
        struct timerequest_ext *time_ext =
            (struct timerequest_ext *)AllocVec(sizeof(struct
                timerequest_ext), MEMF_PUBLIC);
        if (time_ext)
        {
            /* Store the request inside the input handler, so that we can
             ** remove the inputhandler without problems */
            msg->ihnode->ihn_Node.mln_Pred = (struct MinNode *)time_ext;

            time_ext->treq = *data->app_TimerReq;
            time_ext->treq.tr_node.io_Command = TR_ADDREQUEST;
            time_ext->treq.tr_time.tv_secs = msg->ihnode->ihn_Millis / 1000;
            time_ext->treq.tr_time.tv_micro =
                (msg->ihnode->ihn_Millis % 1000) * 1000;
            time_ext->ihn = msg->ihnode;
            SendIO((struct IORequest *)time_ext);
        }
    }
    else
        AddTail((struct List *)&data->app_IHList,
            (struct Node *)msg->ihnode);
    return TRUE;
}


/**************************************************************************
 MUIM_Application_RemInputHandler
**************************************************************************/
static IPTR Application__MUIM_RemInputHandler(struct IClass *cl,
    Object *obj, struct MUIP_Application_RemInputHandler *msg)
{
    //struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    if (msg->ihnode->ihn_Flags & MUIIHNF_TIMER)
    {
        struct timerequest_ext *time_ext =
            (struct timerequest_ext *)msg->ihnode->ihn_Node.mln_Pred;
        if (!CheckIO((struct IORequest *)time_ext))
            AbortIO((struct IORequest *)time_ext);
        WaitIO((struct IORequest *)time_ext);
        FreeVec(time_ext);
    }
    else
        Remove((struct Node *)msg->ihnode);

    return TRUE;
}


void _zune_window_message(struct IntuiMessage *imsg);   /* from window.c */

/*
 * MUIM_Application_InputBuffered : process all pending events
 */
static IPTR Application__MUIM_InputBuffered(struct IClass *cl, Object *obj,
    struct MUIP_Application_InputBuffered *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct IntuiMessage *imsg;

    /* process all pushed methods */
    while (application_do_pushed_method(data))
        ;

    imsg =
        (struct IntuiMessage *)GetMsg(data->app_GlobalInfo.mgi_WindowsPort);
    if (imsg != NULL)
    {
        /* Let window object process message */
        _zune_window_message(imsg);     /* will reply the message */
    }
    return TRUE;
}

/**************************************************************************
 MUIM_Application_NewInput : application main loop
**************************************************************************/
static IPTR Application__MUIM_NewInput(struct IClass *cl, Object *obj,
    struct MUIP_Application_NewInput *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct RIDNode *rid;
    ULONG retval = 0;
    ULONG signal, signalmask;
    ULONG handler_mask = 0;     /* the mask of the signal handlers */
    struct MinNode *mn;

    //struct MinNode ihn_Node;

    signal = *msg->signal;

    /* process all pushed methods */
    while (application_do_pushed_method(data))
        /* nothing */ ;

    /* query the signal for the handlers */
    for (mn = data->app_IHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
        struct MUI_InputHandlerNode *ihn;
        ihn = (struct MUI_InputHandlerNode *)mn;
        handler_mask |= ihn->ihn_Signals;
    }

    signalmask = (1L << data->app_GlobalInfo.mgi_WindowsPort->mp_SigBit)
        | (1L << data->app_TimerPort->mp_SigBit) | handler_mask;

    if (data->app_Broker)
        signalmask |= (1L << data->app_BrokerPort->mp_SigBit);

    if (data->app_RexxPort)
        signalmask |= (1L << data->app_RexxPort->mp_SigBit);

    if (data->app_AppPort)
        signalmask |= (1L << data->app_AppPort->mp_SigBit);

    if (signal == 0)
    {
        /* Stupid app which (always) passes 0 in signals. It's impossible to
           know which signals were really set as the app will already have
           called Wait() which has cleared the task's tc_SigRecvd. So assume
           the window, timer, and broker signals all to be set. Also all of
           the inputhandler signals (MUI does that too). */

        signal = signalmask;
    }

    if (signal & signalmask)
    {
        if (signal & (1L << data->app_GlobalInfo.mgi_WindowsPort->
                mp_SigBit))
        {
            struct IntuiMessage *imsg;
            /* process all pushed methods */

            while ((imsg =
                    (struct IntuiMessage *)GetMsg(data->app_GlobalInfo.
                        mgi_WindowsPort)))
            {
                /* Let window object process message */
                _zune_window_message(imsg);     /* will reply the message */
            }
        }

        if (signal & (1L << data->app_TimerPort->mp_SigBit))
        {
            struct timerequest_ext *time_ext;
            struct Node *n;
            struct List list;
            NewList(&list);

            /* At first we fetch all messages from the message port and store
             ** them in a list, we use the node of the Message here */
            while ((time_ext =
                    (struct timerequest_ext *)GetMsg(data->app_TimerPort)))
                AddTail(&list, (struct Node *)time_ext);

            /* Now we proccess the list and resend the timer io, no loop can
             ** happen. We use RemHead() because the handler can remove it
             ** itself and so a FreeVec() could happen in
             ** MUIM_Application_RemInputHandler which would destroy the
             ** ln->Succ of course */
            while ((n = RemHead(&list)))
            {
                struct timerequest_ext *time_ext =
                    (struct timerequest_ext *)n;
                struct MUI_InputHandlerNode *ihn = time_ext->ihn;
                time_ext->treq.tr_time.tv_secs =
                    time_ext->ihn->ihn_Millis / 1000;
                time_ext->treq.tr_time.tv_micro =
                    (time_ext->ihn->ihn_Millis % 1000) * 1000;
                SendIO((struct IORequest *)&time_ext->treq);
                DoMethod(ihn->ihn_Object, ihn->ihn_Method);
            }
        }

        if (data->app_BrokerPort
            && (signal & (1L << data->app_BrokerPort->mp_SigBit)))
        {
            CxMsg *msg;

            while ((msg = (CxMsg *) GetMsg(data->app_BrokerPort)))
            {
                switch (CxMsgType(msg))
                {
                case CXM_COMMAND:
                    switch (CxMsgID(msg))
                    {
                    case CXCMD_DISABLE:
                        set(obj, MUIA_Application_Active, FALSE);
                        break;

                    case CXCMD_ENABLE:
                        set(obj, MUIA_Application_Active, TRUE);
                        break;

                    case CXCMD_APPEAR:
                    case CXCMD_DISAPPEAR:
                        /* No default handling - application needs to be in
                         * control of this */
                        break;

                    case CXCMD_KILL:
                        SetSignal(SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C);
                        break;
                    }
                    break;
                }

                if (data->app_BrokerHook)
                {
                    CallHookPkt(data->app_BrokerHook, obj, msg);
                }

                ReplyMsg((struct Message *)msg);
            }
        }

        if (data->app_RexxPort
            && (signal & (1L << data->app_RexxPort->mp_SigBit)))
        {

            D(bug("[MUI] Got Rexx message!\n"));
            struct Message *msg;
            while ((msg = GetMsg(data->app_RexxPort)))
            {
                ReplyMsg(msg);
            }
        }

        if (data->app_AppPort
            && (signal & (1L << data->app_AppPort->mp_SigBit)))
        {
            struct AppMessage *appmsg;
            while ((appmsg =
                    (struct AppMessage *)GetMsg(data->app_AppPort)))
            {
                if ((appmsg->am_Type == AMTYPE_APPICON)
                    && (appmsg->am_NumArgs == 0)
                    && (appmsg->am_ArgList == NULL)
                    && (XGET(obj, MUIA_Application_Iconified) == TRUE))
                {
                    /* Reply before removing AppIcon */
                    ReplyMsg((struct Message *)appmsg);
                    set(obj, MUIA_Application_Iconified, FALSE);
                    continue;
                }
                else if (appmsg->am_Type == AMTYPE_APPWINDOW)
                {
                    set((Object *) appmsg->am_UserData, MUIA_AppMessage,
                        appmsg);
                }

                ReplyMsg((struct Message *)appmsg);
            }
        }

        if (signal & handler_mask)
        {
            for (mn = data->app_IHList.mlh_Head; mn->mln_Succ;
                mn = mn->mln_Succ)
            {
                struct MUI_InputHandlerNode *ihn;
                ihn = (struct MUI_InputHandlerNode *)mn;
                if (signal & ihn->ihn_Signals)
                    DoMethod(ihn->ihn_Object, ihn->ihn_Method);
            }
        }
    }

    /* process all pushed methods - again */
    while (application_do_pushed_method(data))
        /* nothing */ ;

    *msg->signal = signalmask;

    /* set return code */
    if ((rid =
            (struct RIDNode *)RemHead((struct List *)&data->
                app_ReturnIDQueue)))
    {
        retval = rid->rid_Value;
        DeleteRIDNode(data, rid);
        return retval;
    }
    return 0;
}

/**************************************************************************
 MUIM_Application_Input : application main loop
 This method shouldn't be used in any new program. As it polls all signals.
**************************************************************************/
static IPTR Application__MUIM_Input(struct IClass *cl, Object *obj,
    struct MUIP_Application_Input *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    ULONG signal = 0, handler_mask = 0;
    struct MinNode *mn;

    /* query the signal for the handlers */
    for (mn = data->app_IHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
        struct MUI_InputHandlerNode *ihn;
        ihn = (struct MUI_InputHandlerNode *)mn;
        handler_mask |= ihn->ihn_Flags;
    }

    signal = (1L << data->app_GlobalInfo.mgi_WindowsPort->mp_SigBit)
        | (1L << data->app_TimerPort->mp_SigBit) | handler_mask;

    if (data->app_RexxPort)
        signal |= (1L << data->app_RexxPort->mp_SigBit);

    if (data->app_AppPort)
        signal |= (1L << data->app_AppPort->mp_SigBit);


    *msg->signal = signal;
    return Application__MUIM_NewInput(cl, obj, (APTR) msg);
}

/**************************************************************************
 MUIM_Application_PushMethod: Add a method in the method FIFO. Will
 be executed in the next event loop.
**************************************************************************/
static IPTR Application__MUIM_PushMethod(struct IClass *cl, Object *obj,
    struct MUIP_Application_PushMethod *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct MQNode *mq;
    LONG i;
    IPTR *m = (IPTR *) & msg->count; /* FIXME: breaks on 64-bit BigEndian systems */
    LONG count;

    count = msg->count & 0xf; /* MUI4 uses count to pass additional info */

    mq = CreateMQNode(count);
    if (!mq)
        return 0;
    mq->mq_Dest = msg->dest;

    /* fill msg */
    for (i = 0; i < count; i++)
        mq->mq_Msg[i] = *(m + 1 + i);

    /* enqueue method */
    ObtainSemaphore(&data->app_MethodSemaphore);
    AddTail((struct List *)&data->app_MethodQueue, (struct Node *)mq);
    ReleaseSemaphore(&data->app_MethodSemaphore);

    /* CHECKME: to wake task up as soon as possible! */
    Signal(data->app_Task,
        1L << data->app_GlobalInfo.mgi_WindowsPort->mp_SigBit);

    return (IPTR)mq;
}

/**************************************************************************
 MUIM_Application_UnpushMethod: Removes a method which was added by
 MUIM_Application_PushMethod.
**************************************************************************/
static IPTR Application__MUIM_UnpushMethod(struct IClass *cl, Object *obj,
    struct MUIP_Application_UnpushMethod *msg)
{
    D(bug("[Application__MUIM_UnpushMethod] dest %p id %p method %u\n", msg->dest, msg->methodid, msg->method));

    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    struct MQNode *current, *next;
    ULONG removed = 0;

    ObtainSemaphore(&data->app_MethodSemaphore);
    ForeachNodeSafe(&data->app_MethodQueue, current, next)
    {
        D(bug("[Application__MUIM_UnpushMethod] examine dest %p id %p method %u\n",
                current->mq_Dest, current, current->mq_Msg[0]));
        if (
               ((msg->dest == NULL) || (msg->dest == current->mq_Dest))
            && ((msg->methodid == 0) || (msg->methodid == (IPTR)current))   // Method identifier returned by
                                                                            // MUIM_Application_PushMethod.
            && ((msg->method == 0) || (msg->method == current->mq_Msg[0]))
        )
        {
            Remove((struct Node*)current);
            DeleteMQNode(current);
            removed++;
            D(bug("[Application__MUIM_UnpushMethod] current %p removed\n", current));
        }
    }
    ReleaseSemaphore(&data->app_MethodSemaphore);

    return removed;
}


/*
 * MUIM_Application_ReturnID : Tell MUI to return the given id with
 * the next call to MUIM_Application_NewInput. kinda obsolete :)
 */
static IPTR Application__MUIM_ReturnID(struct IClass *cl, Object *obj,
    struct MUIP_Application_ReturnID *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct RIDNode *rid;

/*
    if (!data->app_RIDMemChunk)
    {
        data->app_RIDMemChunk =
            g_mem_chunk_create(struct RIDNode, 10, G_ALLOC_AND_FREE);
    }
*/
    rid = CreateRIDNode(data, msg->retid);
    if (!rid)
        return FALSE;
    AddTail((struct List *)&data->app_ReturnIDQueue, (struct Node *)rid);
    return TRUE;
}


/*
 * MUIM_FindUData : tests if the MUIA_UserData of the object
 * contains the given <udata> and returns the object pointer in this case.
 */
static IPTR Application__MUIM_FindUData(struct IClass *cl, Object *obj,
    struct MUIP_FindUData *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
        return (IPTR) obj;

    return DoMethodA(data->app_WindowFamily, (Msg) msg);
}


/*
 * MUIM_GetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and gets <attr> to <storage> for itself
 * in this case.
 */
static IPTR Application__MUIM_GetUData(struct IClass *cl, Object *obj,
    struct MUIP_GetUData *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
        get(obj, msg->attr, msg->storage);
        return TRUE;
    }
    return DoMethodA(data->app_WindowFamily, (Msg) msg);
}


/*
 * MUIM_SetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 */
static IPTR Application__MUIM_SetUData(struct IClass *cl, Object *obj,
    struct MUIP_SetUData *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
        set(obj, msg->attr, msg->val);

    DoMethodA(data->app_WindowFamily, (Msg) msg);
    return TRUE;
}


/*
 * MUIM_SetUDataOnce : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 */
static IPTR Application__MUIM_SetUDataOnce(struct IClass *cl, Object *obj,
    struct MUIP_SetUDataOnce *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
        set(obj, msg->attr, msg->val);
        return TRUE;
    }
    return DoMethodA(data->app_WindowFamily, (Msg) msg);
}


/****** Application.mui/MUIM_Application_AboutMUI ****************************
*
*   NAME
*       MUIM_Application_AboutMUI (V14)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_Application_AboutMUI, Object refwindow);
*
*   FUNCTION
*       Show Zune's About window.
*
*   INPUTS
*       refwindow - the window object relative to which the About window will
*           be placed.
*
*   SEE ALSO
*       MUIA_Window_RefWindow.
*
******************************************************************************
*
*/

static IPTR Application__MUIM_AboutMUI(struct IClass *cl, Object *obj,
    struct MUIP_Application_AboutMUI *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (!data->app_AboutWin)
    {
        data->app_AboutWin = AboutmuiObject,
            msg->refwindow ? MUIA_Window_RefWindow : TAG_IGNORE,
            msg->refwindow, MUIA_Window_LeftEdge,
            MUIV_Window_LeftEdge_Centered, MUIA_Window_TopEdge,
            MUIV_Window_TopEdge_Centered, MUIA_Aboutmui_Application, obj,
            End;
    }

    if (data->app_AboutWin)
        set(data->app_AboutWin, MUIA_Window_Open, TRUE);

    return 0;
}

static IPTR Application__MUIM_SetConfigdata(struct IClass *cl, Object *obj,
    struct MUIP_Application_SetConfigdata *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct MinList *children = NULL;
    Object *cstate;
    Object *child;

    get(data->app_WindowFamily, MUIA_Family_List, &children);
    if (children)
    {
        cstate = (Object *) children->mlh_Head;
        if ((child = NextObject(&cstate)))
        {
            D(bug("closing window %p\n", child));

            set(child, MUIA_Window_Open, FALSE);

        }
    }
    if (data->app_GlobalInfo.mgi_Configdata)
        MUI_DisposeObject(data->app_GlobalInfo.mgi_Configdata);
    data->app_GlobalInfo.mgi_Configdata = msg->configdata;
    get(data->app_GlobalInfo.mgi_Configdata, MUIA_Configdata_ZunePrefs,
        &data->app_GlobalInfo.mgi_Prefs);

    DoMethod(obj, MUIM_Application_PushMethod, (IPTR) obj, 1,
        MUIM_Application_OpenWindows);
    return 0;
}


/* MUIM_Application_OpenWindows
 * Opens all windows of an application
 */
static IPTR Application__MUIM_OpenWindows(struct IClass *cl, Object *obj,
    struct MUIP_Application_OpenWindows *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct MinList *children = NULL;
    Object *cstate;
    Object *child;

    get(data->app_WindowFamily, MUIA_Family_List, &children);
    if (!children)
        return 0;

    cstate = (Object *) children->mlh_Head;
    if ((child = NextObject(&cstate)))
    {
        set(child, MUIA_Window_Open, TRUE);
    }
    return 0;
}


static IPTR Application__MUIM_OpenConfigWindow(struct IClass *cl,
    Object *obj, struct MUIP_Application_OpenConfigWindow *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct TagItem tags[] = {
        {SYS_Asynch, FALSE},
        {SYS_Input, 0},
        {SYS_Output, 0},
        {NP_StackSize, AROS_STACKSIZE},
        {TAG_DONE}
    };
    char cmd[255];

    snprintf(cmd, 255, "sys:prefs/Zune %s %ld",
        data->app_Base ? data->app_Base : (STRPTR) "", (long)obj);

    if (SystemTagList(cmd, tags) == -1)
    {
        return 0;
    }
    Delay(50);

    if (data->app_Base)
    {
        snprintf(cmd, 255, "ENV:zune/%s.prefs", data->app_Base);
        DoMethod(data->app_GlobalInfo.mgi_Configdata, MUIM_Configdata_Load,
            (IPTR) cmd);
    }

    return 1;
}

static IPTR Application__MUIM_Execute(Class *CLASS, Object *self,
    Msg message)
{
    IPTR signals = 0L;

    while
        (DoMethod(self, MUIM_Application_NewInput, (IPTR) & signals)
        != MUIV_Application_ReturnID_Quit)
    {
        if (signals)
        {
            signals = Wait(signals | SIGBREAKF_CTRL_C);
            if (signals & SIGBREAKF_CTRL_C)
                break;
        }
    }

    return 0;
}


static IPTR Application__MUIM_UpdateMenus(struct IClass *cl, Object *obj,
    Msg message)
{
    struct List *wlist = NULL;
    APTR wstate;
    Object *curwin;

    get(obj, MUIA_Application_WindowList, &wlist);

    if (wlist)
    {
        wstate = wlist->lh_Head;
        while ((curwin = NextObject(&wstate)))
        {
            DoMethod(curwin, MUIM_Window_UpdateMenu);
        }
    }

    return 0;
}

static IPTR Application__MUIM_Load(struct IClass *cl, Object *obj,
    struct MUIP_Application_Load *message)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct IFFHandle *iff;
    char name[1024];
    BPTR fh;
    Object *dataspace;
    struct MinList *children = NULL;
    Object *cstate;
    Object *child;

    if (!data->app_Base)
        return 0;

    dataspace = MUI_NewObject(MUIC_Dataspace, TAG_DONE);
    if (!dataspace)
        return 0;

    if (message->name == MUIV_Application_Load_ENV)
        snprintf(name, sizeof(name), "ENV:Zune/%s.cfg", data->app_Base);
    else if (message->name == MUIV_Application_Load_ENVARC)
        snprintf(name, sizeof(name), "ENVARC:Zune/%s.cfg", data->app_Base);
    else
        strncpy(name, message->name, sizeof(name));

    fh = Open(name, MODE_OLDFILE);
    if (fh)
    {
        if ((iff = AllocIFF()))
        {
            iff->iff_Stream = (IPTR) fh;

            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_READ))
            {
                if (!StopChunk(iff, ID_PREF, ID_MUIO))
                {
                    if (!ParseIFF(iff, IFFPARSE_SCAN))
                    {
                        DoMethod(dataspace, MUIM_Dataspace_ReadIFF, iff,
                            ID_PREF, ID_MUIO);
                    }
                }

                CloseIFF(iff);
            }
            FreeIFF(iff);
        }
        Close(fh);
    }

    get(data->app_WindowFamily, MUIA_Family_List, &children);
    cstate = (Object *) children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        DoMethod(child, MUIM_Import, dataspace);
    }

    MUI_DisposeObject(dataspace);

    return 0;
}

static IPTR Application__MUIM_Save(struct IClass *cl, Object *obj,
    struct MUIP_Application_Save *message)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct IFFHandle *iff;
    char name[1024];
    BPTR fh;
    Object *dataspace;
    struct MinList *children = NULL;
    Object *cstate;
    Object *child;

    if (!data->app_Base)
        return 0;

    dataspace = MUI_NewObject(MUIC_Dataspace, TAG_DONE);
    if (!dataspace)
        return 0;

    get(data->app_WindowFamily, MUIA_Family_List, &children);
    cstate = (Object *) children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        DoMethod(child, MUIM_Export, dataspace);
    }

    if (message->name == MUIV_Application_Save_ENV)
        snprintf(name, sizeof(name), "ENV:Zune/%s.cfg", data->app_Base);
    else if (message->name == MUIV_Application_Save_ENVARC)
        snprintf(name, sizeof(name), "ENVARC:Zune/%s.cfg", data->app_Base);
    else
        strncpy(name, message->name, sizeof(name));

    fh = Open(name, MODE_NEWFILE);
    if (fh)
    {
        if ((iff = AllocIFF()))
        {
            iff->iff_Stream = (IPTR) fh;

            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_WRITE))
            {
                if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
                {
                    if (!PushChunk(iff, ID_PREF, ID_PRHD,
                            sizeof(struct FilePrefHeader)))
                    {
                        struct FilePrefHeader head;

                        head.ph_Version = PHV_CURRENT;
                        head.ph_Type = 0;
                        head.ph_Flags[0] =
                            head.ph_Flags[1] =
                            head.ph_Flags[2] = head.ph_Flags[3] = 0;

                        if (WriteChunkBytes(iff, &head,
                                sizeof(head)) == sizeof(head))
                        {
                            PopChunk(iff);
                            DoMethod(dataspace, MUIM_Dataspace_WriteIFF,
                                iff, ID_PREF, ID_MUIO);
                        }
                        else
                        {
                            PopChunk(iff);
                        }
                    }
                    PopChunk(iff);
                }
                CloseIFF(iff);
            }
            FreeIFF(iff);
        }
        Close(fh);
    }

    MUI_DisposeObject(dataspace);

    return 0;
}

/*
 * The class dispatcher
 */
BOOPSI_DISPATCHER(IPTR, Application_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Application__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Application__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Application__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Application__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_ADDMEMBER:
        return Application__OM_ADDMEMBER(cl, obj, (APTR) msg);
    case OM_REMMEMBER:
        return Application__OM_REMMEMBER(cl, obj, (APTR) msg);
    case MUIM_Application_AddInputHandler:
        return Application__MUIM_AddInputHandler(cl, obj, (APTR) msg);
    case MUIM_Application_RemInputHandler:
        return Application__MUIM_RemInputHandler(cl, obj, (APTR) msg);
    case MUIM_Application_Input:
        return Application__MUIM_Input(cl, obj, (APTR) msg);
    case MUIM_Application_InputBuffered:
        return Application__MUIM_InputBuffered(cl, obj, (APTR) msg);
    case MUIM_Application_NewInput:
        return Application__MUIM_NewInput(cl, obj, (APTR) msg);
    case MUIM_Application_PushMethod:
        return Application__MUIM_PushMethod(cl, obj, (APTR) msg);
    case MUIM_Application_UnpushMethod:
        return Application__MUIM_UnpushMethod(cl, obj, (APTR) msg);
    case MUIM_Application_ReturnID:
        return Application__MUIM_ReturnID(cl, obj, (APTR) msg);
    case MUIM_FindUData:
        return Application__MUIM_FindUData(cl, obj, (APTR) msg);
    case MUIM_GetUData:
        return Application__MUIM_GetUData(cl, obj, (APTR) msg);
    case MUIM_SetUData:
        return Application__MUIM_SetUData(cl, obj, (APTR) msg);
    case MUIM_SetUDataOnce:
        return Application__MUIM_SetUDataOnce(cl, obj, (APTR) msg);
    case MUIM_Application_AboutMUI:
        return Application__MUIM_AboutMUI(cl, obj, (APTR) msg);
    case MUIM_Application_SetConfigdata:
        return Application__MUIM_SetConfigdata(cl, obj, (APTR) msg);
    case MUIM_Application_OpenWindows:
        return Application__MUIM_OpenWindows(cl, obj, (APTR) msg);
    case MUIM_Application_OpenConfigWindow:
        return Application__MUIM_OpenConfigWindow(cl, obj, (APTR) msg);
    case MUIM_Application_Execute:
        return Application__MUIM_Execute(cl, obj, msg);
    case MUIM_Application_UpdateMenus:
        return Application__MUIM_UpdateMenus(cl, obj, msg);
    case MUIM_Application_Load:
        return Application__MUIM_Load(cl, obj, (APTR) msg);
    case MUIM_Application_Save:
        return Application__MUIM_Save(cl, obj, (APTR) msg);
    }

    return (DoSuperMethodA(cl, obj, msg));
}
BOOPSI_DISPATCHER_END
/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Application_desc =
{
    MUIC_Application,
    MUIC_Notify,
    sizeof(struct MUI_ApplicationData),
    (void *) Application_Dispatcher
};

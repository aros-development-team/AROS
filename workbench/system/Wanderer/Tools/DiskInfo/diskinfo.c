/*
    Copyright © 2005-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#include <dos/filehandler.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>

#include <stdio.h>

#include <zune/iconimage.h>
#include "diskinfo.h"
#include "locale.h"
#include "support.h"

#define MINUSER 1
#define NOVICE 0
#define AVERAGE 1
#define ADVANCED 1
#define EXPERT 2

#ifndef ID_FAT12_DISK
#define ID_FAT12_DISK      (0x46415400L)
#define ID_FAT16_DISK      (0x46415401L)
#define ID_FAT32_DISK      (0x46415402L)
#endif
#ifndef ID_CDFS_DISK
#define ID_CDFS_DISK       (0x43444653L)
#endif

static LONG dt[]={ID_NO_DISK_PRESENT, ID_UNREADABLE_DISK,
    ID_DOS_DISK, ID_FFS_DISK, ID_INTER_DOS_DISK, ID_INTER_FFS_DISK,
    ID_FASTDIR_DOS_DISK, ID_FASTDIR_FFS_DISK, ID_NOT_REALLY_DOS,
    ID_KICKSTART_DISK, ID_MSDOS_DISK, ID_SFS_BE_DISK, ID_SFS_LE_DISK,
    ID_FAT12_DISK, ID_FAT16_DISK, ID_FAT32_DISK, ID_CDFS_DISK };

/*** Instance data **********************************************************/
struct DiskInfo_DATA
{
    Object            *dki_Window;
    Object            *dki_VolumeIcon;
    Object            *dki_VolumeName;
    Object            *dki_VolumeUseGauge;
    Object            *dki_VolumeUsed;
    Object            *dki_VolumeFree;
    char            *dki_DOSDev;
    struct MsgPort              *dki_NotifyPort;
    LONG            dki_DiskType;
    LONG            dki_Aspect;
    struct MUI_InputHandlerNode dki_NotifyIHN;
    struct NotifyRequest        dki_FSNotifyRequest;
};

/*** Methods ****************************************************************/
Object *DiskInfo__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct DiskInfo_DATA    *data           = NULL;
    const struct TagItem    *tstate         = message->ops_AttrList;
    struct TagItem        *tag            = NULL;
    BPTR                        initial         = BNULL;
    Object            *window,
                *volnameobj, *voliconobj, *volusegaugeobj, *volusedobj, *volfreeobj,
                *grp, *grpformat;
    ULONG                       percent        = 0;
    LONG                        disktype       = ID_NO_DISK_PRESENT;
    LONG                        aspect         = 0;
    TEXT                        volname[108];
    TEXT                        size[64];
    TEXT                        used[64];
    TEXT                        free[64];
    TEXT                        blocksize[16];
    STRPTR                      status = NULL;
    STRPTR            dosdevname = "";
    STRPTR            filesystem = NULL;
    STRPTR                      volicon = NULL;
    STRPTR            handlertype = "";
    STRPTR            deviceinfo = "";

    BOOL                        disktypefound = FALSE;

    static struct InfoData id;

    static STRPTR disktypelist[] = 
    { 
    "No Disk",
    "Unreadable",
    "OFS",
    "FFS",
    "OFS-Intl",
    "FFS-Intl",
    "OFS-DC",
    "FFS-DC",
    "Not DOS",
    "KickStart",
    "MSDOS",
    "SFS0 BE",
    "SFS0 LE",
    "FAT12",
    "FAT16",
    "FAT32",
    "CD-ROM"
    };

    /* Parse initial taglist -----------------------------------------------*/
    D(bug("[DiskInfo] %s()\n", __PRETTY_FUNCTION__));

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_DiskInfo_Initial:
                initial = (BPTR) tag->ti_Data;
                D(bug("[DiskInfo] %s: initial lock @ 0x%p\n", __PRETTY_FUNCTION__, initial));
                break;
/* TODO: Remove MUIA_DiskInfo_Aspect */
            case MUIA_DiskInfo_Aspect:
                aspect = tag->ti_Data;
                D(bug("[DiskInfo] %s: aspect: %d\n", __PRETTY_FUNCTION__, aspect));
                break;
        }
    }
    
    /* Initial lock is required */
    if (initial == BNULL)
    {
        return NULL;
    }

    /* obtain volume's name from the lock */
    if (!NameFromLock(initial, volname, sizeof(volname))) {
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return NULL;
    }
    int volname_len = strlen(volname);
    if ((volicon = AllocVec(volname_len + 5, MEMF_CLEAR)) == NULL)
    {
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return NULL;
    }
    strcpy(volicon, volname);
    strcat(volicon, "disk");
    volname[strlen(volname)-1] = '\0';
    D(bug("[DiskInfo] %s: Volume '%s'\n", __PRETTY_FUNCTION__, volname));

    /* find the volumes doslist information .. */
    filesystem = _(MSG_UNKNOWN);

    volname[strlen(volname)] = ':';

    /* Extract volume info from InfoData */
    if (Info(initial, &id) == DOSTRUE)
    {
        if (!disktypefound) /* Workaround for FFS-Intl having 0 as dol_DiskType */
        {
            LONG i;
            filesystem = _(MSG_UNKNOWN);
            disktype = id.id_DiskType;

            for (i = 0; i < sizeof(dt) / sizeof(LONG); ++i)
            {
                if (disktype == dt[i])
                {
                    filesystem = disktypelist[i];
                    break;
                }
            }
        }

        FormatSize(size, id.id_NumBlocks, id.id_NumBlocks, id.id_BytesPerBlock, FALSE);
        percent = FormatSize(used, id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
        FormatSize(free, id.id_NumBlocks - id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
        sprintf(blocksize, "%d %s", (int)id.id_BytesPerBlock, _(MSG_BYTES));

        switch (id.id_DiskState)
    {
        case (ID_WRITE_PROTECTED):
        status = _(MSG_READABLE);
        break;
        case (ID_VALIDATING):
        status = _(MSG_VALIDATING);
        break;
        case (ID_VALIDATED):
        status = _(MSG_READABLE_WRITABLE);
        break;
        default:
        status = _(MSG_UNKNOWN);
    }
    }

    /* Create application and window objects -------------------------------*/
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_Application_Title, __(MSG_TITLE),
        MUIA_Application_Version, (IPTR) "$VER: DiskInfo 0.5 ("ADATE") ©2006-2009 AROS Dev Team",
        MUIA_Application_Copyright, __(MSG_COPYRIGHT),
        MUIA_Application_Author, __(MSG_AUTHOR),
        MUIA_Application_Description, __(MSG_DESCRIPTION),
        MUIA_Application_Base, (IPTR) "DISKINFO",
        SubWindow, (IPTR) (window = (Object *)WindowObject,
            MUIA_Window_Title, (IPTR) volname,
            MUIA_Window_Activate,    TRUE,
            MUIA_Window_NoMenus,     TRUE,
            MUIA_Window_CloseGadget, TRUE,

            WindowContents, (IPTR) VGroup,
                Child, (IPTR) HGroup,
            Child, (IPTR) VGroup,
            Child, (IPTR) HVSpace,
            Child, (IPTR) HGroup,
                Child, (IPTR) HVSpace,
                Child, (IPTR)(voliconobj = (Object *)IconImageObject,
                MUIA_InputMode, MUIV_InputMode_Toggle,
                MUIA_IconImage_File, (IPTR) volicon,
                End),
                Child, (IPTR) HVSpace,
            End,
            Child, (IPTR) HVSpace,
            End,
                    Child, (IPTR) (grp = (Object *)VGroup,
            Child, (IPTR) HVSpace,
                        Child, (IPTR) ColGroup(2),
/* TODO: Build this list only when data is realy available, and localise */
                            Child, (IPTR) TextObject, 
                                MUIA_Text_PreParse, (IPTR) "\33r",
                                MUIA_Text_Contents, (IPTR) __(MSG_DOSDEVICE),
                            End,
                Child, (IPTR) TextObject, 
                MUIA_Text_PreParse, (IPTR) "\33l",
                MUIA_Text_Contents, (IPTR) dosdevname,
                End,
                            Child, (IPTR) TextObject, 
                                MUIA_Text_PreParse, (IPTR) "\33r",
                                MUIA_Text_Contents, (IPTR) __(MSG_DEVICEINFO),
                            End,
                Child, (IPTR) TextObject, 
                MUIA_Text_PreParse, (IPTR) "\33l",
                MUIA_Text_Contents, (IPTR) deviceinfo,
                End,
                            Child, (IPTR) TextObject, 
                                MUIA_Text_PreParse, (IPTR) "\33r",
                                MUIA_Text_Contents, (IPTR) __(MSG_FILESYSTEM),
                            End,
                Child, (IPTR) TextObject, 
                MUIA_Text_PreParse, (IPTR) "\33I[6:24] \33l",
                MUIA_Text_Contents, (IPTR) filesystem,
                End,
                            Child, (IPTR) HVSpace,
                Child, (IPTR) TextObject, 
                MUIA_Text_PreParse, (IPTR) "\33l",
                MUIA_Text_Contents, (IPTR) handlertype,
                End,
            End,
            Child, (IPTR) HVSpace,
                    End),
            Child, (IPTR) HVSpace,
                End,
                Child, (IPTR) VGroup,
            Child, (IPTR) HGroup,
            MUIA_Weight, 100,
            GroupFrame,
            Child, (IPTR) HVSpace,
            Child, (IPTR) ColGroup(2),
                Child, (IPTR) TextObject, 
                MUIA_Text_PreParse, (IPTR) "\33r",
                MUIA_Text_Contents, __(MSG_NAME),
                End,
                Child, (IPTR)(volnameobj = (Object *)TextObject, TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_Text_PreParse, (IPTR) "\33b\33l",
                MUIA_Text_Contents, (IPTR) volname,
                End),
                Child, (IPTR) VGroup,
                Child, (IPTR) TextObject, TextFrame,
                    MUIA_FramePhantomHoriz, (IPTR)TRUE,
                    MUIA_Text_PreParse, (IPTR) "\33r",
                    MUIA_Text_Contents, __(MSG_SIZE),
                End,
                Child, (IPTR) TextObject, TextFrame,
                    MUIA_FramePhantomHoriz, (IPTR)TRUE,
                    MUIA_Text_PreParse, (IPTR) "\33r",
                    MUIA_Text_Contents, __(MSG_USED),
                End,
                Child, (IPTR) TextObject, TextFrame,
                    MUIA_FramePhantomHoriz, (IPTR)TRUE,
                    MUIA_Text_PreParse, (IPTR) "\33r",
                    MUIA_Text_Contents, __(MSG_FREE),
                End,
                End,
                Child, (IPTR) HGroup,
                Child, (IPTR) VGroup,
                    Child, (IPTR) TextObject, TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR) "\33l",
                    MUIA_Text_Contents, (IPTR) size,
                    End,
                    Child, (IPTR)(volusedobj = (Object *)TextObject, TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR) "\33l",
                    MUIA_Text_Contents, (IPTR) used,
                    End),
                    Child, (IPTR)(volfreeobj = (Object *)TextObject, TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR) "\33l",
                    MUIA_Text_Contents, (IPTR) free,
                    End),
                End,
                Child, (IPTR)(volusegaugeobj = (Object *)GaugeObject, GaugeFrame,
                    MUIA_Gauge_InfoText, (IPTR) "",
                    MUIA_Gauge_Horiz, FALSE,
                    MUIA_Gauge_Current, percent,
                End),
                End,
                Child, (IPTR) TextObject,
                MUIA_Text_PreParse, (IPTR) "\33r",
                MUIA_Text_Contents, __(MSG_BLOCK_SIZE),
                End,
                Child, (IPTR) TextObject, TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_Text_PreParse, (IPTR) "\33l",
                MUIA_Text_Contents, (IPTR) blocksize,
                End,
                Child, (IPTR) TextObject,
                MUIA_Text_PreParse, (IPTR) "\33r",
                MUIA_Text_Contents, __(MSG_STATUS),
                End,
                Child, (IPTR) TextObject, TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_Text_PreParse, (IPTR) "\33l",
                MUIA_Text_Contents, (IPTR) status,
                End,
                Child, (IPTR) HVSpace,
                Child, (IPTR) HVSpace,
            End,
            Child, (IPTR) HVSpace,
            End,
            Child, (IPTR) (grpformat = (Object *)HGroup,
            // grpformat object userlevel sensitive
            Child, (IPTR) HVSpace,
            End),
            Child, (IPTR) HVSpace,
                End,
            End,
        End),
        TAG_DONE);

    /* Check if object creation succeeded */
    if (self == NULL)
        return NULL;

    /* Store instance data -------------------------------------------------*/
    data = INST_DATA(CLASS, self);

    data->dki_NotifyPort = CreateMsgPort();

    data->dki_Window        = window;

    data->dki_VolumeName    = volnameobj;
    data->dki_VolumeIcon    = voliconobj;
    data->dki_VolumeUseGauge    = volusegaugeobj;
    data->dki_VolumeUsed    = volusedobj;
    data->dki_VolumeFree    = volfreeobj;

    data->dki_DOSDev        = AllocVec(strlen(dosdevname) +2, MEMF_CLEAR);
    sprintf(data->dki_DOSDev, "%s:", dosdevname);

    data->dki_Aspect        = aspect;

    /* Setup notifications -------------------------------------------------*/
    DoMethod( window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR) self, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    if (data->dki_NotifyPort)
    {
    /* Setup filesystem notification handler ---------------------------*/
    data->dki_NotifyIHN.ihn_Signals = 1UL << data->dki_NotifyPort->mp_SigBit;
    data->dki_NotifyIHN.ihn_Object  = self;
    data->dki_NotifyIHN.ihn_Method  = MUIM_DiskInfo_HandleNotify;

    DoMethod(self, MUIM_Application_AddInputHandler, (IPTR)&data->dki_NotifyIHN);

    data->dki_FSNotifyRequest.nr_Name                 = volname;
    data->dki_FSNotifyRequest.nr_Flags                = NRF_SEND_MESSAGE;
    data->dki_FSNotifyRequest.nr_stuff.nr_Msg.nr_Port = data->dki_NotifyPort;
    if (StartNotify(&data->dki_FSNotifyRequest))
    {
        D(bug("[DiskInfo] %s: FileSystem-Notification setup for '%s'\n", __PRETTY_FUNCTION__, data->dki_FSNotifyRequest.nr_Name));
    }
    else
    {
        D(bug("[DiskInfo] %s: FAILED to setup FileSystem-Notification for '%s'\n", __PRETTY_FUNCTION__, data->dki_FSNotifyRequest.nr_Name));
        DoMethod(self, MUIM_Application_RemInputHandler, (IPTR)&data->dki_NotifyIHN);
        DeleteMsgPort(data->dki_NotifyPort);
        data->dki_NotifyPort = NULL;
    }
    }
    return self;
}

IPTR DiskInfo__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);

    D(bug("[DiskInfo] %s()\n", __PRETTY_FUNCTION__));

    if (data->dki_NotifyPort)
    {
        DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->dki_NotifyIHN);

        EndNotify(&data->dki_FSNotifyRequest);

        DeleteMsgPort(data->dki_NotifyPort);
    }

    if (data->dki_DOSDev) FreeVec(data->dki_DOSDev);

    return DoSuperMethodA(CLASS, self, message);
}

IPTR DiskInfo__MUIM_Application_Execute(Class *CLASS, Object *self, Msg message)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);

    D(bug("[DiskInfo] %s()\n", __PRETTY_FUNCTION__));

    SET(data->dki_Window, MUIA_Window_Open, TRUE);

    DoSuperMethodA(CLASS, self, message);

    SET(data->dki_Window, MUIA_Window_Open, FALSE);

    return (IPTR) NULL;
}

IPTR DiskInfo__MUIM_DiskInfo_HandleNotify
(
    Class *CLASS, Object *self, Msg message
)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);
    struct NotifyMessage *npMessage = NULL;
    static struct InfoData id;
    BPTR fsdevlock = BNULL;
    BOOL di_Quit = FALSE;

    D(bug("[DiskInfo] %s()\n", __PRETTY_FUNCTION__));

    if (data->dki_NotifyPort)
    {
    while ((npMessage = (struct NotifyMessage *)GetMsg(data->dki_NotifyPort)) != NULL)
    {
        D(bug("[DiskInfo] %s: FS notification received\n", __PRETTY_FUNCTION__));

        if ((fsdevlock = Lock(data->dki_DOSDev, SHARED_LOCK)) != BNULL)
        {
        /* Extract volume info from InfoData */
        if (Info(fsdevlock, &id) == DOSTRUE)
        {
            if (id.id_DiskType != ID_NO_DISK_PRESENT)
            {
            ULONG                       percent;
            TEXT                        used[64];
            TEXT                        free[64];
            //TEXT                        blocksize[16];

            D(bug("[DiskInfo] %s: Updating Window from DOS Device '%s'\n", __PRETTY_FUNCTION__, data->dki_DOSDev));

            //FormatSize(size, id.id_NumBlocks, id.id_NumBlocks, id.id_BytesPerBlock, FALSE);
            percent = FormatSize(used, id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
            FormatSize(free, id.id_NumBlocks - id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
            //sprintf(blocksize, "%d %s", id.id_BytesPerBlock, _(MSG_BYTES));

            //data->dki_VolumeName    = volnameobj;
            SET(data->dki_VolumeUsed, MUIA_Text_Contents, used);
            SET(data->dki_VolumeFree, MUIA_Text_Contents, free);
            SET(data->dki_VolumeUseGauge, MUIA_Gauge_Current, percent);
            }
            else
            {
            D(bug("[DiskInfo] %s: Volume no longer available on DOS Device '%s'\n", __PRETTY_FUNCTION__, data->dki_DOSDev));
            di_Quit = TRUE;
            }
        }
        else
        {
            D(bug("[DiskInfo] %s: Failed to obtain Info for DOS Device '%s'\n", __PRETTY_FUNCTION__, data->dki_DOSDev));
            di_Quit = TRUE;
        }

        UnLock(fsdevlock);
        }
        else
        {
        D(bug("[DiskInfo] %s: Failed to lock DOS Device '%s'\n", __PRETTY_FUNCTION__, data->dki_DOSDev));
        di_Quit = TRUE;
        }
        ReplyMsg((struct Message *)npMessage);
    }
    }
    if (di_Quit)
    {
/* TODO: set MUIV_Application_ReturnID_Quit */
    }
    return (IPTR)NULL;
}

IPTR DiskInfo__OM_GET(Class *CLASS, Object *self, struct opGet *msg)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);
    IPTR retval = TRUE;

    D(bug("[DiskInfo] %s()\n", __PRETTY_FUNCTION__));

    switch(msg->opg_AttrID)
    {
        case MUIA_DiskInfo_Volname:
            retval = (IPTR)XGET(data->dki_VolumeName, MUIA_Text_Contents);
            break;
        case MUIA_DiskInfo_Percent:
            retval = (ULONG)XGET(data->dki_VolumeUseGauge, MUIA_Gauge_Current);
            break;
        case MUIA_DiskInfo_Aspect:
            retval = (ULONG) data->dki_Aspect;
            break;
        default:
            retval = DoSuperMethodA(CLASS, self, (Msg)msg);
            break;
    }
    return retval;
}

ULONG DiskInfo__OM_SET(Class *CLASS, Object *self, struct opSet *msg)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);
    const struct TagItem *tags = msg->ops_AttrList;
    struct TagItem       *tag;

    D(bug("[DiskInfo] %s()\n", __PRETTY_FUNCTION__));

    while ((tag = NextTagItem((const struct TagItem **)&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_DiskInfo_Aspect:
                data->dki_Aspect = tag->ti_Data;
                break;
        }
    }
    return DoSuperMethodA(CLASS, self, (Msg)msg);
}

/*** Dispatcher *************************************************************/
BOOPSI_DISPATCHER(IPTR, DiskInfo_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW:                return (IPTR) DiskInfo__OM_NEW(CLASS, self, (struct opSet *) message);
        case OM_DISPOSE:            return DiskInfo__OM_DISPOSE(CLASS, self, message);
        case OM_GET:                return (IPTR) DiskInfo__OM_GET(CLASS, self, (struct opGet *)message);
        case OM_SET:                return (IPTR) DiskInfo__OM_SET(CLASS, self, (struct opSet *)message);
    case MUIM_DiskInfo_HandleNotify:    return DiskInfo__MUIM_DiskInfo_HandleNotify(CLASS, self, message);
        case MUIM_Application_Execute:        return DiskInfo__MUIM_Application_Execute(CLASS, self, message);
        default:                return DoSuperMethodA(CLASS, self, message);
    }
    return 0;
}
BOOPSI_DISPATCHER_END

/*** Setup ******************************************************************/
struct MUI_CustomClass *DiskInfo_CLASS;

BOOL DiskInfo_Initialize()
{
    D(bug("[DiskInfo] %s()\n", __PRETTY_FUNCTION__));

    DiskInfo_CLASS = MUI_CreateCustomClass(
        NULL, MUIC_Application, NULL, 
        sizeof(struct DiskInfo_DATA), DiskInfo_Dispatcher);

    return DiskInfo_CLASS ? TRUE : FALSE;
}

VOID DiskInfo_Deinitialize()
{
    D(bug("[DiskInfo] %s()\n", __PRETTY_FUNCTION__));

    if (DiskInfo_CLASS != NULL)
    {
        MUI_DeleteCustomClass(DiskInfo_CLASS);
    }
}

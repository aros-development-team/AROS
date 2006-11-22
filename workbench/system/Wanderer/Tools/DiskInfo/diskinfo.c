/*
    Copyright © 2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <libraries/asl.h>

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

#ifndef ID_SFS_DISK
#define ID_SFS_DISK AROS_MAKE_ID('S','F','S',0)
#endif

static LONG dt[12]={ID_NO_DISK_PRESENT, ID_UNREADABLE_DISK,
    ID_DOS_DISK, ID_FFS_DISK, ID_INTER_DOS_DISK, ID_INTER_FFS_DISK,
    ID_FASTDIR_DOS_DISK, ID_FASTDIR_FFS_DISK, ID_NOT_REALLY_DOS,
    ID_KICKSTART_DISK, ID_MSDOS_DISK, ID_SFS_DISK};

/*** Instance data **********************************************************/
struct DiskInfo_DATA
{
    Object *dki_Window;
    STRPTR  dki_Volname;
    ULONG   dki_Percent;
    LONG    dki_DiskType;
    LONG    dki_Aspect;
};

/*** Methods ****************************************************************/
Object *DiskInfo__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct DiskInfo_DATA       *data           = NULL;
    const struct TagItem       *tstate         = message->ops_AttrList;
    struct TagItem             *tag            = NULL;
    CONST_STRPTR                initial        = NULL;
    Object                     *window, *grp, *grpformat;
    Object                     *textspace, *levelspace, *typespace;
    ULONG                       percent        = 0;
    LONG                        disktype       = ID_NO_DISK_PRESENT;
    LONG                        aspect         = 0;
    STRPTR                      name           = NULL,
                                s1             = NULL,
                                volname        = NULL;

    static STRPTR disktypelist[12] = {"No Disk", "Unreadable",
    "OFS", "FFS", "OFS-Intl", "FFS-Intl",
    "OFS-DC", "FFS-DC", "Not DOS",
    "KickStart", "MSDOS", "SFS"};

    /* Parse initial taglist -----------------------------------------------*/
    D(bug("[DiskInfo] OM_NEW\n"));
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_DiskInfo_Initial:
                initial = (CONST_STRPTR) tag->ti_Data;
                D(bug("[DiskInfo] initial: %s\n",initial));
                break;
            case MUIA_DiskInfo_Aspect:
                aspect = tag->ti_Data;
                D(bug("[DiskInfo] aspect: %d\n",aspect));
                break;
        }
    }

    // glue from c:info
    // compute the volume name from an absolute path
    s1       = name = (STRPTR)initial;
    while (*s1 != 0)
    {
        if (*s1++ == ':')
        {
            volname = (STRPTR)AllocMem(s1 - name, MEMF_ANY);

            if (volname == NULL)
            {
                SetIoErr(ERROR_NO_FREE_STORE);
                return NULL;
            }

            CopyMem(name, volname, s1 - name - 1);
            volname[s1 - name - 1] = '\0';
            break;
        }
    }

    // extract volume info from InfoData
    BPTR lock;
    UBYTE volnode[20];
    STRPTR dtr = NULL;
    sprintf(volnode, "%s:", volname);
    lock = Lock(volnode, SHARED_LOCK);
    if (lock != NULL)
    {
        static struct InfoData id;
        if(Info(lock, &id) == DOSTRUE)
        {
            percent = (100 * id.id_NumBlocksUsed/id.id_NumBlocks);

            disktype = id.id_DiskType;
            ULONG i;
            for( i = 0; disktype != dt[i]; i++);
            dtr = disktypelist[i];
            D(bug("[DiskInfo] Disk Type: %s\n", dtr));
        }
    }
    /* Create application and window objects -------------------------------*/
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_Application_Title, __(MSG_TITLE),
        MUIA_Application_Version, (IPTR) "$VER: DiskInfo 0.2 ("ADATE") ©2006 AROS Dev Team",
        MUIA_Application_Copyright, __(MSG_COPYRIGHT),
        MUIA_Application_Author, __(MSG_AUTHOR),
        MUIA_Application_Description, __(MSG_DESCRIPTION),
        MUIA_Application_Base, (IPTR) "DISKINFO",
        SubWindow, (IPTR) (window = WindowObject,
            MUIA_Window_Title,(IPTR) volname,
            MUIA_Window_Activate,    TRUE,
            MUIA_Window_NoMenus,     TRUE,
            MUIA_Window_CloseGadget, TRUE,

            WindowContents, (IPTR) VGroup,
                Child, (IPTR) HGroup,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) IconImageObject,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_IconImage_File, (IPTR) initial,
                        End,
                    End,
                    Child, (IPTR) (grp = VGroup,
                        // grp object aspect sensitive
                    End),
                End,
                Child, (IPTR) HGroup,
                    Child, (IPTR) VGroup, GroupFrame,
                        Child, (IPTR) GaugeObject, GaugeFrame,
                            MUIA_Gauge_InfoText, __(MSG_PERCENTFULL),
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, percent,
                        End,
                        Child, (IPTR) ScaleObject, End,
                    End,
                End,
                Child, (IPTR) (grpformat = HGroup,
                    // grpformat object userlevel sensitive
                End),
            End,
        End),
        TAG_DONE);

    /* Check if object creation succeeded */
    if (self == NULL)
        return NULL;

    /* Store instance data -------------------------------------------------*/
    data = INST_DATA(CLASS, self);
    data->dki_Window        = window;
    data->dki_Volname       = volname;
    data->dki_Percent       = percent;
    data->dki_Aspect        = aspect;

    // aspect dependant GUI
    if (aspect > 0)
    {
        textspace = MUI_NewObject(MUIC_Text,
            MUIA_Text_PreParse, (IPTR) "\33I[6:24] \33b\33l",
            MUIA_Text_Contents, (IPTR) volname, TAG_DONE);

        DoMethod(grp, MUIM_Group_InitChange);
        DoMethod(grp, OM_ADDMEMBER, textspace);
        DoMethod(grp, MUIM_Group_ExitChange);
    } else {
        levelspace = MUI_NewObject(MUIC_Levelmeter,MUIA_Numeric_Min, 0,
            MUIA_Numeric_Max, 100,
            MUIA_Numeric_Value, percent, TAG_DONE);

        DoMethod(grp, MUIM_Group_InitChange);
        DoMethod(grp, OM_ADDMEMBER, levelspace);
        DoMethod(grp, MUIM_Group_ExitChange);
    }

    typespace = MUI_NewObject(MUIC_Text,
        MUIA_Text_PreParse, (IPTR) "\33I[6:24] \33b\33l",
        MUIA_Text_Contents, (IPTR) dtr, TAG_DONE);
    DoMethod(grp, MUIM_Group_InitChange);
    DoMethod(grp, OM_ADDMEMBER, typespace);
    DoMethod(grp, MUIM_Group_ExitChange);

    /* Setup notifications -------------------------------------------------*/
    DoMethod( window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR) self, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    if (lock) UnLock(lock);
    FreeMem(volname, s1 - name);
    return self;

}

IPTR DiskInfo__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    D(bug("[DiskInfo] OM_DISPOSE\n"));
    return DoSuperMethodA(CLASS, self, message);
}

IPTR DiskInfo__MUIM_Application_Execute(Class *CLASS, Object *self, Msg message)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);

    SET(data->dki_Window, MUIA_Window_Open, TRUE);

    DoSuperMethodA(CLASS, self, message);

    SET(data->dki_Window, MUIA_Window_Open, FALSE);

    return (IPTR) NULL;
}

IPTR DiskInfo__OM_GET(Class *CLASS, Object *self, struct opGet *msg)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);
    IPTR retval = TRUE;
    D(bug("[DiskInfo] OM_GET\n"));
    switch(msg->opg_AttrID)
    {
        case MUIA_DiskInfo_Volname:
            retval = (IPTR) data->dki_Volname;
            break;
        case MUIA_DiskInfo_Percent:
            retval = (ULONG) data->dki_Percent;
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
    D(bug("[DiskInfo] OM_SET\n"));
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
        case OM_NEW:                    return (IPTR) DiskInfo__OM_NEW(CLASS, self, (struct opSet *) message);
        case OM_DISPOSE:                return DiskInfo__OM_DISPOSE(CLASS, self, message);
        case OM_GET:                    return (IPTR) DiskInfo__OM_GET(CLASS, self, (struct opGet *)message);
        case OM_SET:                    return (IPTR) DiskInfo__OM_SET(CLASS, self, (struct opSet *)message);
        case MUIM_Application_Execute:  return DiskInfo__MUIM_Application_Execute(CLASS, self, message);
        default:                        return DoSuperMethodA(CLASS, self, message);
    }
    return 0;
}
BOOPSI_DISPATCHER_END

/*** Setup ******************************************************************/
struct MUI_CustomClass *DiskInfo_CLASS;

BOOL DiskInfo_Initialize()
{
    D(bug("[DiskInfo] Initialize\n"));
    DiskInfo_CLASS = MUI_CreateCustomClass(
        NULL, MUIC_Application, NULL, 
        sizeof(struct DiskInfo_DATA), DiskInfo_Dispatcher);

    return DiskInfo_CLASS ? TRUE : FALSE;
}

VOID DiskInfo_Deinitialize()
{
    D(bug("[DiskInfo] Deinitialize\n"));
    if (DiskInfo_CLASS != NULL)
    {
        MUI_DeleteCustomClass(DiskInfo_CLASS);
    }
}

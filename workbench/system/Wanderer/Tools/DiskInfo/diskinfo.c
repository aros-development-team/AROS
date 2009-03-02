/*
    Copyright © 2005-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
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

#ifndef ID_FAT12_DISK
#define ID_FAT12_DISK      (0x46415400L)
#define ID_FAT16_DISK      (0x46415401L)
#define ID_FAT32_DISK      (0x46415402L)
#endif
#ifndef ID_CDFS_DISK
#define ID_CDFS_DISK	   (0x43444653L)
#endif

static LONG dt[]={ID_NO_DISK_PRESENT, ID_UNREADABLE_DISK,
    ID_DOS_DISK, ID_FFS_DISK, ID_INTER_DOS_DISK, ID_INTER_FFS_DISK,
    ID_FASTDIR_DOS_DISK, ID_FASTDIR_FFS_DISK, ID_NOT_REALLY_DOS,
    ID_KICKSTART_DISK, ID_MSDOS_DISK, ID_SFS_BE_DISK, ID_SFS_LE_DISK,
    ID_FAT12_DISK, ID_FAT16_DISK, ID_FAT32_DISK, ID_CDFS_DISK };

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
    BPTR                        initial        = NULL;
    Object                     *window, *grp, *grpformat;
    Object                     *textspace, *levelspace, *typespace;
    ULONG                       percent        = 0;
    LONG                        disktype       = ID_NO_DISK_PRESENT;
    LONG                        aspect         = 0;
    TEXT                        volname[108];
    TEXT                        size[64];
    TEXT                        used[64];
    TEXT                        free[64];
    TEXT                        blocksize[16];
    TEXT                        status[64];
    STRPTR			            dtr;
    struct DosList	           *dl, *dn;
    BOOL                        disktypefound = FALSE;

    static struct InfoData id;

    static STRPTR disktypelist[] = {"No Disk", "Unreadable",
    "OFS", "FFS", "OFS-Intl", "FFS-Intl",
    "OFS-DC", "FFS-DC", "Not DOS",
    "KickStart", "MSDOS", "SFS0 BE", "SFS0 LE",
    "FAT12", "FAT16", "FAT32", "CD-ROM" };

    /* Parse initial taglist -----------------------------------------------*/
    D(bug("[DiskInfo] OM_NEW\n"));
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_DiskInfo_Initial:
                initial = (BPTR) tag->ti_Data;
                D(bug("[DiskInfo] initial: 0x%08lX\n",initial));
                break;
            case MUIA_DiskInfo_Aspect:
                aspect = tag->ti_Data;
                D(bug("[DiskInfo] aspect: %d\n",aspect));
                break;
        }
    }
    
    /* Initial lock is required */
    if (initial == NULL)
    {
        return NULL;
    }

    // obtain the volume name from a lock
    if (!NameFromLock(initial, volname, sizeof(volname))) {
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return NULL;
    }
    volname[strlen(volname)-1] = '\0';
    dtr = _(MSG_UNKNOWN);
    dl = LockDosList(LDF_VOLUMES|LDF_READ);
    if (dl) {
	dn = FindDosEntry(dl, volname, LDF_VOLUMES);
	if (dn) {
	    ULONG i;

	    disktype = dn->dol_misc.dol_volume.dol_DiskType;
	    for (i = 0; i < sizeof(dt) / sizeof(LONG); ++i)
	    {
		if (disktype == dt[i])
		{
		    dtr = disktypelist[i];
            disktypefound = TRUE;
		    break;
		}
	    }
            D(bug("[DiskInfo] Disk Type: %s\n", dtr));
	}    
	UnLockDosList(LDF_VOLUMES|LDF_READ);
    }

    /* Extract volume info from InfoData */
    if (Info(initial, &id) == DOSTRUE)
    {
        if (!disktypefound) /* Workaround for FFS-Intl having 0 as dol_DiskType */
        {
            LONG i;
            dtr = _(MSG_UNKNOWN);
            disktype = id.id_DiskType;

            for (i = 0; i < sizeof(dt) / sizeof(LONG); ++i)
            {
                if (disktype == dt[i])
                {
                    dtr = disktypelist[i];
                    break;
                }
            }
        }

        percent = (100 * id.id_NumBlocksUsed/id.id_NumBlocks);
        FormatSize(size, id.id_NumBlocks, id.id_NumBlocks, id.id_BytesPerBlock, FALSE);
        FormatSize(used, id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
        FormatSize(free, id.id_NumBlocks - id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
        sprintf(blocksize, "%d %s", id.id_BytesPerBlock, _(MSG_BYTES));

        sprintf(status,"%s", _(MSG_UNKNOWN));
        if ((id.id_DiskState & ID_WRITE_PROTECTED) == ID_WRITE_PROTECTED)
            sprintf(status,"%s", _(MSG_READABLE));
        if ((id.id_DiskState & ID_VALIDATING) == ID_VALIDATING)
            sprintf(status,"%s", _(MSG_VALIDATING));
        if ((id.id_DiskState & ID_VALIDATED) == ID_VALIDATED)
            sprintf(status,"%s", _(MSG_READABLE_WRITABLE));
            
    }

    /* Create application and window objects -------------------------------*/
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_Application_Title, __(MSG_TITLE),
        MUIA_Application_Version, (IPTR) "$VER: DiskInfo 0.3 ("ADATE") ©2006-2009 AROS Dev Team",
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
                        Child, (IPTR) ColGroup(2),
                            Child, (IPTR) TextObject, 
                                MUIA_Text_PreParse, (IPTR) "\33r",
                                MUIA_Text_Contents, (IPTR) __(MSG_SIZE),
                            End,
			    Child, (IPTR) GaugeObject, GaugeFrame,
				MUIA_Gauge_InfoText, size,
				MUIA_Gauge_Horiz, TRUE,
				MUIA_Gauge_Current, percent,
			    End,
                            Child, (IPTR) TextObject, 
                                MUIA_Text_PreParse, (IPTR) "\33r",
                                MUIA_Text_Contents, (IPTR) __(MSG_USED),
                            End,
                            Child, (IPTR) TextObject, TextFrame,
                                MUIA_Background, MUII_TextBack,
                                MUIA_Text_PreParse, (IPTR) "\33l",
                                MUIA_Text_Contents, (IPTR) used,
                            End,
                            Child, (IPTR) TextObject,
                                MUIA_Text_PreParse, (IPTR) "\33r",
                                MUIA_Text_Contents, (IPTR) __(MSG_FREE),
                            End,
                            Child, (IPTR) TextObject, TextFrame,
                                MUIA_Background, MUII_TextBack,
                                MUIA_Text_PreParse, (IPTR) "\33l",
                                MUIA_Text_Contents, (IPTR) free,
                            End,
                            Child, (IPTR) TextObject,
                                MUIA_Text_PreParse, (IPTR) "\33r",
                                MUIA_Text_Contents, (IPTR) __(MSG_BLOCK_SIZE),
                            End,
                            Child, (IPTR) TextObject, TextFrame,
                                MUIA_Background, MUII_TextBack,
                                MUIA_Text_PreParse, (IPTR) "\33l",
                                MUIA_Text_Contents, (IPTR) blocksize,
                            End,
                            Child, (IPTR) TextObject,
                                MUIA_Text_PreParse, (IPTR) "\33r",
                                MUIA_Text_Contents, (IPTR) __(MSG_STATUS),
                            End,
                            Child, (IPTR) TextObject, TextFrame,
                                MUIA_Background, MUII_TextBack,
                                MUIA_Text_PreParse, (IPTR) "\33l",
                                MUIA_Text_Contents, (IPTR) status,
                            End,
                        End,
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
//    if (aspect > 0)
//    {
        textspace = MUI_NewObject(MUIC_Text,
            MUIA_Text_PreParse, (IPTR) "\33I[6:24] \33b\33l",
            MUIA_Text_Contents, (IPTR) volname, TAG_DONE);

        DoMethod(grp, MUIM_Group_InitChange);
        DoMethod(grp, OM_ADDMEMBER, textspace);
        DoMethod(grp, MUIM_Group_ExitChange);
//    } else {
//        levelspace = MUI_NewObject(MUIC_Levelmeter,MUIA_Numeric_Min, 0,
//            MUIA_Numeric_Max, 100,
//            MUIA_Numeric_Value, percent, TAG_DONE);

//        DoMethod(grp, MUIM_Group_InitChange);
//        DoMethod(grp, OM_ADDMEMBER, levelspace);
//        DoMethod(grp, MUIM_Group_ExitChange);
//    }

    typespace = MUI_NewObject(MUIC_Text,
        MUIA_Text_PreParse, (IPTR) "\33I[6:24] \33b\33l",
        MUIA_Text_Contents, (IPTR) dtr, TAG_DONE);
    DoMethod(grp, MUIM_Group_InitChange);
    DoMethod(grp, OM_ADDMEMBER, typespace);
    DoMethod(grp, MUIM_Group_ExitChange);

    /* Setup notifications -------------------------------------------------*/
    DoMethod( window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR) self, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

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

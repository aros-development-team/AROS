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
#include <dos/filehandler.h>

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
    ULONG                       percent        = 0;
    LONG                        disktype       = ID_NO_DISK_PRESENT;
    LONG                        aspect         = 0;
    TEXT                        size[64];
    TEXT                        used[64];
    TEXT                        free[64];
    TEXT                        blocksize[16];
    STRPTR                      status = NULL;
    STRPTR			dosdevname = NULL;
    STRPTR			filesystem = NULL;
    STRPTR			fstype = NULL;
    STRPTR			fshandler = NULL;
    TEXT                        volname[108];
    STRPTR                      volicon = NULL;
    STRPTR			handlertype = "";
    STRPTR			deviceinfo = "";
 
    struct DosList	        *dl, *dn;
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
    D(bug("[DiskInfo] %s\n", __PRETTY_FUNCTION__));
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_DiskInfo_Initial:
                initial = (BPTR) tag->ti_Data;
                D(bug("[DiskInfo] %s: initial lock @ 0x%p\n", __PRETTY_FUNCTION__, initial));
                break;
#warning "TODO: Remove MUIA_DiskInfo_Aspect"
            case MUIA_DiskInfo_Aspect:
                aspect = tag->ti_Data;
                D(bug("[DiskInfo] %s: aspect: %d\n", __PRETTY_FUNCTION__, aspect));
                break;
        }
    }
    
    /* Initial lock is required */
    if (initial == NULL)
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
    IPTR volunit = NULL;
    filesystem = _(MSG_UNKNOWN);
    dl = LockDosList(LDF_VOLUMES|LDF_READ);
    if (dl) {
	dn = FindDosEntry(dl, volname, LDF_VOLUMES);
	if (dn) {
	    ULONG i;

	    volunit = dn->dol_Ext.dol_AROS.dol_Unit;

	    D(bug("[DiskInfo] %s: Volume's unit @ %p\n", __PRETTY_FUNCTION__, volunit));

	    if (dn->dol_Task != NULL)
	    {
		handlertype = "Packet Style device";
	    }
	    else if (dn->dol_Ext.dol_AROS.dol_Device != NULL)
	    {
		handlertype = "IOFS Style device";
	    }

	    disktype = dn->dol_misc.dol_volume.dol_DiskType;
	    for (i = 0; i < sizeof(dt) / sizeof(LONG); ++i)
	    {
		if (disktype == dt[i])
		{
		    fstype = disktypelist[i];
		    fshandler = dn->dol_Ext.dol_AROS.dol_Device->dd_Library.lib_Node.ln_Name;
		    disktypefound = TRUE;
		    break;
		}
	    }
            D(bug("[DiskInfo] %s: Disk Type: %s\n", __PRETTY_FUNCTION__, filesystem));
	}
	UnLockDosList(LDF_VOLUMES|LDF_READ);
    }
    /* If we know the volumes unit - find its device information .. */
    if (volunit != NULL)
    {
	dl = LockDosList(LDF_DEVICES|LDF_READ);
	if (dl) {
	    while((dl = NextDosEntry(dl, LDF_DEVICES)))
	    {
		if (dl->dol_Ext.dol_AROS.dol_Unit == volunit)
		{
		    struct FileSysStartupMsg *fsstartup = (struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup);
		    dosdevname = (UBYTE*)AROS_BSTR_ADDR(dl->dol_Name);
		    fshandler = (UBYTE*)AROS_BSTR_ADDR(dl->dol_misc.dol_handler.dol_Handler);

		    D(bug("[DiskInfo] %s: Found Volumes device @ %p, '%s'\n", __PRETTY_FUNCTION__, dl, dosdevname));
		    if (fsstartup != NULL)
		    {
			deviceinfo = AllocVec(strlen((UBYTE*)AROS_BSTR_ADDR(fsstartup->fssm_Device)) + (fsstartup->fssm_Unit/10 + 1) + 7, MEMF_CLEAR);
			sprintf(deviceinfo,"%s %s %d", (UBYTE*)AROS_BSTR_ADDR(fsstartup->fssm_Device), _(MSG_UNIT), fsstartup->fssm_Unit);
		    }
		    D(bug("[DiskInfo] %s: Handler '%s'\n", __PRETTY_FUNCTION__, fshandler));
		    break;
		}
	    }
	    UnLockDosList(LDF_VOLUMES|LDF_READ);
	}
    }

    if (fstype && fshandler)
    {
	filesystem = AllocVec(strlen(fstype) + strlen(fshandler) + 4, MEMF_CLEAR);
	sprintf(filesystem, "%s (%s)", fstype, fshandler);
    }

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

        percent = (100 * (id.id_NumBlocksUsed / id.id_NumBlocks));
        FormatSize(size, id.id_NumBlocks, id.id_NumBlocks, id.id_BytesPerBlock, FALSE);
        FormatSize(used, id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
        FormatSize(free, id.id_NumBlocks - id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
        sprintf(blocksize, "%d %s", id.id_BytesPerBlock, _(MSG_BYTES));

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
			Child, HVSpace,
			Child, (IPTR) HGroup,
			    Child, HVSpace,
			    Child, (IPTR) IconImageObject,
				MUIA_InputMode, MUIV_InputMode_Toggle,
				MUIA_IconImage_File, (IPTR) volicon,
			    End,
			    Child, HVSpace,
			End,
			Child, HVSpace,
		    End,
                    Child, (IPTR) (grp = (Object *)VGroup,
			Child, HVSpace,
                        Child, (IPTR) ColGroup(2),
#warning "TODO: Build this list only when data is realy available, and localise"
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
			Child, HVSpace,
                    End),
		    Child, HVSpace,
                End,
                Child, (IPTR) VGroup,
		    Child, (IPTR) HGroup,
			MUIA_Weight, 100,
			GroupFrame,
			Child, HVSpace,
			Child, (IPTR) ColGroup(2),
			    Child, (IPTR) TextObject, 
				MUIA_Text_PreParse, (IPTR) "\33r",
				MUIA_Text_Contents, (IPTR) __(MSG_NAME),
			    End,
			    Child, (IPTR) TextObject, TextFrame,
				MUIA_Background, MUII_TextBack,
				MUIA_Text_PreParse, (IPTR) "\33b\33l",
				MUIA_Text_Contents, (IPTR) volname,
			    End,
			    Child, (IPTR) VGroup,
				Child, (IPTR) TextObject, 
				    MUIA_Text_PreParse, (IPTR) "\33r",
				    MUIA_Text_Contents, (IPTR) __(MSG_SIZE),
				End,
				Child, (IPTR) TextObject, 
				    MUIA_Text_PreParse, (IPTR) "\33r",
				    MUIA_Text_Contents, (IPTR) __(MSG_USED),
				End,
				Child, (IPTR) TextObject,
				    MUIA_Text_PreParse, (IPTR) "\33r",
				    MUIA_Text_Contents, (IPTR) __(MSG_FREE),
				End,
			    End,
			    Child, (IPTR) HGroup,
				Child, (IPTR) VGroup,
				    Child, (IPTR) TextObject, TextFrame,
					MUIA_Background, MUII_TextBack,
					MUIA_Text_PreParse, (IPTR) "\33l",
					MUIA_Text_Contents, (IPTR) size,
				    End,
				    Child, (IPTR) TextObject, TextFrame,
					MUIA_Background, MUII_TextBack,
					MUIA_Text_PreParse, (IPTR) "\33l",
					MUIA_Text_Contents, (IPTR) used,
				    End,
				    Child, (IPTR) TextObject, TextFrame,
					MUIA_Background, MUII_TextBack,
					MUIA_Text_PreParse, (IPTR) "\33l",
					MUIA_Text_Contents, (IPTR) free,
				    End,				
				End,
				Child, (IPTR) GaugeObject, GaugeFrame,
				    MUIA_Width, 4,
				    MUIA_Gauge_InfoText, "",
				    MUIA_Gauge_Horiz, FALSE,
				    MUIA_Gauge_Current, percent,
				End,
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
			    Child, HVSpace,
			    Child, HVSpace,
			End,
			Child, HVSpace,
		    End,
		    Child, (IPTR) (grpformat = HGroup,
			// grpformat object userlevel sensitive
			Child, HVSpace,
		    End),
		    Child, HVSpace,
                End,
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

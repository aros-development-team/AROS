/*
Copyright  2002-2009, The AROS Development Team. All rights reserved.
$Id$
*/

#ifndef __AROS__
#include "../portable_macros.h"
#define WANDERER_BUILTIN_ICONVOLUMELIST 1 
#else
#define DEBUG 0
#include <aros/debug.h>
#endif

#define DEBUG_ILC_EVENTS
#define DEBUG_ILC_KEYEVENTS
#define DEBUG_ILC_ICONRENDERING
#define DEBUG_ILC_ICONSORTING
#define DEBUG_ILC_ICONSORTING_DUMP
#define DEBUG_ILC_ICONPOSITIONING
#define DEBUG_ILC_LASSO
#define DEBUG_ILC_MEMALLOC

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <dos/dos.h>
#include <dos/datetime.h>
#include <dos/filehandler.h>
#include <dos/filesystem.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/rpattr.h>
#include <workbench/icon.h>
#include <workbench/workbench.h>

#ifdef __AROS__
#include <devices/rawkeycodes.h>
#include <clib/alib_protos.h>
#endif

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/layers.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#ifdef __AROS__
#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>
#else
#include <prefs_AROS/prefhdr.h>
#include <prefs_AROS/wanderer.h>
#endif

#include <proto/cybergraphics.h>

#ifdef __AROS__
#include <cybergraphx/cybergraphics.h>
#else
#include <cybergraphx_AROS/cybergraphics.h>
#endif


#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
//#include "muimaster_intern.h"
//#include "support.h"
//#include "imspec.h"
#include "iconlist_attributes.h"
#include "icon_attributes.h"
#include "iconlist.h"
#include "iconvolumelist_private.h"

#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #ifdef __amigaos4__
  #define bug DebugPrintF
  #else
  #define bug kprintf
  #endif
#else
  #define  D(...)
#endif
#endif

#define __DL_UNIT       dl->dol_Ext.dol_AROS.dol_Unit
#define __DL_DEVICE     dl->dol_Ext.dol_AROS.dol_Device

extern struct Library *MUIMasterBase;

struct DOSVolumeList
{
    struct List       		dvl_List;
    APTR              		dvl_Pool;
};

struct DOSVolumeNode
{
    struct Node			dvn_Node;
    STRPTR			dvn_VolName;
    STRPTR			dvn_DevName;
    ULONG			dvn_FLags;
    struct Device		*dvn_Device;
    struct Unit			*dvn_Unit;
    struct MsgPort		*dvn_Port;
};

///IconVolumeList__CreateDOSList()
static struct DOSVolumeList *IconVolumeList__CreateDOSList(void)
{
    APTR                        pool = NULL;
    struct DosList              *dl = NULL;
    struct DOSVolumeNode        *newdvn = NULL;
    struct DOSVolumeList        *newdvl = NULL;

    D(bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__));

    if ((pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 4096, 4096)) != NULL)
    {
	if ((newdvl = (struct DOSVolumeList*)AllocPooled(pool, sizeof(struct DOSVolumeList))) != NULL)
	{
	    NewList((struct List*)&newdvl->dvl_List);
	    newdvl->dvl_Pool = pool;

            /* work around to only start scanning dos list after all */
            /* shared locks are gone, eg. in rom/dos/getdeviceproc.c RunHandler */
	    dl = LockDosList(LDF_VOLUMES|LDF_WRITE);
	    while(( dl = NextDosEntry(dl, LDF_VOLUMES)))
	    {
		STRPTR vn_VolName;

		UBYTE *dosname = (UBYTE*)AROS_BSTR_ADDR(dl->dol_Name);
		LONG len = AROS_BSTR_strlen(dl->dol_Name);

		if ((vn_VolName = (STRPTR)AllocPooled(newdvl->dvl_Pool, len + 2)))
		{
		    newdvn = NULL;

		    vn_VolName[len] = ':';
		    vn_VolName[len + 1] = 0;
		    strncpy(vn_VolName, dosname, len);

		    if ((newdvn = (struct DOSVolumeNode*)AllocPooled(newdvl->dvl_Pool, sizeof(struct DOSVolumeNode))))
		    {
			newdvn->dvn_VolName     = vn_VolName;
                        newdvn->dvn_Device      = __DL_DEVICE;
			if ((newdvn->dvn_Unit = __DL_UNIT) == NULL)
                        {
                            D(bug("[IconVolumeList] %s: Volume '%s' is OFFLINE\n", __PRETTY_FUNCTION__, newdvn->dvn_VolName));
                            newdvn->dvn_FLags |= (ICONENTRY_VOL_OFFLINE|ICONENTRY_VOL_DISABLED);
                        }

                        D(bug("[IconVolumeList] %s: Registering Volume '%s' @ %p (Device '%s' @ 0x%p, Unit @ 0x%p) Type: %d\n", __PRETTY_FUNCTION__, newdvn->dvn_VolName, dl, dl->dol_Ext.dol_AROS.dol_Device->dd_Library.lib_Node.ln_Name, dl->dol_Ext.dol_AROS.dol_Device, newdvn->dvn_Unit, dl->dol_Type));
			if (dl->dol_misc.dol_handler.dol_Startup)
			{
			    struct FileSysStartupMsg *thisfs_SM = dl->dol_misc.dol_handler.dol_Startup;
                            D(bug("[IconVolumeList] %s: Startup msg @ 0x%p\n", __PRETTY_FUNCTION__, thisfs_SM));
                            D(bug("[IconVolumeList] %s: Startup Device @ %p, Unit %d\n", __PRETTY_FUNCTION__, thisfs_SM->fssm_Device, thisfs_SM->fssm_Unit));
			}

			if (dl->dol_Task != NULL)
			{
                            D(bug("[IconVolumeList] %s: Packet Style device\n", __PRETTY_FUNCTION__));
			    newdvn->dvn_Port = dl->dol_Task;
			}
#if defined(__AROS__)
			else if (dl->dol_Ext.dol_AROS.dol_Device != NULL)
			{
                            D(bug("[IconVolumeList] %s: IOFS Style device\n", __PRETTY_FUNCTION__));
			    newdvn->dvn_Port = dl->dol_Ext.dol_AROS.dol_Device;
			}
#endif
			else
			{
                            D(bug("[IconVolumeList] %s: Unknown device type\n", __PRETTY_FUNCTION__));
			}
			AddTail((struct List*)&newdvl->dvl_List, (struct Node*)&newdvn->dvn_Node);
		    }
		}
	    }
            D(bug("[IconVolumeList] Finished registering volumes\n"));
	    UnLockDosList(LDF_VOLUMES|LDF_WRITE);

	    dl = LockDosList(LDF_DEVICES|LDF_READ);
	    while(( dl = NextDosEntry(dl, LDF_DEVICES)))
	    {
		struct DOSVolumeNode 		*dvn = NULL;
		char 			  	*nd_nambuf = NULL;
		struct InfoData 		*nd_paramblock = NULL;

		UBYTE             		*dosname = (UBYTE*)AROS_BSTR_ADDR(dl->dol_Name);
		LONG   				len = AROS_BSTR_strlen(dl->dol_Name);

                D(bug("[IconVolumeList] %s: Checking Device '%s' @ %p (Device ", __PRETTY_FUNCTION__, dosname, dl));
                D(if (dl->dol_Ext.dol_AROS.dol_Device) bug("'%s' ", dl->dol_Ext.dol_AROS.dol_Device->dd_Library.lib_Node.ln_Name));
                D(bug("@ 0x%p, Unit @ 0x%p) Type: %d\n", dl->dol_Ext.dol_AROS.dol_Device, __DL_UNIT, dl->dol_Type));
#if defined(__AROS__)
		if (dl->dol_Ext.dol_AROS.dol_Device == NULL)
#else
		if (dl->dol_Task == NULL)
#endif
		{
                    D(bug("[IconVolumeList] %s: '%s' : handler inactive!\n", __PRETTY_FUNCTION__, dosname));
                    continue;
		}

		if ((nd_nambuf = AllocPooled(newdvl->dvl_Pool, len + 2)) != NULL)
		{
		    strncpy(nd_nambuf, dosname, len);
		    nd_nambuf[len] = ':';
		    nd_nambuf[len + 1] = 0;

                    if ((nd_paramblock = AllocMem(sizeof(struct InfoData), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
                    {
                        struct IOFileSys *_iofs = NULL;
                        if ((_iofs = AllocMem(sizeof(struct IOFileSys), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
                        {
                            struct Process *me = (struct Process *)FindTask(NULL);

                            D(bug("[IconVolumeList] %s: Getting Info for '%s'\n", __PRETTY_FUNCTION__, nd_nambuf));

                            _iofs->IOFS.io_Message.mn_Node.ln_Type  = 0;
                            _iofs->IOFS.io_Message.mn_ReplyPort     = &me->pr_MsgPort;
                            _iofs->IOFS.io_Message.mn_Length        = sizeof(struct IOFileSys);
                            _iofs->IOFS.io_Command                  = FSA_DISK_INFO;

                            _iofs->IOFS.io_Flags                    = IOF_QUICK;

                            _iofs->IOFS.io_Device                   = __DL_DEVICE;
                            _iofs->IOFS.io_Unit                     = __DL_UNIT;

                            _iofs->io_Union.io_INFO.io_Info         = nd_paramblock;

                            DoIO(&_iofs->IOFS);

                            if (_iofs->io_DosError != 0)
                            {
                                D(bug("[IconVolumeList] %s: FSA_DISK_INFO returns error %08x\n", __PRETTY_FUNCTION__, _iofs->io_DosError));
                                FreeMem(nd_paramblock, sizeof(struct InfoData));
                                nd_paramblock = NULL;
                            }
                            FreeMem(_iofs, sizeof(struct IOFileSys));
                        }
                        else
                        {
                            D(bug("[IconVolumeList] %s: Failed to allocate IOFileSys storage\n", __PRETTY_FUNCTION__));

                            FreeMem(nd_paramblock, sizeof(struct InfoData));
                            nd_paramblock = NULL;
                        }
                    }
                    else
                    {
                        D(bug("[IconVolumeList] %s: Failed to allocate InfoData storage\n", __PRETTY_FUNCTION__, nd_nambuf));
                    }

                    D(bug("[IconVolumeList] %s: '%s' : Checking for Attached Volumes ... \n", __PRETTY_FUNCTION__, dosname));
		    /* Find the Volume attached to this device */
		    BOOL found = FALSE;
		    dvn = (struct DOSVolumeNode*)GetHead((struct List*)&newdvl->dvl_List);
		    while ((dvn))
		    {
			if ((dvn->dvn_Port != NULL) &&
			    (
			       (dvn->dvn_Port == dl->dol_Task)
#if defined(__AROS__)
			       || (dvn->dvn_Port == dl->dol_Ext.dol_AROS.dol_Device)
#endif
			    ))
			{
			    if ((dvn->dvn_Unit) && ~(newdvn->dvn_FLags & ICONENTRY_VOL_OFFLINE))
			    {
				if (dvn->dvn_Unit == __DL_UNIT)
				{
				    if ((nd_paramblock) && (nd_paramblock->id_DiskType != ID_NO_DISK_PRESENT))
				    {
                                        D(bug("[IconVolumeList] %s: '%s' : Device unit %d, state = %x, Vol node @ %p\n", __PRETTY_FUNCTION__, nd_nambuf, nd_paramblock->id_UnitNumber, nd_paramblock->id_DiskState, BADDR(nd_paramblock->id_VolumeNode)));

					STRPTR nd_namext;
					int nd_namext_len = 0;

					found = TRUE;
                                        dvn->dvn_FLags &= ~(ICONENTRY_VOL_OFFLINE|ICONENTRY_VOL_DISABLED);

					if (nd_paramblock->id_DiskState == ID_VALIDATING)
					{
                                            D(bug("[IconVolumeList] %s: '%s' : Validating\n", __PRETTY_FUNCTION__, nd_nambuf));
					    nd_namext = "BUSY";
					    nd_namext_len = 4;
					}
					else
					{
					    if (nd_paramblock->id_DiskState == ID_WRITE_PROTECTED)
					    {
                                                D(bug("[IconVolumeList] %s: '%s' : Volume is WRITE-PROTECTED\n", __PRETTY_FUNCTION__, nd_nambuf));
						dvn->dvn_FLags |= ICONENTRY_VOL_READONLY;
					    }
					}

					if (nd_namext_len > 0)
					{
					    char *newVolName = NULL;
					    if ((newVolName = AllocPooled(newdvl->dvl_Pool, strlen(dvn->dvn_VolName) + nd_namext_len + 2)) != NULL)
                                            {
                                                sprintf(newVolName, "%s%s", dvn->dvn_VolName, nd_namext);
                                                dvn->dvn_VolName = newVolName;
                                            }
					}
				    }
				    else
				    {
                                        D(bug("[IconVolumeList] %s: '%s' : No Media Inserted (error state?)\n", __PRETTY_FUNCTION__, nd_nambuf));
				    }
				    dvn->dvn_DevName = nd_nambuf;
                                    D(bug("[IconVolumeList] %s: DeviceName set to '%s' for '%s'\n", __PRETTY_FUNCTION__, dvn->dvn_DevName, dvn->dvn_VolName));
				}
				else
				{
                                    D(bug("[IconVolumeList] %s: '%s' : Volume not attached to this device .. skipping\n", __PRETTY_FUNCTION__, nd_nambuf));
				}
			    }
			}
			dvn = (struct DOSVolumeNode*)GetSucc(dvn);
		    } /* dvn */

		    if (!(found))
		    {
                        D(bug("[IconVolumeList] %s: '%s' : Couldnt find an associated Volume\n", __PRETTY_FUNCTION__, nd_nambuf));
                        if ((nd_paramblock) && (nd_paramblock->id_DiskType != ID_NO_DISK_PRESENT))
                        {
                            if ((newdvn = (struct DOSVolumeNode *)AllocPooled(newdvl->dvl_Pool, sizeof(struct DOSVolumeNode))))
                            {
                                STRPTR nd_namext;
                                int nd_namext_len = 0;

                                newdvn->dvn_Unit        = __DL_UNIT;
                                newdvn->dvn_Device      = __DL_DEVICE;

                                switch (nd_paramblock->id_DiskType)
                                {
                                /*case ID_BUSY_DISK:
                                        nd_namext = "BUSY";
                                        nd_namext_len = 4;
                                        break;*/
                                case ID_UNREADABLE_DISK:
                                        nd_namext = "BAD";
                                        nd_namext_len = 3;
                                        break;
                                case ID_NOT_REALLY_DOS:
                                        nd_namext = "NDOS";
                                        nd_namext_len = 4;
                                        break;
                                case ID_KICKSTART_DISK:
                                        nd_namext = "KICK";
                                        nd_namext_len = 4;
                                        break;
                                }
                                if (nd_namext_len > 0)
                                {
                                    if ((newdvn->dvn_VolName = AllocPooled(newdvl->dvl_Pool, strlen(nd_nambuf) + nd_namext_len + 2)) != NULL)
                                    {
                                        sprintf(newdvn->dvn_VolName, "%s%s", nd_nambuf, nd_namext);
                                        newdvn->dvn_DevName = nd_nambuf;
                                        newdvn->dvn_FLags |= ICONENTRY_VOL_DISABLED;
                                        AddTail((struct List*)&newdvl->dvl_List, (struct Node*)&newdvn->dvn_Node);
                                    }
                                }
                                else
                                {
                                    D(bug("[IconVolumeList] %s: '%s' : Unknown Condition?\n", __PRETTY_FUNCTION__, nd_nambuf));
                                }
                            }
                        }
		    }

		    if (nd_paramblock)
			FreeMem(nd_paramblock, sizeof(struct InfoData));
		}
	    }
	    UnLockDosList(LDF_DEVICES|LDF_READ);

	    return newdvl;
	}
	DeletePool(pool);
    }
    return NULL;
}
///

///IconVolumeList__DestroyDOSList()
static void IconVolumeList__DestroyDOSList(struct DOSVolumeList *dvl)
{
D(bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__));
    if (dvl && dvl->dvl_Pool) DeletePool(dvl->dvl_Pool);
}
///
/* sba: End SimpleFind3 */

///OM_NEW()
/**************************************************************************
OM_NEW
**************************************************************************/
IPTR IconVolumeList__OM_NEW(struct IClass *CLASS, Object *obj, struct opSet *message)
{
  struct IconDrawerList_DATA   *data = NULL;
//    struct TagItem            *tag = NULL,
//                                *tags = NULL;

D(bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__));

    obj = (Object *)DoSuperNewTags(CLASS, obj, NULL,
				    TAG_MORE, (IPTR) message->ops_AttrList);

    if (!obj)
	return FALSE;

    data = INST_DATA(CLASS, obj);

    SET(obj, MUIA_IconList_DisplayFlags, ICONLIST_DISP_VERTICAL);
    SET(obj, MUIA_IconList_SortFlags, ICONLIST_SORT_MASK);

D(bug("[IconVolumeList] obj @ %p\n", obj));
    return (IPTR)obj;
}
///

struct IconEntry *FindIconlistVolumeIcon(struct List *iconlist, char *icondevname)
{
    struct IconEntry *foundEntry = NULL;

    ForeachNode(iconlist, foundEntry)
    {
        if ((foundEntry->ie_IconListEntry.type == ST_ROOT) && (((strcasecmp(foundEntry->ie_IconNode.ln_Name, icondevname)) == 0) ||
            ((strcasecmp(foundEntry->ie_IconListEntry.label, icondevname)) == 0)))
            return foundEntry;
    }
    return NULL;
}

///MUIM_IconList_Update()
/**************************************************************************
MUIM_IconList_Update
**************************************************************************/
IPTR IconVolumeList__MUIM_IconList_Update(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Update *message)
{
    //struct IconVolumeList_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry  		*this_Icon = NULL;
    struct DOSVolumeList 	*dvl = NULL;
    struct DOSVolumeNode	*dvn = NULL;
    char                        *devname = NULL;
    struct List                 *iconlist = NULL;
    struct List                 newiconlist;
    struct Node                 *tmpNode = NULL;

D(bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__));

    GET(obj, MUIA_Family_List, &iconlist);

    if (iconlist != NULL)
    {
        NewList(&newiconlist);

        if ((dvl = IconVolumeList__CreateDOSList()) != NULL)
        {
D(bug("[IconVolumeList] %s: DOSVolumeList @ %p\n", __PRETTY_FUNCTION__, dvl));

#ifdef __AROS__
            ForeachNode(dvl, dvn)
#else
            Foreach_Node(dvl, dvn);
#endif
            {
D(bug("[IconVolumeList] %s: DOSVolumeNode  @ %p\n", __PRETTY_FUNCTION__, dvn));
                if (dvn->dvn_VolName)
                {
D(bug("[IconVolumeList] %s: DOSList Entry '%s'\n", __PRETTY_FUNCTION__, dvn->dvn_VolName));
                    struct DiskObject       *volDOB = NULL;

                    if (dvn->dvn_FLags & ICONENTRY_VOL_OFFLINE)
                        devname = dvn->dvn_VolName;
                    else
                        devname = dvn->dvn_DevName;

D(bug("[IconVolumeList] %s: Processing '%s'\n", __PRETTY_FUNCTION__, devname));

                    if ((this_Icon = FindIconlistVolumeIcon(iconlist, devname)) != NULL)
                    {
                        BOOL entrychanged = FALSE;
                        volDOB = this_Icon->ie_DiskObj;

                        Remove((struct Node*)&this_Icon->ie_IconNode);

D(bug("[IconVolumeList] %s: Found existing IconEntry for '%s' @ %p\n", __PRETTY_FUNCTION__, this_Icon->ie_IconListEntry.label, this_Icon));

                        /* Compare the Icon and update as needed ... */
                        if (strcmp(this_Icon->ie_IconListEntry.label, dvn->dvn_VolName) != 0)
                            entrychanged = TRUE;

                        if ((this_Icon->ie_IconListEntry.udata) &&
                            (dvn->dvn_FLags != ((struct VolumeIcon_Private *)this_Icon->ie_IconListEntry.udata)->vip_FLags))
                            entrychanged = TRUE;

                        if ((dvn->dvn_FLags & ICONENTRY_VOL_DISABLED) && !(volDOB))
                        {
                            volDOB = GetIconTags
                              (
                                dvn->dvn_DevName,
                                ICONGETA_FailIfUnavailable,        FALSE,
                                ICONGETA_GenerateImageMasks,       TRUE,
                                TAG_DONE
                              );
                        }

                        if (entrychanged)
                        {
D(bug("[IconVolumeList] %s: IconEntry changed - updating..\n", __PRETTY_FUNCTION__));
                            this_Icon = (struct IconEntry *)DoMethod(obj, MUIM_IconList_UpdateEntry, this_Icon, (IPTR)devname, (IPTR)dvn->dvn_VolName, (IPTR)NULL, volDOB, ST_ROOT);
                        }
                        if (this_Icon)
                            AddTail(&newiconlist, (struct Node*)&this_Icon->ie_IconNode);
                    }
                    else
                    {
                        if (dvn->dvn_FLags & ICONENTRY_VOL_DISABLED)
                        {
                            volDOB = GetIconTags
                              (
                                dvn->dvn_DevName,
                                ICONGETA_FailIfUnavailable,        FALSE,
                                ICONGETA_GenerateImageMasks,       TRUE,
                                TAG_DONE
                              );
                        }

                        if ((this_Icon = (struct IconEntry *)DoMethod(obj, MUIM_IconList_CreateEntry, (IPTR)devname, (IPTR)dvn->dvn_VolName, (IPTR)NULL, volDOB, ST_ROOT)) != NULL)
                        {
                            struct VolumeIcon_Private *volPrivate = this_Icon->ie_IconListEntry.udata;

                            volPrivate->vip_FLags = dvn->dvn_FLags;

D(bug("[IconVolumeList] %s: Created IconEntry for '%s' @ %p\n", __PRETTY_FUNCTION__, this_Icon->ie_IconListEntry.label, this_Icon));
                            if (!(this_Icon->ie_Flags & ICONENTRY_FLAG_HASICON))
                                this_Icon->ie_Flags |= ICONENTRY_FLAG_HASICON;

                            if ((strcasecmp(dvn->dvn_VolName, "Ram Disk:")) == 0)
                            {
D(bug("[IconVolumeList] %s: Setting '%s' icon node priority to 5\n", __PRETTY_FUNCTION__, this_Icon->ie_IconListEntry.label));
                                this_Icon->ie_IconNode.ln_Pri = 5;   // Special dirs get Priority 5
                            }
                            else
                            {
                                this_Icon->ie_IconNode.ln_Pri = 1;   // Fixed Media get Priority 1
                            }
                            AddTail(&newiconlist, (struct Node*)&this_Icon->ie_IconNode);
                        }
                        else
                        {
D(bug("[IconVolumeList] %s: Failed to Add IconEntry for '%s'\n", __PRETTY_FUNCTION__, dvn->dvn_VolName));
                        }
                    }
                } /* (dvn->dvn_VolName) */
            }
            IconVolumeList__DestroyDOSList(dvl);
            ForeachNodeSafe(iconlist, this_Icon, tmpNode)
            {
		if (this_Icon->ie_IconListEntry.type == ST_ROOT)
		{
D(bug("[IconVolumeList] %s: Destroying Removed IconEntry for '%s' @ %p\n", __PRETTY_FUNCTION__, this_Icon->ie_IconListEntry.label, this_Icon));
		    Remove((struct Node*)&this_Icon->ie_IconNode);
		    DoMethod(obj, MUIM_IconList_DestroyEntry, this_Icon);
		}
            }
D(bug("[IconVolumeList] %s: Updating Icon List\n", __PRETTY_FUNCTION__));
            ForeachNodeSafe(&newiconlist, this_Icon, tmpNode)
            {
                Remove((struct Node*)&this_Icon->ie_IconNode);
                DoMethod(obj, MUIM_Family_AddTail, (struct Node*)&this_Icon->ie_IconNode);
            }
        }
    }
    /* default display/sorting flags */
    DoMethod(obj, MUIM_IconList_Sort);

    DoSuperMethodA(CLASS, obj, (Msg) message);

    return 1;
}
///

IPTR IconVolumeList__MUIM_IconList_CreateEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_CreateEntry *message)
{
    struct IconEntry  		*this_Icon = NULL;
    struct VolumeIcon_Private   *volPrivate = NULL;

    D(
        bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__);

        if (message->icon_dob)
        {
            bug("[IconVolumeList] %s: Icon DiskObj @ %p\n", __PRETTY_FUNCTION__, message->icon_dob);
        }
    )

    this_Icon = DoSuperMethodA(CLASS, obj, (Msg) message);
    if ((this_Icon) && (this_Icon->ie_IconListEntry.type == ST_ROOT))
    {
        if ((volPrivate = AllocMem(sizeof(struct VolumeIcon_Private), MEMF_CLEAR)) != NULL)
        {
            this_Icon->ie_IconListEntry.udata = volPrivate;
        }
        else
        {
            DoMethod(obj, MUIM_IconList_DestroyEntry, this_Icon);
        }
    }
    return this_Icon;
}

IPTR IconVolumeList__MUIM_IconList_UpdateEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_UpdateEntry *message)
{
    struct IconEntry  		*this_Icon = NULL;
    struct VolumeIcon_Private   *volPrivate = NULL;

    D(bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__));

    this_Icon = DoSuperMethodA(CLASS, obj, (Msg) message);

    return this_Icon;
}

IPTR IconVolumeList__MUIM_IconList_DestroyEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DestroyEntry *message)
{
    struct VolumeIcon_Private   *volPrivate = NULL;
    IPTR                        rv = NULL;

    D(bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__));

    volPrivate = message->icon->ie_IconListEntry.udata;

    rv = DoSuperMethodA(CLASS, obj, (Msg) message);

    if (volPrivate)
        FreeMem(volPrivate, sizeof(struct VolumeIcon_Private));

    return rv;
}


///OM_GET()
/**************************************************************************
OM_GET
**************************************************************************/
IPTR IconVolumeList__OM_GET(struct IClass *CLASS, Object *obj, struct opGet *message)
{
#define STORE *(message->opg_Storage)

D(bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__));

    switch (message->opg_AttrID)
    {
#warning "TODO: Get the version/revision from our config.."
        case MUIA_Version:                              STORE = (IPTR)1; return 1;
        case MUIA_Revision:                             STORE = (IPTR)3; return 1;
    }

    return DoSuperMethodA(CLASS, obj, (Msg) message);
#undef STORE
}
///

#if WANDERER_BUILTIN_ICONVOLUMELIST
BOOPSI_DISPATCHER(IPTR, IconVolumeList_Dispatcher, CLASS, obj, message)
{
#if !defined(__AROS__)
    struct IClass	*CLASS = cl;
    Msg			message = msg;
#endif
    switch (message->MethodID)
    {
	case OM_NEW:                            return IconVolumeList__OM_NEW(CLASS, obj, (struct opSet *)message);
        case OM_GET:                            return IconVolumeList__OM_GET(CLASS, obj, (struct opGet *)message);
        case MUIM_IconList_Update:              return IconVolumeList__MUIM_IconList_Update(CLASS, obj, (struct MUIP_IconList_Update *)message);
	case MUIM_IconList_CreateEntry:         return IconVolumeList__MUIM_IconList_CreateEntry(CLASS,obj,(APTR)message);
	case MUIM_IconList_UpdateEntry:         return IconVolumeList__MUIM_IconList_UpdateEntry(CLASS,obj,(APTR)message);
	case MUIM_IconList_DestroyEntry:        return IconVolumeList__MUIM_IconList_DestroyEntry(CLASS,obj,(APTR)message);
    }

    return DoSuperMethodA(CLASS, obj, message);
}
BOOPSI_DISPATCHER_END

#if defined(__AROS__)
/* Class descriptor. */
const struct __MUIBuiltinClass _MUI_IconVolumeList_desc = { 
    MUIC_IconVolumeList, 
    MUIC_IconList, 
    sizeof(struct IconVolumeList_DATA), 
    (void*)IconVolumeList_Dispatcher
};
#endif
#else
#if !defined(__AROS__)
struct MUI_CustomClass  *initIconVolumeListClass(void)
{
    return (struct MUI_CustomClass *) MUI_CreateCustomClass(NULL,  NULL, IconList_Class, sizeof(struct IconVolumeList_DATA), ENTRY(IconVolumeList_Dispatcher));
}
#endif
#endif /* WANDERER_BUILTIN_ICONVOLUMELIST */

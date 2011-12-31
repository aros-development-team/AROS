/*
Copyright  2002-2011, The AROS Development Team. All rights reserved.
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

extern struct Library *MUIMasterBase;

struct DOSVolumeList
{
    struct List               dvl_List;
    APTR                      dvl_Pool;
};

struct DOSVolumeNode
{
    struct Node            dvn_Node;
    STRPTR            dvn_VolName;
    STRPTR            dvn_DevName;
    ULONG            dvn_Flags;
    struct MsgPort        *dvn_Port;
};

static BOOL VolumeIsOffline(struct DosList *dl)
{
        return dl->dol_Task == NULL;
}

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

        D(bug("[IconVolumeList] Found volume %b (length %u)\n", dl->dol_Name, len));

        if ((vn_VolName = AllocPooled(newdvl->dvl_Pool, len + 2)))
        {
            vn_VolName[len] = ':';
            vn_VolName[len + 1] = 0;
            strncpy(vn_VolName, dosname, len);

            if ((newdvn = AllocPooled(newdvl->dvl_Pool, sizeof(struct DOSVolumeNode))))
            {
            newdvn->dvn_VolName     = vn_VolName;
            D(bug("[IconVolumeList] Registering Volume '%s' @ %p, Type %d, Port 0x%p\n", newdvn->dvn_VolName, dl, dl->dol_Type, dl->dol_Task));

            newdvn->dvn_Port = dl->dol_Task;

                        if (VolumeIsOffline(dl))
                        {
                            D(bug("[IconVolumeList] %s: Volume '%s' is OFFLINE\n", __PRETTY_FUNCTION__, newdvn->dvn_VolName));
                            newdvn->dvn_Flags |= (ICONENTRY_VOL_OFFLINE|ICONENTRY_VOL_DISABLED);
                        }
#if DEBUG
            if (dl->dol_misc.dol_handler.dol_Startup)
            {
                struct FileSysStartupMsg *thisfs_SM = BADDR(dl->dol_misc.dol_handler.dol_Startup);

                            bug("[IconVolumeList] %s: Startup msg @ 0x%p\n", __PRETTY_FUNCTION__, thisfs_SM);
                            bug("[IconVolumeList] %s: Startup Device @ %p, Unit %d\n", __PRETTY_FUNCTION__, thisfs_SM->fssm_Device, thisfs_SM->fssm_Unit);
            }

            if (dl->dol_Task != NULL)
                            bug("[IconVolumeList] %s: Packet Style device\n", __PRETTY_FUNCTION__);
            else
                            bug("[IconVolumeList] %s: Unknown device type\n", __PRETTY_FUNCTION__);
#endif
            AddTail((struct List*)&newdvl->dvl_List, (struct Node*)&newdvn->dvn_Node);
            }
        }
        }
            D(bug("[IconVolumeList] Finished registering volumes\n"));
        UnLockDosList(LDF_VOLUMES|LDF_WRITE);

        dl = LockDosList(LDF_DEVICES|LDF_READ);
        while(( dl = NextDosEntry(dl, LDF_DEVICES)))
        {
        struct DOSVolumeNode         *dvn = NULL;
        char                   *nd_nambuf = NULL;
        struct InfoData         *nd_paramblock = NULL;

        UBYTE                     *dosname = (UBYTE*)AROS_BSTR_ADDR(dl->dol_Name);
        LONG                   len = AROS_BSTR_strlen(dl->dol_Name);

                D(bug("[IconVolumeList] %s: Checking Device '%s' @ %p (Device ", __PRETTY_FUNCTION__, dosname, dl));

        if (dl->dol_Task == NULL)
        {
                    D(bug("[IconVolumeList] %s: '%s' : handler inactive!\n", __PRETTY_FUNCTION__, dosname));
                    continue;
        }

        if ((nd_nambuf = AllocPooled(newdvl->dvl_Pool, len + 2)) != NULL)
        {
            strncpy(nd_nambuf, dosname, len);
            nd_nambuf[len] = ':';
            nd_nambuf[len + 1] = 0;

            if (!IsFileSystem(nd_nambuf))
            {
            FreePooled(newdvl->dvl_Pool, nd_nambuf, len + 2);
            continue;
            }

                    if ((nd_paramblock = AllocMem(sizeof(struct InfoData), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
                    {
            {
                if (!DoPkt(dl->dol_Task, ACTION_DISK_INFO, (SIPTR)MKBADDR(nd_paramblock), (SIPTR)BNULL, (SIPTR)BNULL, (SIPTR)BNULL, (SIPTR)BNULL)) {
                                FreeMem(nd_paramblock, sizeof(struct InfoData));
                                nd_paramblock = NULL;
                            }    
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
                BOOL volfound;

            /* For packet handlers it's enough to compare MsgPort */
            volfound = dvn->dvn_Port == dl->dol_Task;

            if (volfound)
            {
                if (!(dvn->dvn_Flags & ICONENTRY_VOL_OFFLINE))
                {
                if ((nd_paramblock) && (nd_paramblock->id_DiskType != ID_NO_DISK_PRESENT))
                {
                    STRPTR nd_namext;
                    int nd_namext_len = 0;

                                    D(bug("[IconVolumeList] %s: '%s' : Device unit %d, state = %x, Vol node @ %p\n", __PRETTY_FUNCTION__, nd_nambuf, nd_paramblock->id_UnitNumber, nd_paramblock->id_DiskState, BADDR(nd_paramblock->id_VolumeNode)));

                    found = TRUE;
                                    dvn->dvn_Flags &= ~(ICONENTRY_VOL_OFFLINE|ICONENTRY_VOL_DISABLED);

                    switch (nd_paramblock->id_DiskState)
                    {
                    case ID_VALIDATING:
                                        D(bug("[IconVolumeList] %s: '%s' : Validating\n", __PRETTY_FUNCTION__, nd_nambuf));

                    nd_namext = "BUSY";
                    nd_namext_len = 4;
                    break;

                    case ID_WRITE_PROTECTED:
                                        D(bug("[IconVolumeList] %s: '%s' : Volume is WRITE-PROTECTED\n", __PRETTY_FUNCTION__, nd_nambuf));

                    dvn->dvn_Flags |= ICONENTRY_VOL_READONLY;
                    break;
                    }

                    if (nd_namext_len > 0)
                    {
                    char *newVolName = AllocPooled(newdvl->dvl_Pool, strlen(dvn->dvn_VolName) + nd_namext_len + 2);

                    if (newVolName)
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
                                D(bug("[IconVolumeList] '%s' : Volume is offline... skipping\n", nd_nambuf));
                }
            }
            dvn = (struct DOSVolumeNode*)GetSucc(dvn);
            } /* dvn */

            if (!(found))
            {
                        D(bug("[IconVolumeList] %s: '%s' : Couldn't find an associated Volume\n", __PRETTY_FUNCTION__, nd_nambuf));
                        if ((nd_paramblock) && (nd_paramblock->id_DiskType != ID_NO_DISK_PRESENT))
                        {
                            if ((newdvn = (struct DOSVolumeNode *)AllocPooled(newdvl->dvl_Pool, sizeof(struct DOSVolumeNode))))
                            {
                                STRPTR nd_namext;
                                int nd_namext_len = 0;

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
                                default:
                                        nd_namext = "BUSY";
                                        nd_namext_len = 4;
                                        D(bug("[IconVolumeList] %s: '%s' : disk type 0x%lx\n", __PRETTY_FUNCTION__, nd_nambuf, nd_paramblock->id_DiskType));
                                        break;
                                }
                                if (nd_namext_len > 0)
                                {
                                    if ((newdvn->dvn_VolName = AllocPooled(newdvl->dvl_Pool, strlen(nd_nambuf) + nd_namext_len + 2)) != NULL)
                                    {
                                        sprintf(newdvn->dvn_VolName, "%s%s", nd_nambuf, nd_namext);
                                        newdvn->dvn_DevName = nd_nambuf;
                                        newdvn->dvn_Flags |= ICONENTRY_VOL_DISABLED;
                                        AddTail((struct List*)&newdvl->dvl_List, (struct Node*)&newdvn->dvn_Node);
                                    }
                                }
#if 0
                                else
                                {
                                    D(bug("[IconVolumeList] %s: '%s' : Unknown Condition?\n", __PRETTY_FUNCTION__, nd_nambuf));
                                }
#endif
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
#if 0 /* unused */
  struct IconDrawerList_DATA   *data = NULL;
#endif
//    struct TagItem            *tag = NULL,
//                                *tags = NULL;

D(bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__));

    obj = (Object *)DoSuperNewTags(CLASS, obj, NULL,
                    TAG_MORE, (IPTR) message->ops_AttrList);

    if (!obj)
        return FALSE;

    SET(obj, MUIA_IconList_DisplayFlags, (ICONLIST_DISP_VERTICAL | ICONLIST_DISP_MODEDEFAULT));
    SET(obj, MUIA_IconList_SortFlags, MUIV_IconList_Sort_ByName);

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
    struct IconEntry          *this_Icon = NULL;
    struct DOSVolumeList     *dvl = NULL;
    struct DOSVolumeNode    *dvn = NULL;
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

            ForeachNode(dvl, dvn)
            {
        D(bug("[IconVolumeList] %s: DOSVolumeNode  @ %p\n", __PRETTY_FUNCTION__, dvn));
                if (dvn->dvn_VolName)
                {
                    struct DiskObject       *volDOB = NULL;

            D(bug("[IconVolumeList] %s: DOSList Entry '%s'\n", __PRETTY_FUNCTION__, dvn->dvn_VolName));

                    if (dvn->dvn_Flags & ICONENTRY_VOL_OFFLINE)
                        devname = dvn->dvn_VolName;
                    else
                        devname = dvn->dvn_DevName;

            D(bug("[IconVolumeList] %s: Processing '%s'\n", __PRETTY_FUNCTION__, devname));

                    if ((this_Icon = FindIconlistVolumeIcon(iconlist, devname)) != NULL)
                    {
                        BOOL entrychanged = FALSE;
                        volDOB = this_Icon->ie_DiskObj;

                        if (dvn->dvn_Flags & ICONENTRY_VOL_OFFLINE)
                            this_Icon->ie_IconListEntry.flags |= ICONENTRY_VOL_OFFLINE;
                        if (dvn->dvn_Flags & ICONENTRY_VOL_DISABLED)
                            this_Icon->ie_IconListEntry.flags |= ICONENTRY_VOL_DISABLED;

                        Remove((struct Node*)&this_Icon->ie_IconNode);

            D(bug("[IconVolumeList] %s: Found existing IconEntry for '%s' @ %p\n", __PRETTY_FUNCTION__, this_Icon->ie_IconListEntry.label, this_Icon));

                        /* Compare the Entry and update as needed ... */
                        if (strcmp(this_Icon->ie_IconListEntry.label, dvn->dvn_VolName) != 0)
                            entrychanged = TRUE;

                        if ((this_Icon->ie_IconListEntry.udata) &&
                            (dvn->dvn_Flags != ((struct VolumeIcon_Private *)this_Icon->ie_IconListEntry.udata)->vip_FLags))
                            entrychanged = TRUE;

                        if ((dvn->dvn_Flags & ICONENTRY_VOL_DISABLED) && !(volDOB))
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
                        if (dvn->dvn_Flags & ICONENTRY_VOL_DISABLED)
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

                            volPrivate->vip_FLags = dvn->dvn_Flags;

                D(bug("[IconVolumeList] %s: Created IconEntry for '%s' @ %p\n", __PRETTY_FUNCTION__, this_Icon->ie_IconListEntry.label, this_Icon));

                            if (!(this_Icon->ie_Flags & ICONENTRY_FLAG_HASICON))
                                this_Icon->ie_Flags |= ICONENTRY_FLAG_HASICON;

                            if ((strcasecmp(dvn->dvn_VolName, "Ram Disk:")) == 0)
                            {
                D(bug("[IconVolumeList] %s: Setting '%s' entry node priority to 5\n", __PRETTY_FUNCTION__, this_Icon->ie_IconListEntry.label));
                                this_Icon->ie_IconNode.ln_Pri = 5;   // Special dirs get Priority 5
                            }
                            else
                            {
                                this_Icon->ie_IconNode.ln_Pri = 2;   // Fixed Media get Priority 2
                            }
                            AddTail(&newiconlist, (struct Node*)&this_Icon->ie_IconNode);
                        }
                D(else bug("[IconVolumeList] %s: Failed to Add IconEntry for '%s'\n", __PRETTY_FUNCTION__, dvn->dvn_VolName);)
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

        D(bug("[IconVolumeList] %s: Updating IconList\n", __PRETTY_FUNCTION__));
            ForeachNodeSafe(&newiconlist, this_Icon, tmpNode)
            {
                Remove((struct Node*)&this_Icon->ie_IconNode);
                DoMethod(obj, MUIM_Family_AddTail, (struct Node*)&this_Icon->ie_IconNode);
            }
        }
    }
    /* default display/sorting flags */

    DoSuperMethodA(CLASS, obj, (Msg) message);

    return 1;
}
///

struct IconEntry *IconVolumeList__MUIM_IconList_CreateEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_CreateEntry *message)
{
    struct IconEntry          *this_Icon = NULL;
    struct VolumeIcon_Private   *volPrivate = NULL;

    D(
        bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__);

        if (message->entry_dob)
        {
            bug("[IconVolumeList] %s: Entry DiskObj @ %p\n", __PRETTY_FUNCTION__, message->entry_dob);
        }
    )

    this_Icon = (struct IconEntry *)DoSuperMethodA(CLASS, obj, (Msg) message);
    if ((this_Icon) && (this_Icon->ie_IconListEntry.type == ST_ROOT))
    {
        volPrivate = AllocMem(sizeof(struct VolumeIcon_Private), MEMF_CLEAR);
        D(bug("[IconVolumeList] Allocated VolumeIcon_Private 0x%p\n", volPrivate));

        if (volPrivate)
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
    IPTR this_Icon;

    D(bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__));

    this_Icon = DoSuperMethodA(CLASS, obj, (Msg) message);

    return this_Icon;
}

IPTR IconVolumeList__MUIM_IconList_DestroyEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DestroyEntry *message)
{
    struct VolumeIcon_Private   *volPrivate = NULL;
    IPTR                        rv;

    D(bug("[IconVolumeList]: %s()\n", __PRETTY_FUNCTION__));

    volPrivate = message->entry->ie_IconListEntry.udata;

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
        /* TODO: Get the version/revision from our config.. */
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
    struct IClass    *CLASS = cl;
    Msg            message = msg;
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

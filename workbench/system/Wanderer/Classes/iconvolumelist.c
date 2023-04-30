/*
    Copyright  2002-2023, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/layers.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <proto/cybergraphics.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>

#ifndef AROS_MACROS_H
#   include <aros/macros.h>
#endif

#include <dos/dos.h>
#include <dos/datetime.h>
#include <dos/filehandler.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/rpattr.h>
#include <workbench/icon.h>
#include <workbench/workbench.h>

#include <devices/rawkeycodes.h>
#include <clib/alib_protos.h>

#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>

#include <cybergraphx/cybergraphics.h>

#include <libraries/mui.h>

#define DEBUG_ILC_EVENTS
#define DEBUG_ILC_KEYEVENTS
#define DEBUG_ILC_ICONRENDERING
#define DEBUG_ILC_ICONSORTING
#define DEBUG_ILC_ICONSORTING_DUMP
#define DEBUG_ILC_ICONPOSITIONING
#define DEBUG_ILC_LASSO
#define DEBUG_ILC_MEMALLOC

#include "iconlist_attributes.h"
#include "icon_attributes.h"
#include "iconlist.h"
#include "iconvolumelist_private.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

extern struct Library *MUIMasterBase;

struct DOSVolumeList
{
    struct List dvl_List;
    APTR dvl_Pool;
};

struct DOSVolumeNode
{
    struct Node dvn_Node;
    STRPTR dvn_VolName;
    STRPTR dvn_DosName;
    STRPTR dvn_Dev;
    ULONG dvn_Unit;
    ULONG dvn_Flags;
    struct MsgPort *dvn_Port;
};

static BOOL VolumeIsOffline(struct DosList *dl)
{
    return dl->dol_Task == NULL;
}

///IconVolumeList__IsDeviceAllowed()
static BOOL IconVolumeList__IsDeviceAllowed(struct IClass * CLASS, Object * obj, char *devname)
{
    return TRUE;
}

//#if !defined(ID_BUSY_DISK)
#define ID_BUSY_DISK    AROS_MAKE_ID('B','U','S','Y')
//#endif

///IconVolumeList__CreateDOSList()
static struct DOSVolumeList *IconVolumeList__CreateDOSList(struct IClass * CLASS, Object * obj)
{
    APTR pool = NULL;
    struct DosList *dl = NULL;
    struct DOSVolumeNode *newdvn = NULL;
    struct DOSVolumeList *newdvl = NULL;

    D(bug("[IconVolumeList]: %s()\n", __func__);)

    if ((pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 4096, 4096)) != NULL)
    {
        if ((newdvl =
                (struct DOSVolumeList *)AllocPooled(pool,
                    sizeof(struct DOSVolumeList))) != NULL)
        {
            NewList((struct List *)&newdvl->dvl_List);
            newdvl->dvl_Pool = pool;

            /* work around to only start scanning dos list after all */
            /* shared locks are gone, eg. in rom/dos/getdeviceproc.c RunHandler */
            dl = LockDosList(LDF_VOLUMES | LDF_WRITE);
            while ((dl = NextDosEntry(dl, LDF_VOLUMES)))
            {
                STRPTR vn_VolName;

                UBYTE *dosname = (UBYTE *) AROS_BSTR_ADDR(dl->dol_Name);
                LONG len = AROS_BSTR_strlen(dl->dol_Name);

                D(bug("[IconVolumeList] %s: Found volume %b (length %u)\n", __func__,
                        dl->dol_Name, len));

                if ((vn_VolName = AllocPooled(newdvl->dvl_Pool, len + 2)))
                {
                    strncpy(vn_VolName, dosname, len + 1);
                    vn_VolName[len] = ':';
                    vn_VolName[len + 1] = 0;

                    if ((newdvn =
                            AllocPooled(newdvl->dvl_Pool,
                                sizeof(struct DOSVolumeNode))))
                    {
                        newdvn->dvn_VolName = vn_VolName;
                        D(
                            bug("[IconVolumeList] %s: Volume '%s' @ %p, Type %d, Port 0x%p\n", __func__,
                                newdvn->dvn_VolName, dl, dl->dol_Type,
                                dl->dol_Task);
                            bug("[IconVolumeList] %s: DOSVolumeNode created @ 0x%p\n", __func__, newdvn);
                        )

                        newdvn->dvn_Port = dl->dol_Task;

                        if (VolumeIsOffline(dl))
                        {
                            D(bug
                                ("[IconVolumeList] %s: Volume '%s' is OFFLINE\n",
                                    __func__,
                                    newdvn->dvn_VolName));
                            newdvn->dvn_Flags |= ICONENTRY_VOL_OFFLINE;
                        }
                        AddTail((struct List *)&newdvl->dvl_List,
                            (struct Node *)&newdvn->dvn_Node);
                    }
                }
            }
            D(bug("[IconVolumeList] Finished registering volumes\n"));
            UnLockDosList(LDF_VOLUMES | LDF_WRITE);

            dl = LockDosList(LDF_DEVICES | LDF_READ);
            while ((dl = NextDosEntry(dl, LDF_DEVICES)))
            {
                struct DOSVolumeNode *dvn = NULL;
                char *nd_nambuf = NULL;
                struct InfoData *nd_paramblock = NULL;

                UBYTE *dosname = (UBYTE *) AROS_BSTR_ADDR(dl->dol_Name);
                LONG len = AROS_BSTR_strlen(dl->dol_Name);

                D(bug
                    ("[IconVolumeList] %s: Checking Device '%s' @ %p\n",
                        __func__, dosname, dl));

                if (dl->dol_Task == NULL)
                {
                    D(bug("[IconVolumeList] %s: '%s' : handler inactive!\n",
                            __func__, dosname));
                    continue;
                }

                if ((nd_nambuf =
                        AllocPooled(newdvl->dvl_Pool, len + 2)) != NULL)
                {
                    strncpy(nd_nambuf, dosname, len + 1);
                    nd_nambuf[len] = ':';
                    nd_nambuf[len + 1] = 0;

                    if (!IsFileSystem(nd_nambuf))
                    {
                        FreePooled(newdvl->dvl_Pool, nd_nambuf, len + 2);
                        continue;
                    }

                    if ((nd_paramblock =
                            AllocMem(sizeof(struct InfoData),
                                MEMF_CLEAR | MEMF_PUBLIC)) != NULL)
                    {
                        {
                            if (!DoPkt(dl->dol_Task, ACTION_DISK_INFO,
                                    (SIPTR) MKBADDR(nd_paramblock),
                                    (SIPTR) BNULL, (SIPTR) BNULL,
                                    (SIPTR) BNULL, (SIPTR) BNULL))
                            {
                                FreeMem(nd_paramblock,
                                    sizeof(struct InfoData));
                                nd_paramblock = NULL;
                            }
                        }
                    }
                    else
                    {
                        D(bug
                            ("[IconVolumeList] %s: '%s' : Failed to allocate InfoData storage\n",
                                __func__, dosname));
                    }

                    D(bug
                        ("[IconVolumeList] %s: '%s' : Checking for Attached Volumes ... \n",
                            __func__, dosname));
                    /* Find the Volume attached to this device */
                    BOOL found = FALSE;
                    dvn =
                        (struct DOSVolumeNode *)GetHead((struct List
                            *)&newdvl->dvl_List);
                    while ((dvn))
                    {
                        BOOL volfound;

                        D(
                            bug("[IconVolumeList] %s: '%s' : Checking DOSVolumeNode @ 0x%p \n", __func__, dosname, dvn);
                            if (dl->dol_misc.dol_handler.dol_Startup)
                            {
                                struct FileSysStartupMsg *thisfs_SM =
                                    BADDR(dl->dol_misc.dol_handler.dol_Startup);

                                bug("[IconVolumeList] %s: Device Startup msg @ 0x%p\n",
                                    __func__, thisfs_SM);
                                bug("[IconVolumeList] %s: Device Startup Device @ %p, Unit %d\n", __func__, thisfs_SM->fssm_Device, thisfs_SM->fssm_Unit);
                            })

                        /* For packet handlers it's enough to compare MsgPort */
                        volfound = (dvn->dvn_Port == dl->dol_Task);

                        if (volfound)
                        {
                            D(bug("[IconVolumeList] %s: '%s' : Devices match\n", __func__, dosname));

                            if (IconVolumeList__IsDeviceAllowed(CLASS, obj, dosname) && !(dvn->dvn_Flags & ICONENTRY_VOL_OFFLINE))
                            {
                                if ((nd_paramblock)
                                    && (nd_paramblock->id_DiskType !=
                                        ID_NO_DISK_PRESENT))
                                {
                                    STRPTR nd_namext;
                                    int nd_namext_len = 0;

                                    D(
                                        bug("[IconVolumeList] %s: '%s' : Device unit #%u, type = $%08x, state = $%08x, Vol node @ 0x%p\n",
                                            __func__, nd_nambuf,
                                            nd_paramblock->id_UnitNumber,
                                            nd_paramblock->id_DiskType,
                                            nd_paramblock->id_DiskState,
                                            nd_paramblock->id_VolumeNode ? BADDR(nd_paramblock->id_VolumeNode) : NULL);
                                    )

                                    found = TRUE;
                                    dvn->dvn_Flags &=
                                        ~(ICONENTRY_VOL_OFFLINE |
                                        ICONENTRY_VOL_DISABLED);

                                    switch (nd_paramblock->id_DiskState)
                                    {
                                    case ID_VALIDATING:
                                        D(bug
                                            ("[IconVolumeList] %s: '%s' : Validating\n",
                                                __func__,
                                                nd_nambuf));

                                        nd_namext = "BUSY";
                                        nd_namext_len = 4;
                                        break;

                                    case ID_WRITE_PROTECTED:
                                        D(bug
                                            ("[IconVolumeList] %s: '%s' : Volume is WRITE-PROTECTED\n",
                                                __func__,
                                                nd_nambuf));

                                        dvn->dvn_Flags |=
                                            ICONENTRY_VOL_READONLY;
                                        break;
                                    }

                                    if (nd_namext_len > 0)
                                    {
                                        char *newVolName =
                                            AllocPooled(newdvl->dvl_Pool,
                                            strlen(dvn->dvn_VolName) +
                                            nd_namext_len + 2);

                                        if (newVolName)
                                        {
                                            sprintf(newVolName, "%s%s",
                                                dvn->dvn_VolName,
                                                nd_namext);
                                            dvn->dvn_VolName = newVolName;
                                        }
                                    }
                                }
                                else
                                {
                                    D(bug
                                        ("[IconVolumeList] %s: '%s' : No Media Inserted (error state?)\n",
                                            __func__,
                                            nd_nambuf));
                                }

                                dvn->dvn_DosName = nd_nambuf;
                                D(bug
                                    ("[IconVolumeList] %s: DeviceName set to '%s' for '%s'\n",
                                        __func__,
                                        dvn->dvn_DosName,
                                        dvn->dvn_VolName));
                            }
                            else
                            {
                                D(bug
                                    ("[IconVolumeList] '%s' : Volume is offline... skipping\n",
                                        nd_nambuf));
                            }
                        }
                        dvn = (struct DOSVolumeNode *)GetSucc(dvn);
                    }           /* dvn */

                    if (!(found))
                    {
                        D(
                            bug("[IconVolumeList] %s: '%s' : Couldn't find an associated Volume\n",
                                __func__, nd_nambuf);)
                        if ((nd_paramblock)
                            && (nd_paramblock->id_DiskType !=
                                ID_NO_DISK_PRESENT))
                        {
                            if ((newdvn =
                                    (struct DOSVolumeNode
                                        *)AllocPooled(newdvl->dvl_Pool,
                                        sizeof(struct DOSVolumeNode))))
                            {
                                STRPTR nd_namext;
                                int nd_namext_len = 0;

                                newdvn->dvn_Flags |= ICONENTRY_VOL_DEVICE;
                                D(bug
                                    ("[IconVolumeList] %s: '%s' : disk type $%lx\n",
                                        __func__, nd_nambuf,
                                        nd_paramblock->id_DiskType));
                                switch (nd_paramblock->id_DiskType)
                                {
                                case ID_UNREADABLE_DISK:
                                    D(bug("[IconVolumeList] '%s' :      BAD DISK\n", __func__));
                                    nd_namext = "BAD";
                                    nd_namext_len = 3;
                                    break;
                                case ID_NOT_REALLY_DOS:
                                    D(bug("[IconVolumeList] '%s' :      NDOS DISK\n", __func__));
                                    nd_namext = "NDOS";
                                    nd_namext_len = 4;
                                    break;
                                case ID_KICKSTART_DISK:
                                    D(bug("[IconVolumeList] '%s' :      KICK DISK\n", __func__));
                                    nd_namext = "KICK";
                                    nd_namext_len = 4;
                                    break;
                                case ID_BUSY_DISK:
                                    D(bug("[IconVolumeList] '%s' :      BUSY DISK\n", __func__));
                                    nd_namext = "BUSY";
                                    nd_namext_len = 4;
                                    break;
                                default:
                                    break;
                                }

                                if (nd_namext_len > 0)
                                {
                                    if ((newdvn->dvn_VolName =
                                            AllocPooled(newdvl->dvl_Pool,
                                                strlen(nd_nambuf) +
                                                nd_namext_len + 2)) != NULL)
                                    {
                                        sprintf(newdvn->dvn_VolName, "%s%s",
                                            nd_nambuf, nd_namext);
                                        newdvn->dvn_DosName = nd_nambuf;
                                        newdvn->dvn_Flags |=
                                            ICONENTRY_VOL_DISABLED;
                                        AddTail((struct List *)
                                            &newdvl->dvl_List,
                                            (struct Node *)
                                            &newdvn->dvn_Node);
                                    }
                                }
                                else
                                {
                                    FreePooled(newdvl->dvl_Pool, newdvn, sizeof(struct DOSVolumeNode));
                                }
                            }
                        }
                    }

                    if (nd_paramblock)
                        FreeMem(nd_paramblock, sizeof(struct InfoData));
                }
            }
            UnLockDosList(LDF_DEVICES | LDF_READ);

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
    D(bug("[IconVolumeList]: %s()\n", __func__));
    if (dvl && dvl->dvl_Pool)
        DeletePool(dvl->dvl_Pool);
}
///

///OM_NEW()
/**************************************************************************
OM_NEW
**************************************************************************/
IPTR IconVolumeList__OM_NEW(struct IClass *CLASS, Object * obj,
    struct opSet *message)
{
#if 0                           /* unused */
    struct IconDrawerList_DATA *data = NULL;
#endif
    //    struct TagItem            *tag = NULL,
    //                                *tags = NULL;

    D(bug("[IconVolumeList]: %s()\n", __func__));

    obj = (Object *) DoSuperNewTags(CLASS, obj, NULL,
        TAG_MORE, (IPTR) message->ops_AttrList);

    if (!obj)
        return FALSE;

    SET(obj, MUIA_IconList_DisplayFlags,
        (ICONLIST_DISP_VERTICAL | ICONLIST_DISP_MODEDEFAULT));
    SET(obj, MUIA_IconList_SortFlags, MUIV_IconList_Sort_ByName);

    D(bug("[IconVolumeList] obj @ %p\n", obj));
    return (IPTR) obj;
}

///

static BOOL MatchIconlistNames(char *entryname, char *matchname)
{
    char *enend;
    char *mnend;
    int enlen;
    int mnlen;
    enlen = strlen(entryname);
    enend = strchr(entryname, ':');
    mnlen = strlen(matchname);
    mnend = strchr(matchname, ':');

    if ((enend) && (((IPTR)enend - (IPTR)entryname) <= enlen))
        enlen = ((IPTR)enend - (IPTR)entryname);
    if ((mnend) && (((IPTR)mnend - (IPTR)matchname) <= mnlen))
        mnlen = ((IPTR)mnend - (IPTR)matchname);

    if ((enlen == mnlen)&& (strncasecmp(entryname, matchname, enlen) == 0))
        return TRUE;
    return FALSE;
}

static struct IconEntry *FindIconlistVolumeIcon(struct List *iconlist,
    char *icondevname, char *iconvolname)
{
    struct IconEntry *foundEntry = NULL;

    /* First look for icons which match the volume name */
    ForeachNode(iconlist, foundEntry)
    {
        if ((foundEntry->ie_IconListEntry.type == ST_ROOT) && MatchIconlistNames(foundEntry->ie_IconListEntry.label, iconvolname))
            return foundEntry;
    }

    if (icondevname != iconvolname)
    {
        /* Then, match on device name */
        ForeachNode(iconlist, foundEntry)
        {
            if ((foundEntry->ie_IconListEntry.type == ST_ROOT) && MatchIconlistNames(foundEntry->ie_IconNode.ln_Name, iconvolname))
                return foundEntry;
        }
    }
    return NULL;
}

///MUIM_IconList_Update()
/**************************************************************************
MUIM_IconList_Update
**************************************************************************/
IPTR IconVolumeList__MUIM_IconList_Update(struct IClass * CLASS,
    Object * obj, struct MUIP_IconList_Update * message)
{
    //struct IconVolumeList_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry *this_Icon = NULL;
    struct DOSVolumeList *dvl = NULL;
    struct DOSVolumeNode *dvn = NULL;
    char *devname = NULL;
    struct List *iconlist = NULL;
    struct List newiconlist;
    struct Node *tmpNode = NULL;

    D(bug("[IconVolumeList]: %s()\n", __func__);)

    GET(obj, MUIA_Family_List, &iconlist);

    if (iconlist != NULL)
    {
        NewList(&newiconlist);

        if ((dvl = IconVolumeList__CreateDOSList(CLASS,obj)) != NULL)
        {
            D(bug("[IconVolumeList] %s: DOSVolumeList @ %p\n",
                    __func__, dvl));

            ForeachNode(dvl, dvn)
            {
                D(bug("[IconVolumeList] %s: DOSVolumeNode  @ %p\n",
                        __func__, dvn));
                if (dvn->dvn_VolName)
                {
                    struct DiskObject *volDOB = NULL;

                    D(bug("[IconVolumeList] %s: DOSList Entry '%s'\n",
                            __func__, dvn->dvn_VolName));

                    if (dvn->dvn_Flags & ICONENTRY_VOL_OFFLINE)
                        devname = dvn->dvn_VolName;
                    else
                        devname = dvn->dvn_DosName;

                    D(bug("[IconVolumeList] %s: Processing '%s'\n",
                            __func__, devname));

                    this_Icon = FindIconlistVolumeIcon(iconlist, devname, dvn->dvn_VolName);
                    if ((this_Icon) &&
                        (!(dvn->dvn_Flags & ICONENTRY_VOL_DEVICE) || (_volpriv(this_Icon)->vip_FLags & ICONENTRY_VOL_DEVICE)))
                    {
                        BOOL entrychanged = FALSE;

                        D(
                            bug
                            ("[IconVolumeList] %s: Found existing IconEntry '%s' @ %p\n",
                                __func__,
                                this_Icon->ie_IconListEntry.label,
                                this_Icon);)

                        Remove((struct Node *)&this_Icon->ie_IconNode);

                        /* Compare the Entry and update as needed ... */
                        if (dvn->dvn_VolName && strcmp(this_Icon->ie_IconListEntry.label,
                                dvn->dvn_VolName) != 0)
                        {
                            D(bug("[IconVolumeList] %s: '%s' != '%s'\n",
                                    __func__, this_Icon->ie_IconListEntry.label, dvn->dvn_VolName));
                            entrychanged = TRUE;
                        }
                        else if ((!dvn->dvn_VolName) && (this_Icon->ie_IconListEntry.label))
                        {
                            D(bug("[IconVolumeList] %s: 0x%p != 0x%p\n",
                                    __func__, this_Icon->ie_IconListEntry.label, dvn->dvn_VolName));
                            entrychanged = TRUE;
                        }
                        if ((!dvn->dvn_DosName && this_Icon->ie_DiskObj) ||
                            (!(_volpriv(this_Icon)->vip_FLags & ICONENTRY_VOL_DEVICE) || (dvn->dvn_Flags & ICONENTRY_VOL_DEVICE)))
                        {
                            volDOB = this_Icon->ie_DiskObj;
                        }
                        else
                        {
                            D(bug("[IconVolumeList] %s: Freeing old DOB\n", __func__));
                            FreeDiskObject(this_Icon->ie_DiskObj);
                            this_Icon->ie_DiskObj =  NULL;
                            entrychanged = TRUE;
                        }

                        if ((this_Icon->ie_IconListEntry.udata) &&
                            (dvn->dvn_Flags != _volpriv(this_Icon)->vip_FLags))
                        {
                            D(bug("[IconVolumeList] %s: $%08x != $%08x\n",
                                    __func__, dvn->dvn_Flags, _volpriv(this_Icon)->vip_FLags));
                            entrychanged = TRUE;
                        }

                        if (!(volDOB))
                        {
                            IPTR iconlistScreen = (IPTR)_screen(obj);
                            IPTR geticon_error = 0;
                            D(bug("[IconVolumeList] %s: Getting Icon for '%s'\n", __func__, dvn->dvn_DosName);)
                            volDOB = GetIconTags
                            ((dvn->dvn_DosName) ? dvn->dvn_DosName : dvn->dvn_VolName,
                                (iconlistScreen) ? ICONGETA_Screen : TAG_IGNORE, iconlistScreen,
                                (iconlistScreen) ? ICONGETA_RemapIcon : TAG_IGNORE, TRUE,
                                ICONGETA_FailIfUnavailable, FALSE,
                                ICONGETA_GenerateImageMasks, TRUE,
                                ICONA_ErrorCode, &geticon_error,
                                TAG_DONE);
                            D(bug("[IconVolumeList] %s: IconEntry Default DOB = 0x%p\n", __func__, volDOB);)
                        }

                        if (entrychanged)
                        {
                            D(bug("[IconVolumeList] %s: IconEntry changed - updating..\n", __func__);)

                            _volpriv(this_Icon)->vip_FLags = dvn->dvn_Flags;

                            this_Icon =
                                (struct IconEntry *)DoMethod(obj, MUIM_IconList_UpdateEntry, this_Icon,
                                (IPTR) devname, (IPTR) dvn->dvn_VolName, (IPTR) NULL, volDOB, ST_ROOT);
                        }
                        if (this_Icon)
                            AddTail(&newiconlist,
                                (struct Node *)&this_Icon->ie_IconNode);
                    }
                    
                    if (!(this_Icon))
                    {
                        struct VolumeIcon_Private * volPrivate = AllocMem(sizeof(struct VolumeIcon_Private), MEMF_CLEAR);
                        IPTR iconlistScreen = (IPTR)_screen(obj);
                        IPTR geticon_error = 0;

                        if (volPrivate) volPrivate->vip_FLags = dvn->dvn_Flags;

                        D(bug("[IconVolumeList] %s: Getting Icon for '%s'\n", __func__, dvn->dvn_DosName);)
                        volDOB = GetIconTags
                            ((dvn->dvn_DosName) ? dvn->dvn_DosName : dvn->dvn_VolName,
                            (iconlistScreen) ? ICONGETA_Screen : TAG_IGNORE, iconlistScreen,
                            (iconlistScreen) ? ICONGETA_RemapIcon : TAG_IGNORE, TRUE,
                            ICONGETA_FailIfUnavailable, FALSE,
                            ICONGETA_GenerateImageMasks, TRUE,
                            ICONA_ErrorCode, &geticon_error,
                            TAG_DONE);

                        if ((volPrivate) && ((this_Icon = (struct IconEntry *)DoMethod(obj, MUIM_IconList_CreateEntry,
                                    (IPTR) devname, (IPTR) dvn->dvn_VolName, (IPTR) NULL, volDOB,
                                    ST_ROOT, volPrivate)) != NULL))
                        {
                            D(bug
                                ("[IconVolumeList] %s: Created IconEntry for '%s' @ %p\n",
                                    __func__,
                                    this_Icon->ie_IconListEntry.label,
                                    this_Icon));

                            if (!(this_Icon->ie_Flags &
                                    ICONENTRY_FLAG_HASICON))
                                this_Icon->ie_Flags |=
                                    ICONENTRY_FLAG_HASICON;

                            if ((strcasecmp(dvn->dvn_VolName,
                                        "Ram Disk:")) == 0)
                            {
                                D(bug
                                    ("[IconVolumeList] %s: Setting '%s' entry node priority to 5\n",
                                        __func__,
                                        this_Icon->ie_IconListEntry.label));
                                this_Icon->ie_IconNode.ln_Pri = 5;      // Special dirs get Priority 5
                            }
                            else
                            {
                                this_Icon->ie_IconNode.ln_Pri = 2;      // Fixed Media get Priority 2
                            }
                            AddTail(&newiconlist,
                                (struct Node *)&this_Icon->ie_IconNode);
                        }
                        D(
                            else
                                bug("[IconVolumeList] %s: Failed to Add IconEntry for '%s'\n",
                                    __func__,
                                    dvn->dvn_VolName);)
                    }
                }               /* (dvn->dvn_VolName) */
            }
            IconVolumeList__DestroyDOSList(dvl);
            ForeachNodeSafe(iconlist, this_Icon, tmpNode)
            {
                if (this_Icon->ie_IconListEntry.type == ST_ROOT)
                {
                    D(bug("[IconVolumeList] %s: Destroying Removed IconEntry for '%s' @ %p\n",
                            __func__,
                            this_Icon->ie_IconListEntry.label, this_Icon);)
                    Remove((struct Node *)&this_Icon->ie_IconNode);
                    DoMethod(obj, MUIM_IconList_DestroyEntry, this_Icon);
                }
            }

            D(bug("[IconVolumeList] %s: Updating IconList\n",
                    __func__));
            ForeachNodeSafe(&newiconlist, this_Icon, tmpNode)
            {
                Remove((struct Node *)&this_Icon->ie_IconNode);
                DoMethod(obj, MUIM_Family_AddTail,
                    (struct Node *)&this_Icon->ie_IconNode);
            }
        }
    }
    /* default display/sorting flags */

    DoSuperMethodA(CLASS, obj, (Msg) message);

    return 1;
}

IPTR IconVolumeList__MUIM_IconList_UpdateEntry(struct IClass * CLASS,
    Object * obj, struct MUIP_IconList_UpdateEntry * message)
{
    IPTR this_Icon;

    D(bug("[IconVolumeList]: %s()\n", __func__));

    this_Icon = DoSuperMethodA(CLASS, obj, (Msg) message);

    return this_Icon;
}

IPTR IconVolumeList__MUIM_IconList_DestroyEntry(struct IClass * CLASS,
    Object * obj, struct MUIP_IconList_DestroyEntry * message)
{
    struct VolumeIcon_Private *volPrivate = NULL;
    IPTR rv;

    D(bug("[IconVolumeList]: %s()\n", __func__));

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
IPTR IconVolumeList__OM_GET(struct IClass * CLASS, Object * obj,
    struct opGet * message)
{
#define STORE *(message->opg_Storage)

    D(bug("[IconVolumeList]: %s()\n", __func__));

    switch (message->opg_AttrID)
    {
        /* TODO: Get the version/revision from our config.. */
    case MUIA_Version:
        STORE = (IPTR) 1;
        return 1;
    case MUIA_Revision:
        STORE = (IPTR) 3;
        return 1;
    }

    return DoSuperMethodA(CLASS, obj, (Msg) message);
#undef STORE
}

///

#if WANDERER_BUILTIN_ICONVOLUMELIST
BOOPSI_DISPATCHER(IPTR, IconVolumeList_Dispatcher, CLASS, obj, message)
{
    switch (message->MethodID)
    {
    case OM_NEW:
        return IconVolumeList__OM_NEW(CLASS, obj, (struct opSet *)message);
    case OM_GET:
        return IconVolumeList__OM_GET(CLASS, obj, (struct opGet *)message);
    case MUIM_IconList_Update:
        return IconVolumeList__MUIM_IconList_Update(CLASS, obj,
            (struct MUIP_IconList_Update *)message);
    case MUIM_IconList_CreateEntry:
        return IconVolumeList__MUIM_IconList_CreateEntry(CLASS, obj,
            (APTR) message);
    case MUIM_IconList_UpdateEntry:
        return IconVolumeList__MUIM_IconList_UpdateEntry(CLASS, obj,
            (APTR) message);
    case MUIM_IconList_DestroyEntry:
        return IconVolumeList__MUIM_IconList_DestroyEntry(CLASS, obj,
            (APTR) message);
    }

    return DoSuperMethodA(CLASS, obj, message);
}

BOOPSI_DISPATCHER_END
    /* Class descriptor. */
const struct __MUIBuiltinClass _MUI_IconVolumeList_desc = {
    MUIC_IconVolumeList,
    MUIC_IconList,
    sizeof(struct IconVolumeList_DATA),
    (void *)IconVolumeList_Dispatcher
};
#endif /* WANDERER_BUILTIN_ICONVOLUMELIST */

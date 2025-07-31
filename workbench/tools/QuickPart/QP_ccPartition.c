/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG
#include	<aros/debug.h>

#include	<proto/expansion.h>
#include	<proto/exec.h>
#include	<proto/graphics.h>
#include	<proto/partition.h>
#include	<proto/dos.h>
#include	<proto/intuition.h>
#include	<proto/muimaster.h>
#include	<proto/locale.h>
#include	<proto/utility.h>

#include   <proto/alib.h>

#include	<libraries/configvars.h>
#include	<libraries/expansionbase.h>
#include	<libraries/partition.h>
#include	<libraries/mui.h>

#include	<devices/trackdisk.h>
#include	<devices/scsidisk.h>

#include	<dos/dos.h>
#include	<dos/filehandler.h>

#include <graphics/gfxmacros.h>

#include <utility/tagitem.h>

#include	<exec/memory.h>
#include	<exec/execbase.h>
#include	<exec/lists.h>
#include	<exec/nodes.h>
#include	<exec/types.h>
#include	<exec/ports.h>

#include	<zune/systemprefswindow.h>
#include	<zune/prefseditor.h>
#include	<zune/aboutwindow.h>

#include	<stdlib.h>
#include	<stdio.h>
#include	<strings.h>
#include    <string.h>

#include <aros/locale.h>

#include "QP_Intern.h"

#define _QP_CCPARTITION_C

#include "QP_ccApp.h"
#include "QP_ccDisk.h"
#include "QP_ccPartition.h"
#include "QP_ccPartitionContainer.h"
#include "QP_globals.h"

extern struct   MUI_CustomClass      *mcc_qptxt;
extern struct   MUI_CustomClass      *mcc_qpfree;

/* QuickPart PARTITION Custom Class */

#define SETUP_INST_DATA struct QPPartition_DATA *data = INST_DATA(CLASS, self)

/** Standard Class Methods **/

static IPTR QPPartition__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct	TagItem	*tags = NULL,
                *tag = NULL;

    /* Tmp Object Pointers Used to create our "Partition" */
    Object                      *Object_Disk = NULL, *Object_Contents = NULL;
    Object                      *Part_Parent = NULL;
    Object                      *Part_Label, *Part_Desc;

    /* Tmp strings & values */
    IPTR                        Part_BGColor = (IPTR)NULL;
    struct PartitionHandle      *Part_PHandle = NULL;
    const struct PartitionAttribute   *Part_PAttrlist = NULL; /* supported partition attributes */
    struct PartitionType        Part_Type;
    struct DriveGeometry *Disk_Geom = NULL;
    char *partLabel = "unknown";

    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Background:
            Part_BGColor = (IPTR)tag->ti_Data;
            D(bug("[QuickPart:Part.I] BGColor set\n"));
            break;

        case MUIA_QPart_Disk_Object:
            Object_Disk = (Object *)tag->ti_Data;
            D(bug("[QuickPart:Part.I] Disk Obj @ %p\n", Object_Disk));

        case MUIA_QPart_Disk_Geometry:
            Disk_Geom = (struct DriveGeometry *)tag->ti_Data;
            D(bug("[QuickPart:Part.I] Disk Geom @ %p\n", Disk_Geom));
            break;

        case MUIA_QPart_ccPartition_Handle:
            Part_PHandle = (struct PartitionHandle *)tag->ti_Data;
            D(bug("[QuickPart:Part.I] Partition Handle @ %p\n", Part_PHandle));
            break;

        case MUIA_QPart_ccPartition_Parent:
            Part_Parent = (Object *)tag->ti_Data;
            D(bug("[QuickPart:Part.I] Partition Container @ %p\n", Part_Parent));
            break;

        default:
            continue; /* Don't supress non-processed tags */
        }
        tag->ti_Tag = TAG_IGNORE;
    }

    if ((Object_Disk == NULL) || (Part_PHandle == NULL) ||
        (Part_Parent == NULL) || (Disk_Geom == NULL))
        return (IPTR)NULL;

    Part_PAttrlist = QueryPartitionAttrs(Part_PHandle);
    D(bug("[QuickPart:Part.I] Partition Attributes @ %p\n", Part_PAttrlist));

    struct TagItem ptAttrs[] = 
    {
        { PT_TYPE, (IPTR)&Part_Type },
        { TAG_DONE }
    };
    D(bug("[QuickPart:Part.I] Getting Partition Type Attributes\n"));
    GetPartitionAttrs(Part_PHandle, ptAttrs);

    if (Part_BGColor == (IPTR)NULL)
    {
        D(bug("[QuickPart:Part.I] No BGColor set ..\n"));
        GetPartitionDrawAttribs(&Part_Type, &Part_BGColor, &partLabel);
    }

    D(bug("[QuickPart:Part.I] Creating PARTITION Object\n"));

    self = (Object *)DoSuperNewTags
         (
            CLASS, self, NULL,
            MUIA_InnerLeft, 0,
            MUIA_InnerTop, 0,
            MUIA_InnerRight, 0,
            MUIA_InnerBottom, 0,

            Child, (IPTR)(Object_Contents = VGroup,
                MUIA_Background, (IPTR)Part_BGColor,
                MUIA_InnerLeft, 1,
                MUIA_InnerTop, 0,
                MUIA_InnerRight, 1,
                MUIA_InnerBottom, 1,
                MUIA_InputMode, MUIV_InputMode_RelVerify,
                MUIA_CycleChain, 1,
                MUIA_Group_SameWidth, FALSE,
                Child, HGroup,
                    MUIA_FixHeight, 10,
                    MUIA_InnerLeft, 1,
                    MUIA_InnerTop, 1,
                    Child, (IPTR)(Part_Label = NewObject(mcc_qptxt->mcc_Class, NULL,
                        MUIA_Font, MUIV_Font_Tiny,
                        MUIA_Text_SetMin, FALSE,
                        MUIA_Text_SetMax, FALSE,
                        MUIA_Text_Contents, (IPTR)partLabel,
                    TAG_DONE)),
                    Child, RectangleObject,
                    End,
                End,
                Child, VGroup,
                    MUIA_InnerLeft, 0,
                    MUIA_InnerTop, 0,
                    MUIA_InnerRight, 0,
                    MUIA_InnerBottom, 0,
                    MUIA_Background, (IPTR)DEF_DRIVEPANE_BACK,
                    Child, HGroup,
                        Child, (IPTR)(Part_Desc = NewObject(mcc_qptxt->mcc_Class, NULL,
                            MUIA_Text_SetMin, FALSE,
                            MUIA_Text_SetMax, FALSE,
                            MUIA_Font, MUIV_Font_Tiny,
                        TAG_DONE)),
                        Child, RectangleObject,
                        End,
                    End,
                    Child, RectangleObject,
                    End,
                End,
            End),
            TAG_MORE, (IPTR)message->ops_AttrList   
         );

    if (self != NULL)
    {
        /* Initial local instance data */
        SETUP_INST_DATA;

        struct DriveGeometry Part_Geom;
        char *Part_Name;
        ULONG    Part_Pos;
        struct TagItem ptAttrs[] = 
        {
            { PT_DOSENVEC,  (IPTR)&data->qppd_Part_DE   },
            { PT_GEOMETRY,  (IPTR) &Part_Geom           },
            { PT_POSITION,  (IPTR)&Part_Pos             },
            { PT_NAME,      (IPTR)&Part_Name            },
            { TAG_DONE,     0                           }
        };
        GetPartitionAttrs( Part_PHandle, ptAttrs);
        D(bug("[QuickPart:Part.I] Obtained Partition DOSENVEC\n"));
        D(bug("[QuickPart:Part.I]   DE->LowCyl = %lu, DE->HighCyl = %lu\n", data->qppd_Part_DE.de_LowCyl, data->qppd_Part_DE.de_HighCyl));
        D(bug("[QuickPart:Part.I]   DE->Surfaces = %u, DE->BlocksPerTrack = %u\n", data->qppd_Part_DE.de_Surfaces, data->qppd_Part_DE.de_BlocksPerTrack));
        D(bug("[QuickPart:Part.I]   Geom->TotalSectors = %lu\n", Part_Geom.dg_TotalSectors));
        D(bug("[QuickPart:Part.I]   Geom->Cylinders = %lu\n", Part_Geom.dg_Cylinders));
        D(bug("[QuickPart:Part.I]   Geom->SectorSize = %lu\n", Part_Geom.dg_SectorSize));
        D(bug("[QuickPart:Part.I]   (Disk Geom)->TotalSectors = %lu\n", Disk_Geom->dg_TotalSectors));
        D(bug("[QuickPart:Part.I]   (Disk Geom)->Cylinders = %lu\n", Disk_Geom->dg_Cylinders));
        D(bug("[QuickPart:Part.I]   (Disk Geom)->SectorSize = %lu\n", Disk_Geom->dg_SectorSize));

        data->qppd_Geom = Disk_Geom;
        data->qpdd_Object_Disk = Object_Disk;
        CopyMem(&Part_Type, &data->qppd_Part_Type, sizeof(Part_Type));

        data->qppd_Object_Parent = Part_Parent;
        data->qppd_Part_PH = Part_PHandle;
        data->qppd_Part_Pos = Part_Pos;
        
        data->qppd_PatEH.ehn_Class  = CLASS;
        data->qppd_PatEH.ehn_Events = IDCMP_INTUITICKS;
        data->qppd_Pattern = 0xCCCC;

        if (partLabel)
            data->qppd_Str_Type = StrDup(partLabel);

        D(bug("[QuickPart:Part.I] Self @ %p\n", self));

        if (OpenPartitionTable(data->qppd_Part_PH) == 0)
        {
            D(bug("[QuickPart:Part.I] Partition has subpartitions ..\n"));
            Object *tmp_QPDisk_Object_Partition_Contents = NULL;
            ClosePartitionTable(data->qppd_Part_PH);

            if ((tmp_QPDisk_Object_Partition_Contents = NewObject(mcc_qppartitioncontainer->mcc_Class, NULL,
                        MUIA_Background, (IPTR)Part_BGColor,
                        MUIA_QPart_ccPartitionContainer_PartObj, self,
                        MUIA_QPart_Disk_Object, (IPTR)Object_Disk,
                        MUIA_QPart_Disk_Geometry, (IPTR)data->qppd_Geom,
                        MUIA_QPart_ccPartition_Parent, (IPTR)data->qppd_Object_Parent,
                        MUIA_QPart_ccPartition_Handle, (IPTR)data->qppd_Part_PH,
                        TAG_DONE)) != NULL)
            {
                if (DoMethod(self, MUIM_Group_InitChange))
                {
                    DoMethod(self, OM_REMMEMBER, Object_Contents);

                    DoMethod(self, OM_ADDMEMBER, tmp_QPDisk_Object_Partition_Contents);

                    DoMethod(self, MUIM_Group_ExitChange);
                    data->qppd_Object_Contents = tmp_QPDisk_Object_Partition_Contents;
                    data->qppd_Str_Label = data->qppd_Str_Type;
                    SET(data->qppd_Object_Contents, MUIA_ShortHelp,  data->qppd_Str_Label);
                }
            }
        }
        else
        {
            struct DosList  *dl;
            char            partLabelTxt[128];
            UQUAD           partoff = 0, adjustedsize;
            IPTR            diskDev = 0, diskUnit = 0;
            int             ddlen, sizecounter = 0;

            D(bug("[QuickPart:Part.I] Finding partition details...\n"));

            data->qppd_Object_Part = Object_Contents;

            GET(data->qpdd_Object_Disk, MUIA_QPart_Disk_Handler, &diskDev);
            GET(data->qpdd_Object_Disk, MUIA_QPart_Disk_Unit, &diskUnit);
            ddlen = strlen((char *)diskDev);

            D(bug("[QuickPart:Part.I] Partition device is %s:%u\n", diskDev, diskUnit));

            adjustedsize = (((data->qppd_Part_DE.de_HighCyl - data->qppd_Part_DE.de_LowCyl + 1) * data->qppd_Part_DE.de_Surfaces * data->qppd_Part_DE.de_BlocksPerTrack) - 1) * Part_Geom.dg_SectorSize;

            while (( adjustedsize > 1024 ) && (QP__SizeName[sizecounter] != NULL))
            {
                sizecounter +=1;
                adjustedsize /= 1024;
            }
            sprintf(partLabelTxt, "%u%s", adjustedsize, QP__SizeName[sizecounter]);
            data->qppd_Str_Size = StrDup(partLabelTxt);
            sprintf(partLabelTxt, "%s, %s", partLabel, data->qppd_Str_Size);
            data->qppd_Str_Label = StrDup(partLabelTxt);
            SET(Part_Label, MUIA_Text_Contents, data->qppd_Str_Label);
            SET(data->qppd_Object_Part, MUIA_ShortHelp, data->qppd_Str_Label);

            D(bug("[QuickPart:Part.I] Looking for DOSDevice...\n"));

            while (Part_Parent)
            {
                UQUAD parentStart = 0;
                GET(Part_Parent, MUIA_QPart_ccPartition_Start, &parentStart);
                partoff += parentStart;
                GET(Part_Parent, MUIA_QPart_ccPartition_Parent, &Part_Parent);
            }

            dl = LockDosList(LDF_DEVICES|LDF_READ);
            if (dl) {
                char devNmBufTmp[64];

                while((dl = NextDosEntry(dl, LDF_DEVICES)))
                {
                    if ((dl->dol_Task) && (dl->dol_misc.dol_handler.dol_Startup != BNULL))
                    {
                        char *devNam = AROS_BSTR_ADDR(dl->dol_Name);
                        LONG len = AROS_BSTR_strlen(dl->dol_Name);
                        D(bug("[QuickPart:Part.I] Checking device %s\n", devNam));
                        strncpy(devNmBufTmp, devNam, 64);
                        devNmBufTmp[len] = ':';
                        devNmBufTmp[len + 1] = 0;
                        if (IsFileSystem(devNmBufTmp))
                        {
                            struct FileSysStartupMsg *fsstartup = (struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup);
                            if (fsstartup->fssm_Device != BNULL)
                            {
                                D(bug("[QuickPart:Part.I]         attached to %s:%u\n", AROS_BSTR_ADDR(fsstartup->fssm_Device), fsstartup->fssm_Unit));
                                if ((strncmp(AROS_BSTR_ADDR(fsstartup->fssm_Device), (const char *)diskDev, ddlen) == 0) && (diskUnit == fsstartup->fssm_Unit))
                                {
                                    D(bug("[QuickPart:Part.I] DOSDevice is on this disk...\n"));
                                    if (fsstartup->fssm_Environ != BNULL)
                                    {
                                        struct DosEnvec *fsde = (struct DosEnvec *)BADDR(fsstartup->fssm_Environ);
                                        D(bug("[QuickPart:Part.I]          device LowCyl = %lu, HighCyl = %lu\n", fsde->de_LowCyl, fsde->de_HighCyl));
                                        D(bug("[QuickPart:Part.I]          blocks %llu -> %llu\n", (fsde->de_LowCyl * fsde->de_Surfaces * fsde->de_BlocksPerTrack), ((fsde->de_HighCyl + 1) * fsde->de_Surfaces * fsde->de_BlocksPerTrack - 1)));
                                        if (((fsde->de_LowCyl * fsde->de_Surfaces * fsde->de_BlocksPerTrack) == (partoff + (data->qppd_Part_DE.de_LowCyl * data->qppd_Part_DE.de_Surfaces * data->qppd_Part_DE.de_BlocksPerTrack))) &&
                                            (((fsde->de_HighCyl + 1) * fsde->de_Surfaces * fsde->de_BlocksPerTrack - 1) == (partoff + ((data->qppd_Part_DE.de_HighCyl + 1) * data->qppd_Part_DE.de_Surfaces * data->qppd_Part_DE.de_BlocksPerTrack - 1))))
                                        {
                                            D(bug("[QuickPart:Part.I] Partition Device found!\n"));
                                            data->qppd_Str_DOSDev = StrDup(devNmBufTmp);
                                            SET(Part_Desc, MUIA_Text_Contents, (IPTR)devNmBufTmp);
                                            
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                UnLockDosList(LDF_VOLUMES|LDF_READ);
            }
        }
    }

    return (IPTR)self;
}

static IPTR QPPartition__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    SETUP_INST_DATA;
    struct TagItem              *tstate = message->ops_AttrList,
                                    *tag = NULL;
    IPTR                        	winretval = FALSE, oldroot = (IPTR)NULL;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_HorizWeight:
            data->qppd_Weight = tag->ti_Data;
            //fall through
        default:
                break;
        }
    }

    return DoSuperMethodA(CLASS, self, (Msg) message);
}

static IPTR QPPartition__OM_GET
(
    Class *CLASS, Object *self, struct opGet *message
)
{
    SETUP_INST_DATA;
    IPTR                          *store = message->opg_Storage;
    IPTR    	      		 retval = TRUE;

    switch(message->opg_AttrID)
    {
    case MUIA_HorizWeight:
        *store = (IPTR)data->qppd_Weight;
        break;

    case MUIA_QPart_ccPartition_Parent:
        *store = (IPTR)data->qppd_Object_Parent;
        break;

    case MUIA_QPart_ccPartition_Container:
        *(Object **)store = data->qppd_Object_Contents;
        break;

    case MUIA_QPart_ccPartition_PartObj:
        *(Object **)store = data->qppd_Object_Part;
        break;

    case MUIA_QPart_ccPartition_Type:
        CopyMem(&data->qppd_Part_Type, (APTR)store, sizeof(data->qppd_Part_Type));
        break;

    case MUIA_QPart_ccPartition_TypeStr:
        *(char **)store = data->qppd_Str_Type;
        break;

    case MUIA_QPart_ccPartition_SizeStr:
        *(char **)store = data->qppd_Str_Size;
        break;

    case MUIA_QPart_ccPartition_DOSDevStr:
        *(char **)store = data->qppd_Str_DOSDev;
        break;

    case MUIA_QPart_ccPartition_Position:
        *store = (IPTR)data->qppd_Part_Pos;
        break;

    case MUIA_QPart_ccPartition_Start:
        {
            ULONG SecCylinder = (data->qppd_Geom->dg_TotalSectors / data->qppd_Geom->dg_Cylinders), SecSize = data->qppd_Geom->dg_SectorSize;
            D(bug("[QuickPart:Part.G] %u sectors per cyl, %u bytes\n", SecCylinder, SecSize));
            *(UQUAD *)store = data->qppd_Part_DE.de_LowCyl * data->qppd_Part_DE.de_Surfaces * data->qppd_Part_DE.de_BlocksPerTrack;
            D(bug("[QuickPart:Part.G] Start = %llu\n", *(UQUAD *)store));
            break;
        }
    case MUIA_QPart_ccPartition_End:
        {
            ULONG SecCylinder = (data->qppd_Geom->dg_TotalSectors / data->qppd_Geom->dg_Cylinders), SecSize = data->qppd_Geom->dg_SectorSize;
            D(bug("[QuickPart:Part.G] %u sectors per cyl, %u bytes\n", SecCylinder, SecSize));
            *(UQUAD *)store = (data->qppd_Part_DE.de_HighCyl + 1) * data->qppd_Part_DE.de_Surfaces * data->qppd_Part_DE.de_BlocksPerTrack - 1;
            D(bug("[QuickPart:Part.G] End = %llu\n", *(UQUAD *)store));
            break;
        }
    default:
        retval = DoSuperMethodA(CLASS, self, (Msg)message);
        break;
    }

    return retval;
}

static IPTR QPPartition__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    D(bug("[QuickPart:Part] DISPOSE (data=%p)\n",data));
    
    return DoSuperMethodA(CLASS, self, message);
}

static IPTR QPPartition__MUIM_Setup(Class * CLASS, Object * self, struct MUIP_Setup *message)
{
    SETUP_INST_DATA;
    IPTR retval;

    D(bug("[QuickPart:Part] %s()\n", __func__));

    retval = DoSuperMethodA(CLASS, self, message);

    if (data->qppd_Object_Part)
        DoMethod(data->qppd_Object_Part, MUIM_Notify, MUIA_Selected, FALSE,
            _app(self), 3, MUIM_Set, MUIA_QPart_ccApp_ActivePart, self);

    return retval;
}

static IPTR QPPartition__MUIM_Cleanup(Class * CLASS, Object * self, struct MUIP_Cleanup *message)
{
    SETUP_INST_DATA;

    D(bug("[QuickPart:Part] %s()\n", __func__));

    if (data->qppd_Object_Part)
        DoMethod(data->qppd_Object_Part, MUIM_KillNotify, MUIA_Selected);

    return DoSuperMethodA(CLASS, self, message);
}

static IPTR QPPartition__MUIM_AskMinMax
(
    Class *CLASS, Object *self, struct MUIP_AskMinMax *message
)
{
    D(bug("[QuickPart:Part] %s()\n", __func__));

    return DoSuperMethodA(CLASS, self, (Msg) message);
}

static IPTR QPPartition__MUIM_Draw
(
    Class *CLASS, Object *self, struct MUIP_Draw *message
)
{
    SETUP_INST_DATA;

    struct Region       *r = NULL;
    APTR                c = (APTR)-1;
    IPTR retval;

    D(bug("[QuickPart:Part] %s()\n", __func__));


    if ((Object *)XGET(_app(self), MUIA_QPart_ccApp_ActivePart) == self)
    {
        struct RastPort *rp = _rp(self);
        struct Rectangle    clipRect;
        UWORD oldDrPt;
        ULONG lastPattern = data->qppd_Pattern << 16 | data->qppd_Pattern;
        data->qppd_Pattern = (lastPattern >> 1) & 0xFFFF;
        
        clipRect.MinX = _mleft(self) + 1;
        clipRect.MinY = _mtop(self) + 1;
        clipRect.MaxX = _mright(self) - 1;
        clipRect.MaxY = _mbottom(self) - 1;

        SetABPenDrMd(rp, _pens(self)[MPEN_SHINE], _pens(self)[MPEN_SHADOW], JAM2);

        oldDrPt = rp->LinePtrn;
        SetDrPt(rp, data->qppd_Pattern);
        Move(rp, _mleft(self), _mtop(self));
        Draw(rp, _mright(self), _mtop(self));
        Draw(rp, _mright(self), _mbottom(self));
        Draw(rp, _mleft(self), _mbottom(self));
        Draw(rp, _mleft(self), _mtop(self));
        SetDrPt(rp, oldDrPt);
        if (!data->qppd_PatEH.ehn_Object)
        {
            struct Window *winWindow = NULL;
            data->qppd_PatEH.ehn_Object = self;
#if (0)
            GET(_win(self), MUIA_Window, &winWindow);
            if (winWindow)
            {
                ModifyIDCMP(winWindow, (winWindow->IDCMPFlags | IDCMP_INTUITICKS));
            }
#endif
            DoMethod(_win(self), MUIM_Window_AddEventHandler, &data->qppd_PatEH);
        }
        r = NewRegion();
        OrRectRegion(r, &clipRect);
    }
    else
    {
        if (data->qppd_PatEH.ehn_Object)
        {
            struct Window *winWindow = NULL;
            DoMethod(_win(self),MUIM_Window_RemEventHandler,&data->qppd_PatEH);
            data->qppd_PatEH.ehn_Object = NULL;
#if (0)
            GET(_win(self), MUIA_Window, &winWindow);
            if (winWindow)
            {
                ModifyIDCMP(winWindow, (winWindow->IDCMPFlags & ~(IDCMP_INTUITICKS)));
            }
#endif
        }
    }

    if (r)
        c = MUI_AddClipRegion(muiRenderInfo(self), r);

    retval = DoSuperMethodA(CLASS, self, (Msg) message);

    if (r)
        MUI_RemoveClipRegion(muiRenderInfo(self), c);
    return retval;
}

static IPTR QPPartition__MUIM_HandleEvent
(
    Class *CLASS, Object *self, struct MUIP_HandleEvent *message
)
{
    D(bug("[QuickPart:Part] %s()\n", __func__));

    MUI_Redraw(self, MADF_DRAWUPDATE);

    return 0;
}

BOOPSI_DISPATCHER(IPTR, QPPartition_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW: 
        return QPPartition__OM_NEW(CLASS, self, (struct opSet *) message);

    case OM_DISPOSE: 
        return QPPartition__OM_DISPOSE(CLASS, self, message);

    case OM_SET:
        return QPPartition__OM_SET(CLASS, self, (struct opSet *) message);

    case OM_GET:
        return QPPartition__OM_GET(CLASS, self,  (struct opGet *) message);

	case MUIM_Setup:
		return QPPartition__MUIM_Setup(CLASS, self, (struct MUIP_Setup *)message);

	case MUIM_Cleanup:
		return QPPartition__MUIM_Cleanup(CLASS, self, (struct MUIP_Cleanup *)message);

    case MUIM_AskMinMax:
        return QPPartition__MUIM_AskMinMax(CLASS, self,  (struct MUIP_AskMinMax *) message);

    case MUIM_Draw:
        return QPPartition__MUIM_Draw(CLASS, self,  (struct MUIP_Draw *) message);

    case MUIM_HandleEvent:
        return QPPartition__MUIM_HandleEvent(CLASS, self,  (struct MUIP_HandleEvent *) message);
    default:     
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END

/*
Copyright  2002-2008, The AROS Development Team. All rights reserved.
$Id$
*/
#ifndef __AROS__
#include "../portable_macros.h"
#define WANDERER_BUILTIN_ICONVOLUMELIST 1 
#else
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

//#define CREATE_FULL_DRAGIMAGE

#if !defined(__AROS__)
#define DRAWICONSTATE DrawIconState
#else
#define DRAWICONSTATE DrawIconStateA
#endif

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
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/layers.h>
#include <proto/muimaster.h>
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

#include <libraries/mui.h>
//#include "muimaster_intern.h"
//#include "support.h"
//#include "imspec.h"
#include "iconlist_attributes.h"
#include "iconlist.h"
#include "iconvolumelist_private.h"

#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #define bug DebugPrintF
#else
  #define  D(...)
#endif
#endif

extern struct Library *MUIMasterBase;

/* sba: taken from SimpleFind3 */

struct NewDosList
{
	struct List       list;
	APTR              pool;
};

struct NewDosNode
{
	struct Node       node;
	STRPTR            name;
	struct Device     *device;
	struct Unit       *unit;
	struct MsgPort    *port;
};

static struct NewDosList *IconVolumeList__CreateDOSList(void)
{
	APTR pool = CreatePool(MEMF_PUBLIC,4096,4096);

D(bug("[IconVolList]: %s()\n", __PRETTY_FUNCTION__));

	if (pool)
	{
		struct NewDosList *ndl = (struct NewDosList*)AllocPooled(pool, sizeof(struct NewDosList));
		if (ndl)
		{
			struct DosList *dl = NULL;
		
			NewList((struct List*)ndl);
			ndl->pool = pool;
		
			dl = LockDosList(LDF_VOLUMES|LDF_READ);
			while(( dl = NextDosEntry(dl, LDF_VOLUMES)))
			{
				STRPTR name;
#ifndef __AROS__
				UBYTE *dosname = (UBYTE*)BADDR(dl->dol_Name);
				LONG len = dosname[0];
				dosname++;
#else
				UBYTE *dosname = dl->dol_Ext.dol_AROS.dol_DevName;
				LONG len = strlen(dosname);
#endif

				if ((name = (STRPTR)AllocPooled(pool, len + 1)))
				{
					struct NewDosNode *ndn = NULL;
	
					name[len] = 0;
					strncpy(name, dosname, len);
	
					if ((ndn = (struct NewDosNode*)AllocPooled(pool, sizeof(*ndn))))
					{
						ndn->name = name;
						#ifdef __AROS__
						ndn->device = dl->dol_Ext.dol_AROS.dol_Device;
						ndn->unit = dl->dol_Ext.dol_AROS.dol_Unit;
						#endif
D(bug("[IconVolList] %s: adding node for '%s' (Device @ 0x%p, Unit @ 0x%p) Type: %d\n", __PRETTY_FUNCTION__, ndn->name, ndn->device, ndn->unit, dl->dol_Type));
D(bug("[IconVolList] %s: Device '%s'\n", __PRETTY_FUNCTION__, ndn->device->dd_Library.lib_Node.ln_Name));
						if (dl->dol_misc.dol_handler.dol_Startup)
						{
							struct FileSysStartupMsg *thisfs_SM = dl->dol_misc.dol_handler.dol_Startup;
D(bug("[IconVolList] %s: Startup msg @ 0x%p\n", __PRETTY_FUNCTION__, thisfs_SM));
D(bug("[IconVolList] %s: Startup Device '%s', Unit %d\n", __PRETTY_FUNCTION__, thisfs_SM->fssm_Device, thisfs_SM->fssm_Unit));
						}
#ifndef __AROS__
						ndn->port = dl->dol_Task;
#else
						ndn->port = NULL;
#endif
						AddTail((struct List*)ndl, (struct Node*)ndn);
					}
				}
			}
			UnLockDosList(LDF_VOLUMES|LDF_READ);

#ifndef __AROS__
			dl = LockDosList(LDF_DEVICES|LDF_READ);
			while(( dl = NextDosEntry(dl, LDF_DEVICES)))
			{
				struct NewDosNode *ndn = NULL;
	
				if (!dl->dol_Task) continue;
	
				ndn = (struct NewDosNode*)GetHead(ndl);
				while ((ndn))
				{
					if (dl->dol_Task == ndn->port)
					{
						STRPTR name;
						UBYTE  *dosname = (UBYTE*)BADDR(dl->dol_Name);
						LONG   len = dosname[0];
	
						if ((name = (STRPTR)AllocPooled(pool, len + 1)))
						{
							name[len] = 0;
							strncpy(name, &dosname[1], len);
						}
	
						ndn->device = name;
						break;
					}
	
					ndn = (struct NewDosNode*)GetSucc(ndn);
				}
			}
			UnLockDosList(LDF_DEVICES|LDF_READ);
#endif
			return ndl;
		}
		DeletePool(pool);
	}
	return NULL;
}

static void IconVolumeList__DestroyDOSList(struct NewDosList *ndl)
{
D(bug("[IconVolList]: %s()\n", __PRETTY_FUNCTION__));
	if (ndl && ndl->pool) DeletePool(ndl->pool);
}
/* sba: End SimpleFind3 */


/**************************************************************************
OM_NEW
**************************************************************************/
IPTR IconVolumeList__OM_NEW(struct IClass *CLASS, Object *obj, struct opSet *message)
{
	struct IconDrawerList_DATA   *data = NULL;
//    struct TagItem  	        *tag = NULL,
//                                *tags = NULL;

D(bug("[IconVolList]: %s()\n", __PRETTY_FUNCTION__));

	obj = (Object *)DoSuperNewTags(CLASS, obj, NULL,
		TAG_MORE, (IPTR) message->ops_AttrList);

	if (!obj) return FALSE;

	data = INST_DATA(CLASS, obj);

	SET(obj, MUIA_IconList_DisplayFlags, ICONLIST_DISP_VERTICAL);
	SET(obj, MUIA_IconList_SortFlags, 0);

	return (IPTR)obj;
}

/**************************************************************************
MUIM_IconList_Update
**************************************************************************/
IPTR IconVolumeList__MUIM_IconList_Update(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Update *message)
{
	//struct IconVolumeList_DATA *data = INST_DATA(CLASS, obj);
	struct IconEntry  *this_Icon = NULL;
	struct NewDosList *ndl = NULL;

D(bug("[IconVolList]: %s()\n", __PRETTY_FUNCTION__));

	DoSuperMethodA(CLASS, obj, (Msg) message);
	
	DoMethod(obj, MUIM_IconList_Clear);

	/* If not in setup do nothing */
#warning "TODO: Handle MADF_SETUP"
//	if (!(_flags(obj) & MADF_SETUP)) return 1;

	if ((ndl = IconVolumeList__CreateDOSList()))
	{
		struct NewDosNode *nd = NULL;
		struct	MsgPort 			*mp=NULL;
			
		mp = CreateMsgPort();
		if (mp)
		{	
			#ifdef __AROS__
			ForeachNode(ndl, nd)
			#else
			Foreach_Node(ndl, nd);
			#endif
			{
				char buf[300];
				if (nd->name)
				{
					strcpy(buf, nd->name);
					strcat(buf, ":Disk");
			
					if ((this_Icon = (struct IconEntry *)DoMethod(obj, MUIM_IconList_CreateEntry, (IPTR)buf, (IPTR)nd->name, (IPTR)NULL, (IPTR)NULL)) == NULL)
					{
D(bug("[IconVolList] %s: Failed to Add IconEntry for '%s'\n", __PRETTY_FUNCTION__, nd->name));
					}
					else
					{
						this_Icon->ile_IconListEntry.type == ST_ROOT;

						if (!(this_Icon->ile_Flags & ICONENTRY_FLAG_HASICON))
							this_Icon->ile_Flags |= ICONENTRY_FLAG_HASICON;

						if ((strcasecmp(nd->name, "Ram Disk")) == 0)
						{
D(bug("[IconVolList] %s: Setting Ram Disk's icon node priority to 5\n", __PRETTY_FUNCTION__));
							this_Icon->ile_IconNode.ln_Pri = 5;   // Special dirs get Priority 5
						}
						else
						{
							this_Icon->ile_IconNode.ln_Pri = 1;   // Fixed Media get Priority 1

/*							struct IOExtTD *ioreq = NULL;
							struct DriveGeometry dg;

							if (ioreq = (struct IOStdReq *)CreateIORequest(mp, sizeof(struct IOStdReq)))
							{
								if (OpenDevice(boot_Device, boot_Unit, (struct IORequest *)ioreq, 0) == 0)
								{
									ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
									ioreq->iotd_Req.io_Data = &dg;
									ioreq->iotd_Req.io_Length = sizeof(struct DriveGeometry);
									DoIO((struct IORequest *)ioreq);
									if (dg.dg_Flags & DGF_REMOVABLE)
									{
										this_Icon->ile_IconNode.ln_Pri = 0;   // Removable Media get Priority 0
									}
									CloseDevice((struct IORequest *)ioreq);
								}
								DeleteIORequest((struct IORequest *)ioreq);
							}*/
						}
					}
				}
			}
			IconVolumeList__DestroyDOSList(ndl);
		}
	}

	/* default display/sorting flags */
	DoMethod(obj, MUIM_IconList_Sort);

	return 1;
}

#if WANDERER_BUILTIN_ICONVOLUMELIST
BOOPSI_DISPATCHER(IPTR, IconVolumeList_Dispatcher, CLASS, obj, message)
{
    	#ifdef __AROS__
    	switch (message->MethodID)
    	#else
    	struct IClass *CLASS = cl;
    	Msg message = msg;

    	switch (msg->MethodID)
    	#endif
	{
		case OM_NEW: return IconVolumeList__OM_NEW(CLASS, obj, (struct opSet *)message);
		case MUIM_IconList_Update: return IconVolumeList__MUIM_IconList_Update(CLASS,obj,(APTR)message);
	}

	return DoSuperMethodA(CLASS, obj, message);
}
BOOPSI_DISPATCHER_END

#ifdef __AROS__
/* Class descriptor. */
const struct __MUIBuiltinClass _MUI_IconVolumeList_desc = { 
	MUIC_IconVolumeList, 
	MUIC_IconList, 
	sizeof(struct IconVolumeList_DATA), 
	(void*)IconVolumeList_Dispatcher
};
#endif
#endif

#ifndef __AROS__
struct MUI_CustomClass  *initIconVolumeListClass(void)
{
  return (struct MUI_CustomClass *) MUI_CreateCustomClass(NULL,  NULL, IconList_Class, sizeof(struct IconVolumeList_DATA), ENTRY(IconVolumeList_Dispatcher));
}
#endif

/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <workbench/icon.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/icon.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "imspec.h"

extern struct Library *MUIMasterBase;

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

struct IconEntry
{
    struct MinNode node;
    struct IconList_Entry entry;

    /* the following could be removed because they are inside IconList_Entry */
    char *filename;
    char *label;
    void *udata;

    struct DiskObject *dob; /* The icons disk objects */

    int x,y;
    int width,height;
    int selected;
};

static void *Node_Next(APTR node)
{
    if(node == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ->mln_Succ == NULL)
	return NULL;
    return ((struct MinNode*)node)->mln_Succ;
}

static void *List_First(APTR list)
{
    if( !((struct MinList*)list)->mlh_Head) return NULL;
    if(((struct MinList*)list)->mlh_Head->mln_Succ == NULL) return NULL;
    return ((struct MinList*)list)->mlh_Head;
}

static char *StrDup(char *x)
{
    char *dup;
    if (!x) return NULL;
    dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC);
    if (dup) CopyMem((x), dup, strlen(x) + 1);
    return dup;
}

struct MUI_IconData
{
    APTR pool; /* Pool to allocate data from */

    struct MinList icon_list; /* IconEntry */
    int view_x,view_y; /* the leftmost/upper coordinates of the view */
    int view_width,view_height; /* dimensions of the view (_mwidth(obj) and _mheight(obj)) */
    int mouse_pressed;

    struct MUI_EventHandlerNode ehn;

    LONG touch_x;
    LONG touch_y;

    LONG click_x;
    LONG click_y;

    struct IconEntry *first_selected; /* the icon which has been selected first or NULL */

    /* DoubleClick stuff */
    ULONG last_secs;
    ULONG last_mics;
    struct IconEntry *last_selected;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR IconList_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_IconData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperNew(cl, obj,
	MUIA_Dropable, TRUE,
    	TAG_MORE, msg->ops_AttrList);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);
    NewList((struct List*)&data->icon_list);

//    set(obj,MUIA_FillArea,FALSE);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }

    data->pool =  CreatePool(0,4096,4096);
    if (!data->pool)
    {
	CoerceMethod(cl,obj,OM_DISPOSE);
	return NULL;
    }

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR IconList_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct IconEntry *node;

    node = List_First(&data->icon_list);
    while (node)
    {
	if (node->dob) FreeDiskObject(node->dob);
	node = Node_Next(node);
    }
    
    if (data->pool) DeletePool(data->pool);

    DoSuperMethodA(cl,obj,msg);
    return 0;
}


/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR IconList_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_IconData   *data = INST_DATA(cl, obj);
    struct TagItem  	    *tag, *tags;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG IconList_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)
    struct MUI_IconData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    }

    if (DoSuperMethodA(cl, obj, (Msg) msg)) return 1;
    return 0;
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG IconList_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    if (!DoSuperMethodA(cl, obj, (Msg) msg)) return 0;

    DoMethod(obj,MUIM_IconList_Update);

    DoMethod(_win(obj),MUIM_Window_AddEventHandler, &data->ehn);

    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG IconList_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);

    struct IconEntry *node;

    node = List_First(&data->icon_list);
    while (node)
    {
	if (node->dob)
	{
	    FreeDiskObject(node->dob);
	    node->dob = NULL;
	}
	node = Node_Next(node);
    }


    DoMethod(_win(obj),MUIM_Window_RemEventHandler, &data->ehn);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG IconList_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    ULONG rc = DoSuperMethodA(cl, obj, (Msg) msg);

//    msg->MinMaxInfo->MinWidth  += 0;
//    msg->MinMaxInfo->MinHeight += 0;

    msg->MinMaxInfo->DefWidth  += 200;
    msg->MinMaxInfo->DefHeight += 180;

    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return rc;
}

/**************************************************************************
 MUIM_Layout
**************************************************************************/
static ULONG IconList_Layout(struct IClass *cl, Object *obj,struct MUIP_Layout *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl,obj,(Msg)msg);
    return rc;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG IconList_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    APTR clip;
    struct IconEntry *node;

    DoSuperMethodA(cl, obj, (Msg) msg);

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
			   _mwidth(obj), _mheight(obj));

    node = List_First(&data->icon_list);
    while (node)
    {
	if (node->dob && node->x != NO_ICON_POSITION && node->y != NO_ICON_POSITION)
	{
	    SetABPenDrMd(_rp(obj),_pens(obj)[MPEN_TEXT],0,JAM1);
	    DrawIconState(_rp(obj),node->dob,node->label,_mleft(obj) - data->view_x + node->x, _mtop(obj) - data->view_y + node->y, node->selected?IDS_SELECTED:IDS_NORMAL, ICONDRAWA_EraseBackground, FALSE, TAG_DONE);
	}
	node = Node_Next(node);
    }

    MUI_RemoveClipping(muiRenderInfo(obj),clip);

    return 0;
}

/**************************************************************************
 MUIM_IconList_Refresh
 Implemented by subclasses
**************************************************************************/
static ULONG IconList_Update(struct IClass *cl, Object *obj, struct MUIP_IconList_Update *msg)
{
    return 1;
}

/**************************************************************************
 MUIM_IconList_Clear
**************************************************************************/
static ULONG IconList_Clear(struct IClass *cl, Object *obj, struct MUIP_IconList_Clear *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct IconEntry *node;

    while ((node = (struct IconEntry*)RemTail((struct List*)&data->icon_list)))
    {
	if (node->dob) FreeDiskObject(node->dob);
	if (node->label) FreePooled(data->pool,node->label,strlen(node->label)+1);
	if (node->filename) FreePooled(data->pool,node->filename,strlen(node->filename)+1);
        FreePooled(data->pool,node,sizeof(struct IconEntry));
    }

    data->first_selected = NULL;
    data->view_x = data->view_y = 0;

    MUI_Redraw(obj,MADF_DRAWOBJECT);
    return 1;
}

/**************************************************************************
 MUIM_IconList_Add.
 Returns 0 on failure otherwise 1
**************************************************************************/
static IPTR IconList_Add(struct IClass *cl, Object *obj, struct MUIP_IconList_Add *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct IconEntry *entry;
    struct DiskObject *dob;
    struct Rectangle rect;

    if (!(dob = GetIconTagList(msg->filename,TAG_DONE)))
	return 0;

    if (!(entry = AllocPooled(data->pool,sizeof(struct IconEntry))))
    {
	FreeDiskObject(dob);
	return 0;
    }

    memset(entry,0,sizeof(struct IconEntry));

    if (!(entry->filename = AllocPooled(data->pool,strlen(msg->filename)+1)))
    {
	FreePooled(data->pool,entry,sizeof(struct IconEntry));
	FreeDiskObject(dob);
	return 0;
    }

    strcpy(entry->filename,msg->filename);

    if (!(entry->label = AllocPooled(data->pool,strlen(msg->label)+1)))
    {
    	FreePooled(data->pool,entry->filename,strlen(entry->filename)+1);
	FreePooled(data->pool,entry,sizeof(struct IconEntry));
	FreeDiskObject(dob);
	return 0;
    }

    strcpy(entry->label,msg->label);

    GetIconRectangleA(NULL,dob,NULL,&rect,NULL);

    entry->dob = dob;
    entry->udata = msg->udata;

    entry->x = dob->do_CurrentX;
    entry->y = dob->do_CurrentY;
    entry->width = rect.MaxX - rect.MinX;
    entry->height = rect.MaxY - rect.MinY;
    if (entry->x < data->view_x && entry->x != NO_ICON_POSITION) data->view_x = entry->x;
    if (entry->y < data->view_y && entry->y != NO_ICON_POSITION) data->view_y = entry->y;
    strcpy(entry->filename,msg->filename);
    AddTail((struct List*)&data->icon_list,(struct Node*)entry);
    return 1;
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG IconList_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
    	LONG mx = msg->imsg->MouseX - _mleft(obj);
    	LONG my = msg->imsg->MouseY - _mtop(obj);
	switch (msg->imsg->Class)
	{
	    case    IDCMP_MOUSEBUTTONS:
		    if (msg->imsg->Code == SELECTDOWN)
		    {
			if (mx >= 0 && mx < _width(obj) && my >= 0 && my < _height(obj))
			{
			    struct IconEntry *node;

			    node = List_First(&data->icon_list);
			    while (node)
			    {
			    	node->selected = 0;
				node = Node_Next(node);
			    }
			    data->first_selected = NULL;

			    DoMethod(_win(obj),MUIM_Window_RemEventHandler, &data->ehn);
			    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			    DoMethod(_win(obj),MUIM_Window_AddEventHandler, &data->ehn);
			    data->mouse_pressed = 1;

			    node = List_First(&data->icon_list);
			    while (node)
			    {
			    	if (mx >= node->x - data->view_x && mx < node->x - data->view_x + node->width &&
			    	    my >= node->y - data->view_y && my < node->y - data->view_y + node->height)
			    	{
				    node->selected = 1;
				    data->first_selected = node;
				    break;
				}
				node = Node_Next(node);
			    }

			    if (DoubleClick(data->last_secs, data->last_mics, msg->imsg->Seconds, msg->imsg->Micros) && data->last_selected == node)
			    {
				set(obj,MUIA_IconList_DoubleClick, TRUE);
			    } else
			    {
				data->last_selected = node;
				data->last_secs = msg->imsg->Seconds;
				data->last_mics = msg->imsg->Micros;
			    }

			    MUI_Redraw(obj,MADF_DRAWOBJECT);

			    data->click_x = mx;
			    data->click_y = my;

			    return MUI_EventHandlerRC_Eat;
			}
		    } else
		    {
			if (msg->imsg->Code == SELECTUP && data->mouse_pressed)
			{
			    DoMethod(_win(obj),MUIM_Window_RemEventHandler, &data->ehn);
			    data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
			    DoMethod(_win(obj),MUIM_Window_AddEventHandler, &data->ehn);
			    data->mouse_pressed = 0;
			    return MUI_EventHandlerRC_Eat;
			}
		    }
		    break;

	    case    IDCMP_MOUSEMOVE:
		    if (data->mouse_pressed)
		    {
		    	int move_x = mx;
		    	int move_y = my;
		    	
			if (data->first_selected && (abs(move_x - data->click_x) >= 2 || abs(move_y - data->click_y) >= 2))
			{
			    DoMethod(_win(obj),MUIM_Window_RemEventHandler, &data->ehn);
			    data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
			    DoMethod(_win(obj),MUIM_Window_AddEventHandler, &data->ehn);

			    data->touch_x = move_x + data->view_x - data->first_selected->x;
			    data->touch_y = move_y + data->view_y - data->first_selected->y;
			    DoMethod(obj,MUIM_DoDrag, data->touch_x, data->touch_y, 0);
			}

			return MUI_EventHandlerRC_Eat;
		    }
		    break;
	}
    }

    return 0;
}

/**************************************************************************
 MUIM_IconList_NextSelected
**************************************************************************/
static ULONG IconList_NextSelected(struct IClass *cl, Object *obj, struct MUIP_IconList_NextSelected *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct IconEntry *node;
    struct IconList_Entry *ent;

    if (!msg->entry) return NULL;
    ent = *msg->entry;
    if (((IPTR)ent) == MUIV_IconList_NextSelected_Start)
    {
	if (!(node = data->last_selected))
	{
	    *msg->entry = (struct IconList_Entry*)MUIV_IconList_NextSelected_End;
	} else
	{
	    node->entry.filename = node->filename;
	    node->entry.label = node->label;
	    node->entry.udata = node->udata;
	    *msg->entry = &node->entry;
	}
	return 0;
    }
    

    node = List_First(&data->icon_list); /* not really necessary but it avoids compiler warnings */

    node = (struct IconEntry*)((char*)ent) - ((char*)(&node->entry) - (char*)node);
    node = Node_Next(node);

    while (node)
    {
	if (node->selected)
	{
	    node->entry.filename = node->filename;
	    node->entry.label = node->label;
	    node->entry.udata = node->udata;
	    *msg->entry = &node->entry;
	    return 0;
	}
	node = Node_Next(node);
    }

    *msg->entry = (struct IconList_Entry*)MUIV_IconList_NextSelected_End;

    return NULL;
}

/**************************************************************************
 MUIM_CreateDragImage
**************************************************************************/
static ULONG IconList_CreateDragImage(struct IClass *cl, Object *obj, struct MUIP_CreateDragImage *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct MUI_DragImage *img;

    if (!data->first_selected) DoSuperMethodA(cl,obj,(Msg)msg);

    if ((img = (struct MUI_DragImage *)AllocVec(sizeof(struct MUIP_CreateDragImage),MEMF_CLEAR)))
    {
    	struct IconEntry *node;
	LONG depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap,BMA_DEPTH);

	node = data->first_selected;

    	img->width = node->width;
    	img->height = node->height;

    	if ((img->bm = AllocBitMap(img->width,img->height,depth,BMF_MINPLANES,_screen(obj)->RastPort.BitMap)))
    	{
    	    struct RastPort temprp;
    	    InitRastPort(&temprp);
    	    temprp.BitMap = img->bm;

	    DrawIconState(&temprp,node->dob,NULL,0,0, node->selected?IDS_SELECTED:IDS_NORMAL, ICONDRAWA_EraseBackground, TRUE, TAG_DONE);
    	}

    	img->touchx = msg->touchx;
    	img->touchy = msg->touchy;
    	img->flags = 0;
    }
    return (ULONG)img;
}

/**************************************************************************
 MUIM_DeleteDragImage
**************************************************************************/
static ULONG IconList_DeleteDragImage(struct IClass *cl, Object *obj, struct MUIP_DeleteDragImage *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);

    if (!data->first_selected) return DoSuperMethodA(cl,obj,(Msg)msg);

    if (msg->di)
    {
	if (msg->di->bm) FreeBitMap(msg->di->bm);
	FreeVec(msg->di);
    }
    return NULL;
}

/**************************************************************************
 MUIM_DragQuery
**************************************************************************/
static ULONG IconList_DragQuery(struct IClass *cl, Object *obj, struct MUIP_DragQuery *msg)
{
    if (msg->obj == obj) return MUIV_DragQuery_Accept;
    return MUIV_DragQuery_Refuse;
}

/**************************************************************************
 MUIM_DragDrop
**************************************************************************/
static ULONG IconList_DragDrop(struct IClass *cl, Object *obj, struct MUIP_DragDrop *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);

    if (msg->obj == obj)
    {
	if (data->first_selected)
	{
	    data->first_selected->x = msg->x - _mleft(obj) + data->view_x - data->touch_x;
	    data->first_selected->y = msg->y - _mtop(obj) + data->view_y - data->touch_y;
	    MUI_Redraw(obj,MADF_DRAWOBJECT);
	}
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

#ifndef _AROS
__asm IPTR IconList_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,IconList_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return IconList_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return IconList_Dispose(cl,obj, msg);
	case OM_SET: return IconList_Set(cl,obj,(struct opSet *)msg);
	case OM_GET: return IconList_Get(cl,obj,(struct opGet *)msg);
	case MUIM_Setup: return IconList_Setup(cl,obj,(struct MUIP_Setup *)msg);
	case MUIM_Cleanup: return IconList_Cleanup(cl,obj,(struct MUIP_Cleanup *)msg);
	case MUIM_AskMinMax: return IconList_AskMinMax(cl,obj,(struct MUIP_AskMinMax *)msg);
	case MUIM_Draw: return IconList_Draw(cl,obj,(struct MUIP_Draw *)msg);
	case MUIM_Layout: return IconList_Layout(cl,obj,(struct MUIP_Layout *)msg);
	case MUIM_HandleEvent: return IconList_HandleEvent(cl,obj,(struct MUIP_HandleEvent *)msg);
	case MUIM_CreateDragImage: return IconList_CreateDragImage(cl,obj,(APTR)msg);
	case MUIM_DeleteDragImage: return IconList_DeleteDragImage(cl,obj,(APTR)msg);
	case MUIM_DragQuery: return IconList_DragQuery(cl,obj,(APTR)msg);
	case MUIM_DragDrop: return IconList_DragDrop(cl,obj,(APTR)msg);

	case MUIM_IconList_Update: return IconList_Update(cl,obj,(APTR)msg);
	case MUIM_IconList_Clear: return IconList_Clear(cl,obj,(APTR)msg);
	case MUIM_IconList_Add: return IconList_Add(cl,obj,(APTR)msg);
	case MUIM_IconList_NextSelected: return IconList_NextSelected(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

struct MUI_IconDrawerData
{
    char *drawer;
};

/**************************************************************************
 Read icons in
**************************************************************************/
static int ReadIcons(struct IClass *cl, Object *obj)
{
    struct MUI_IconDrawerData *data = INST_DATA(cl, obj);
    BPTR lock;
    struct ExAllControl *eac;
    struct ExAllData *ead, *entry;
    LONG more;
    BPTR olddir;
    char pattern[40];
    char filename[256];

    if (!data->drawer) return 1;
    lock = Lock(data->drawer,ACCESS_READ);
    if (!lock) return 0;

    eac = (struct ExAllControl*)AllocDosObject(DOS_EXALLCONTROL,NULL);
    if (!eac)
    {
	UnLock(lock);
	return 0;
    }

    ead = AllocVec(1024,0);
    if (!ead)
    {
	FreeDosObject(DOS_EXALLCONTROL,eac);
	UnLock(lock);
	return 0;
    }

    ParsePatternNoCase("#?.info",pattern,sizeof(pattern));
    eac->eac_MatchString = pattern;
    eac->eac_LastKey = 0;

    olddir = CurrentDir(lock);

    do
    {
    	more = ExAll(lock,ead,1024,ED_TYPE,eac);
	if ((!more) && (IoErr() != ERROR_NO_MORE_ENTRIES)) break;
	if (eac->eac_Entries == 0) continue;

	entry = ead;
	do
	{
	    strcpy(filename,ead->ed_Name);
	    filename[strlen(filename)-5]=0;

	    if (Stricmp(filename,"Disk")) /* skip disk.info */
	    {
		char buf[512];
		strcpy(buf,data->drawer);
		AddPart(buf,filename,sizeof(buf));

		if (!(DoMethod(obj,MUIM_IconList_Add,buf,filename,NULL /* udata */)))
		{
		}

#if 0
		dob = GetIconTagList(filename,TAG_DONE);
		if (dob)
		{
	    	    if (!(DoMethod(obj,MUIM_IconList_Add,dob,filename,NULL/* udata */))) /* control of dob is given to super class */
	    	    {
	    	        /* The DiskObject couldn't be added so super class also has no control over it */
	    	        FreeDiskObject(dob);
	    	    }
	    	}
#endif
	    }

	    ead = ead->ed_Next;
	}   while (ead);
    } while (more);

    CurrentDir(olddir);

    FreeVec(ead);
    FreeDosObject(DOS_EXALLCONTROL,eac);
    UnLock(lock);
    return 1;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR IconDrawerList_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_IconDrawerData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperNew(cl, obj,
    	TAG_MORE, msg->ops_AttrList);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_IconDrawerList_Drawer:
		    data->drawer = StrDup((char*)tag->ti_Data);
		    break;
    	}
    }

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR IconDrawerList_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_IconDrawerData *data = INST_DATA(cl, obj);

    if (data->drawer) FreeVec(data->drawer);
    return DoSuperMethodA(cl,obj,msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR IconDrawerList_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_IconDrawerData   *data = INST_DATA(cl, obj);
    struct TagItem  	    *tag, *tags;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_IconDrawerList_Drawer:
		    if (data->drawer) FreeVec(data->drawer);
		    data->drawer = StrDup((char*)tag->ti_Data);
		    DoMethod(obj,MUIM_IconList_Update);
		    break;
    	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG IconDrawerList_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)
    struct MUI_IconDrawerData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
	case MUIA_IconDrawerList_Drawer: STORE = (unsigned long)data->drawer; return 1;
    }

    if (DoSuperMethodA(cl, obj, (Msg) msg)) return 1;
    return 0;
#undef STORE
}

/**************************************************************************
 MUIM_IconList_Update
**************************************************************************/
ULONG IconDrawerList_Update(struct IClass *cl, Object *obj, struct MUIP_IconDrawerList *msg)
{
    struct MUI_IconDrawerData *data = INST_DATA(cl, obj);
    struct IconEntry *node;
    DoMethod(obj,MUIM_IconList_Clear);

    /* If not in setup do nothing */
    if (!(_flags(obj)&MADF_SETUP)) return 1;
    ReadIcons(cl,obj);

    MUI_Redraw(obj,MADF_DRAWOBJECT);
}

#ifndef _AROS
__asm IPTR IconDrawerList_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,IconDrawerList_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return IconDrawerList_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return IconDrawerList_Dispose(cl,obj,msg);
	case OM_SET: return IconDrawerList_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return IconDrawerList_Get(cl, obj, (struct opGet *)msg);
	case MUIM_IconList_Update: return IconDrawerList_Update(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/* sba: taken from SimpleFind3 */

struct NewDosList
{
	struct MinList list;
	APTR pool;
};

struct NewDosNode
{
    struct MinNode node;
    STRPTR name;
    STRPTR device;
    struct MsgPort *port;
};

static struct NewDosList *DosList_Create(void)
{
	APTR pool = CreatePool(MEMF_PUBLIC,4096,4096);
	if (pool)
	{
		struct NewDosList *ndl = (struct NewDosList*)AllocPooled(pool,sizeof(struct NewDosList));
		if (ndl)
		{
			struct DosList *dl;

			NewList((struct List*)ndl);
			ndl->pool = pool;

			dl = LockDosList(LDF_VOLUMES|LDF_READ);
			while(( dl = NextDosEntry(dl, LDF_VOLUMES)))
			{
				STRPTR name;
				UBYTE *dosname = (UBYTE*)BADDR(dl->dol_Name);
				LONG len = dosname[0];

				if ((name = (STRPTR)AllocPooled(pool, len+1)))
				{
					struct NewDosNode *ndn;

					name[len] = 0;
					strncpy(name,&dosname[1],len);

					if ((ndn = (struct NewDosNode*)AllocPooled(pool, sizeof(*ndn))))
					{
						ndn->name = name;
						ndn->device = NULL;
						ndn->port = dl->dol_Task;
						AddTail((struct List*)ndl,(struct Node*)ndn);
					}
				}
			}
			UnLockDosList(LDF_VOLUMES|LDF_READ);

			dl = LockDosList(LDF_DEVICES|LDF_READ);
			while(( dl = NextDosEntry(dl, LDF_DEVICES)))
			{
				struct NewDosNode *ndn;

				if (!dl->dol_Task) continue;

				ndn = (struct NewDosNode*)List_First(ndl);
				while ((ndn))
				{
					if (dl->dol_Task == ndn->port)
					{
						STRPTR name;
						UBYTE *dosname = (UBYTE*)BADDR(dl->dol_Name);
						LONG len = dosname[0];

						if ((name = (STRPTR)AllocPooled(pool, len+1)))
						{
							name[len] = 0;
							strncpy(name,&dosname[1],len);
						}

						ndn->device = name;
						break;
					}

					ndn = (struct NewDosNode*)Node_Next(ndn);
				}
			}
			UnLockDosList(LDF_DEVICES|LDF_READ);

			return ndl;
		}
		DeletePool(pool);
	}
	return NULL;
}

static void DosList_Dispose(struct NewDosList *ndl)
{
	if (ndl && ndl->pool) DeletePool(ndl->pool);
}
/* sba: End SimpleFind3 */


struct MUI_IconVolumneData
{
    int dummy;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR IconVolumeList_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_IconDrawerData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperNew(cl, obj,
    	TAG_MORE, msg->ops_AttrList);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }

    return (IPTR)obj;
}

/**************************************************************************
 MUIM_IconList_Update
**************************************************************************/
ULONG IconVolumeList_Update(struct IClass *cl, Object *obj, struct MUIP_IconDrawerList *msg)
{
    struct MUI_IconVolumeData *data = INST_DATA(cl, obj);
    struct IconEntry *node;
    struct NewDosList *ndl;
    DoMethod(obj,MUIM_IconList_Clear);

    /* If not in setup do nothing */
    if (!(_flags(obj)&MADF_SETUP)) return 1;

    if ((ndl = DosList_Create()))
    {
	struct NewDosNode *nd = List_First(ndl);
	while (nd)
	{
	    char buf[300];
	    if (nd->name)
	    {
		strcpy(buf,nd->name);
		strcat(buf,":Disk");

		if (!(DoMethod(obj,MUIM_IconList_Add,buf,nd->name,NULL/* udata */)))
		{
		}
	    }
	    nd = Node_Next(nd);
	}
	DosList_Dispose(ndl);
    }

    MUI_Redraw(obj,MADF_DRAWOBJECT);
}


#ifndef _AROS
__asm IPTR IconVolumeList_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,IconVolumeList_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return IconVolumeList_New(cl, obj, (struct opSet *)msg);
	case MUIM_IconList_Update: return IconVolumeList_Update(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_IconList_desc = { 
    MUIC_IconList, 
    MUIC_Area, 
    sizeof(struct MUI_IconData), 
    (void*)IconList_Dispatcher 
};

const struct __MUIBuiltinClass _MUI_IconDrawerList_desc = { 
    MUIC_IconDrawerList, 
    MUIC_IconList, 
    sizeof(struct MUI_IconDrawerData), 
    (void*)IconDrawerList_Dispatcher 
};

const struct __MUIBuiltinClass _MUI_IconVolumeList_desc = { 
    MUIC_IconVolumeList, 
    MUIC_IconList, 
    sizeof(struct MUI_IconVolumneData), 
    (void*)IconVolumeList_Dispatcher
};


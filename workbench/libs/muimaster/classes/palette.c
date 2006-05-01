/*
    Copyright  2002, The AROS Development Team.
    All rights reserved.

    $Id: palette.c 21734 2006-04-09 23:22:13Z darius brewka $
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/colorwheel.h>
#include <utility/hooks.h>
#include "gadgets/colorwheel.h"
#include <gadgets/gradientslider.h>
#include "intuition/icclass.h"
#include "intuition/gadgetclass.h"
#include <string.h>
#include <stdio.h>

#include "debug.h"

#include "mui.h"
#include "imspec.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"
#include "palette.h"
#include "palette_private.h"

#define ColorWheelBase data->colorwheelbase

extern struct Library *MUIMasterBase;

static LONG display_func(struct Hook *hook, char **array, struct MUI_Palette_Entry *entry)
{
    struct MUI_PaletteData *data = hook->h_Data;
    
    char buf[20];

    if (data->names) {  /* does any strings exist */
        *array++ = data->names[(int) array[-1]];  /*then display user names */
    } else {
        sprintf(buf,"Color %ld",(array[-1]+1));   /* if nos show default color names */
        *array++ = buf;
    }
    return 0;
}

static void NotifyGun(Object *obj, struct MUI_PaletteData *data, LONG gun)
{
    static Tag guntotag[3] =
    {
        MUIA_Coloradjust_Red,
        MUIA_Coloradjust_Green,
        MUIA_Coloradjust_Blue
    };
    
    struct TagItem tags[] =
    {
        {0                   , 0},
        {MUIA_Coloradjust_RGB, 0},
        {TAG_DONE   	    	    	    	    }
    };

    tags[0].ti_Tag = guntotag[gun];
    tags[0].ti_Data = data->rgb[gun];
    tags[1].ti_Data = (IPTR)data->rgb;
    
    CoerceMethod(data->notifyclass, obj, OM_SET, (IPTR)tags, NULL);
}

static LONG setcolor_func(struct Hook *hook, APTR *obj, ULONG *notify)
{
    ULONG   val;
    ULONG mode = *notify++;
    ULONG gun = *notify++;
    struct MUI_PaletteData *data = (struct MUI_PaletteData *) *notify++;
    
    LONG entrie = XGET(data->list, MUIA_List_Active);
    if ((entrie < 0) || (entrie >= data->numentries)) return 0;
    if (mode == 1) {
        if (data->numentries > 0) {
            val = XGET(data->list, MUIA_List_Active);
            ULONG r = data->entries[entrie].mpe_Red;
            ULONG g = data->entries[entrie].mpe_Green;
            ULONG b = data->entries[entrie].mpe_Blue;
            nnset(data->coloradjust, MUIA_Coloradjust_Red, r);
            nnset(data->coloradjust, MUIA_Coloradjust_Green, g);
            nnset(data->coloradjust, MUIA_Coloradjust_Blue, b);
            data->rgb[0] = r;
            data->rgb[1] = g;
            data->rgb[2] = b;
            NotifyGun(obj, data, gun);
        }
    } else if (mode == 2) {
        data->entries[entrie].mpe_Red = XGET(data->coloradjust, MUIA_Coloradjust_Red);
        data->entries[entrie].mpe_Green = XGET(data->coloradjust, MUIA_Coloradjust_Green);
        data->entries[entrie].mpe_Blue = XGET(data->coloradjust, MUIA_Coloradjust_Blue);
    }
    return 0;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
IPTR Palette__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PaletteData      *data;
    struct TagItem  	        *tag, *tags;
    struct MUI_Palette_Entry    *e;
    Object *list, *coloradjust;
    int i;

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        GroupFrame,
        MUIA_Background,  MUII_ButtonBack,
        MUIA_Group_Horiz, TRUE,
        Child, list = ListviewObject,
            MUIA_Listview_List, ListObject,
            End,
        End,
        Child, coloradjust = ColoradjustObject,
        End,
    TAG_MORE, (IPTR) msg->ops_AttrList);

    if (obj == NULL) return NULL;
    
    data = INST_DATA(cl, obj);
    
    data->list = list;
    data->coloradjust = coloradjust;
        
    data->display_hook.h_Entry  	= HookEntry;
    data->display_hook.h_SubEntry 	= (HOOKFUNC)display_func;
    data->display_hook.h_Data   	= data;
    data->setcolor_hook.h_Entry  	= HookEntry;
    data->setcolor_hook.h_SubEntry 	= (HOOKFUNC)setcolor_func;
    data->setcolor_hook.h_Data   	= data;

    nnset(list, MUIA_List_DisplayHook, (IPTR)&data->display_hook);

    data->entries = GetTagData(MUIA_Palette_Entries, 0, msg->ops_AttrList);
    data->names = GetTagData(MUIA_Palette_Names, 0, msg->ops_AttrList);
    data->group = GetTagData(MUIA_Palette_Groupable, 0, msg->ops_AttrList);

    data->numentries = 0;
    e = data->entries;
    if (e) {
        data->numentries = 0;
        while(e->mpe_ID != MUIV_Palette_Entry_End) {
            data->numentries++;
            e++;
        }
    }

    data->notifyclass = cl->cl_Super->cl_Super;

    if (data->numentries > 0) {
        for (i = 0; i < data->numentries; i++) DoMethod(data->list,MUIM_List_InsertSingle,&data->entries[i],MUIV_List_Insert_Bottom);
	
        nnset(data->coloradjust, MUIA_Coloradjust_Red, data->entries[0].mpe_Red);
        nnset(data->coloradjust, MUIA_Coloradjust_Green, data->entries[0].mpe_Green);
        nnset(data->coloradjust, MUIA_Coloradjust_Blue, data->entries[0].mpe_Blue);
        nnset(data->list, MUIA_List_Active, 0);
    }
    data->rgb[0] = data->entries[0].mpe_Red;
    data->rgb[1] = data->entries[0].mpe_Green;
    data->rgb[2] = data->entries[0].mpe_Blue;
    
    DoMethod(data->list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime , obj, 5, MUIM_CallHook, &data->setcolor_hook, 1, 0, data);
    DoMethod(data->coloradjust, MUIM_Notify, MUIA_Coloradjust_Red, MUIV_EveryTime, obj, 5, MUIM_CallHook, &data->setcolor_hook, 2, 0, data);
    DoMethod(data->coloradjust, MUIM_Notify, MUIA_Coloradjust_Green, MUIV_EveryTime, obj, 5, MUIM_CallHook, &data->setcolor_hook, 2, 1, data);
    DoMethod(data->coloradjust, MUIM_Notify, MUIA_Coloradjust_Blue, MUIV_EveryTime, obj, 5, MUIM_CallHook, &data->setcolor_hook, 2, 2, data);
    return (IPTR)obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
IPTR Palette__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PaletteData    *data;
    struct TagItem  	    *tag, *tags;
    
    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
     case    MUIA_Palette_Entries:
         data->entries = (struct MUI_Palette_Entry *) tag->ti_Data;
         break;
		    
     case    MUIA_Palette_Names:
         data->names = (char **) tag->ti_Data;
         break;
         
     case    MUIA_Palette_Groupable:
         data->group = (ULONG) tag->ti_Data;
         break;
	}
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
IPTR Palette__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_PaletteData *data = INST_DATA(cl, obj);
    
    IPTR    	    	      *store = msg->opg_Storage;

    switch(msg->opg_AttrID)
    {
        case	MUIA_Palette_Entries:
            *store =  &data->entries;
            return TRUE;
        case	MUIA_Palette_Names:
            *store =  &data->names;
            return TRUE;
            
        case MUIA_Coloradjust_Red:
            *store = data->rgb[0];
            return TRUE;
	    
        case MUIA_Coloradjust_Green:
            *store = data->rgb[1];
            return TRUE;
	    
        case MUIA_Coloradjust_Blue:
            *store = data->rgb[2];
            return TRUE;
	    
        case MUIA_Coloradjust_RGB:
            *(IPTR **)store = data->rgb;
            return TRUE;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}
/**************************************************************************
 OM_GET
**************************************************************************/

IPTR Palette__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_PaletteData  *data;
    
    data = INST_DATA(cl, obj);
    
    return DoSuperMethodA(cl, obj, msg);
}

#if ZUNE_BUILTIN_PALETTE

BOOPSI_DISPATCHER(IPTR, Palette_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Palette__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_SET: return Palette__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET: return Palette__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_DISPOSE: return Palette__OM_DISPOSE(cl, obj, (struct opGet *)msg);
    default:     return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Palette_desc = {
    MUIC_Palette,
    MUIC_Group, 
    sizeof(struct MUI_PaletteData),
    (void*)Palette_Dispatcher
};
#endif

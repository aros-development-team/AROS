/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>

#include <intuition/classes.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "support.h"
#include "support_classes.h"
#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;

/**************************************************************************
 ...
**************************************************************************/
struct IClass *GetPublicClass(CONST_STRPTR className, struct Library *mb)
{
    struct IClass *cl;
    int i;

    for (i = 0; i < MUIMB(mb)->ClassCount; i++)
    {
      cl = MUIMB(mb)->Classes[i];
      if (cl && !strcmp(cl->cl_ID, className))
      {
        return cl;
      }
    }
    return NULL;
}

/**************************************************************************
 ...
**************************************************************************/
BOOL DestroyClasses(struct Library *MUIMasterBase)
{
    int i;
    struct MUIMasterBase_intern *mb = MUIMB(MUIMasterBase);

    /* NOTE: not the other way round, otherwise you will
     * try to free superclasses before subclasses... */
    /* TODO: when we'll have a hash table, we'll need to loop thru it
     * until either we don't have any more classes or we can't free any
     * (think of it like the last pass of a bubble sort). */
    for (i = mb->ClassCount-1; i >= 0; i--)
    {
      if (mb->Classes[i])
      {
        if (FreeClass(mb->Classes[i])) mb->Classes[i] = NULL;
        else
        {
#if 0
          kprintf("*** destroy_classes: FreeClass() failed for %s:\n"
                  "    SubclassCount=%ld ObjectCount=%ld\n",
                  Classes[i]->cl_ID,
                  Classes[i]->cl_SubclassCount,
                  Classes[i]->cl_ObjectCount);
#endif
          return FALSE;
        }
      }
    }

    FreeVec(mb->Classes);
    mb->Classes = NULL;
    mb->ClassCount = 0;
    mb->ClassSpace = 0;
    return TRUE;
}

static const struct __MUIBuiltinClass *builtins[] =
{
    &_MUI_Notify_desc,
    &_MUI_Family_desc,
    &_MUI_Application_desc,
    &_MUI_Window_desc,
    &_MUI_Area_desc,
    &_MUI_Rectangle_desc,
    &_MUI_Group_desc,
    &_MUI_Image_desc,
    &_MUI_Configdata_desc,
    &_MUI_Text_desc,
    &_MUI_Numeric_desc,
    &_MUI_Slider_desc,
    &_MUI_String_desc,
    ZUNE_BOOPSI_DESC
    &_MUI_Prop_desc,
    &_MUI_Scrollbar_desc,
    &_MUI_Register_desc,
    &_MUI_Menuitem_desc,
    &_MUI_Menu_desc,
    &_MUI_Menustrip_desc,
    ZUNE_VIRTGROUP_DESC
    ZUNE_SCROLLGROUP_DESC
    &_MUI_Scrollbutton_desc,
    &_MUI_Semaphore_desc,
    &_MUI_Dataspace_desc,
    &_MUI_Bitmap_desc,
    &_MUI_Bodychunk_desc,
    &_MUI_ChunkyImage_desc,
    &_MUI_Cycle_desc,
    &_MUI_Popstring_desc,
    &_MUI_Listview_desc,
    &_MUI_List_desc,
    ZUNE_POPASL_DESC
    &_MUI_Popobject_desc,
    ZUNE_GAUGE_DESC
    ZUNE_ABOUTMUI_DESC
    ZUNE_SETTINGSGROUP_DESC
    ZUNE_IMAGEADJUST_DESC
    ZUNE_POPIMAGE_DESC
    ZUNE_SCALE_DESC
    &_MUI_Radio_desc,
    &_MUI_IconList_desc,
    &_MUI_IconDrawerList_desc,
    &_MUI_IconVolumeList_desc,
    ZUNE_ICONLISTVIEW_DESC
    &_MUI_Balance_desc,
    ZUNE_COLORFIELD_DESC
    ZUNE_COLORADJUST_DESC
    ZUNE_IMAGEDISPLAY_DESC
    ZUNE_PENDISPLAY_DESC
    ZUNE_PENADJUST_DESC
    ZUNE_POPPEN_DESC
    &_MUI_Mccprefs_desc,
    ZUNE_FRAMEDISPLAY_DESC
    ZUNE_POPFRAME_DESC
    ZUNE_FRAMEADJUST_DESC
};

#define NUM_BUILTINS  sizeof(builtins) / sizeof(struct __MUIBuiltinClass *)

/*
 * metaDispatcher - puts h_Data in A6 and calls real dispatcher
 */

#ifdef __AROS__
AROS_UFH3(IPTR, metaDispatcher,
	AROS_UFHA(struct IClass  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
    return AROS_UFC4(IPTR, cl->cl_Dispatcher.h_SubEntry,
        AROS_UFPA(Class  *, cl,  A0),
        AROS_UFPA(Object *, obj, A2),
        AROS_UFPA(Msg     , msg, A1),
        AROS_UFPA(APTR    , cl->cl_Dispatcher.h_Data, A6)
    );
}

#else
#ifdef __MAXON__
__asm ULONG metaDispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
{
	  /* We don't use a metaDispatcher */
    return 0;
}
#else
__asm ULONG metaDispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
{
    __asm ULONG (*entry)(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg) =
	(__asm ULONG (*)(register __a0 struct IClass *, register __a2 Object *, register __a1 Msg))cl->cl_Dispatcher.h_SubEntry;

    putreg(REG_A6,(long)cl->cl_Dispatcher.h_Data);
    return entry(cl,obj,msg);
}
#endif
#endif


/**************************************************************************
 Given the builtin class, construct the
 class and make it public (because of the fake lib base).
**************************************************************************/
static struct IClass *builtin_to_public_class(const struct __MUIBuiltinClass *desc, struct Library *MUIMasterBase)
{
    struct IClass *cl;
    struct IClass *superClassPtr;
    CONST_STRPTR superClassID = NULL;

    if (strcmp(desc->supername, ROOTCLASS) == 0)
    {
        superClassID  = desc->supername;
        superClassPtr = NULL;
    }
    else
    {
        superClassID  = NULL;
        superClassPtr = MUI_GetClass((char *)desc->supername);
        if (!superClassPtr)
            return NULL;
    }

    if (!(cl = MakeClass((STRPTR)desc->name, (STRPTR)superClassID, superClassPtr, desc->datasize, 0)))
	return NULL;

#ifdef __MAXON__
    cl->cl_Dispatcher.h_Entry = desc->dispatcher;
#else
    cl->cl_Dispatcher.h_Entry = (HOOKFUNC)metaDispatcher;
    cl->cl_Dispatcher.h_SubEntry = desc->dispatcher;
#endif
    cl->cl_Dispatcher.h_Data = MUIMasterBase;
    return cl;
}


/**************************************************************************
 Create a builtin class and all its superclasses.
**************************************************************************/
struct IClass *CreateBuiltinClass(CONST_STRPTR className, struct Library *MUIMasterBase)
{
    int i;

    for (i = 0 ; i < NUM_BUILTINS ; i++)
    {
	const struct __MUIBuiltinClass *builtin = builtins[i];

	/* found the class to create */
	if (!strcmp(builtin->name, className))
	{
	    return builtin_to_public_class(builtin,MUIMasterBase);
	}
    }
    return NULL;
}


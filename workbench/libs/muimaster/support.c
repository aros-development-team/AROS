#include <string.h>

#ifndef _AROS
#include <dos.h>
#endif

#include <intuition/classes.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "support.h"
#include "mui.h"
#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;

/**************************************************************************
 check if region is entirely within given bounds
**************************************************************************/
int isRegionWithinBounds(struct Region *r, int left, int top, int width, int height)
{
	if ((left <= r->bounds.MinX) && (left + width  - 1 >= r->bounds.MaxX)
	 && (top  <= r->bounds.MinY) && (top  + height - 1 >= r->bounds.MaxY))
		return 1;

	return 0;
}

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

    /* NOTE: not the other way round, otherwise you will
     * try to free superclasses before subclasses... */
    /* TODO: when we'll have a hash table, we'll need to loop thru it
     * until either we don't have any more classes or we can't free any
     * (think of it like the last pass of a bubble sort). */
    for (i = MUIMB(MUIMasterBase)->ClassCount-1; i >= 0; i--)
    {
      if (MUIMB(MUIMasterBase)->Classes[i])
      {
        if (FreeClass(MUIMB(MUIMasterBase)->Classes[i]))
          MUIMB(MUIMasterBase)->Classes[i] = NULL;
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

    FreeVec(MUIMB(MUIMasterBase)->Classes);
    MUIMB(MUIMasterBase)->Classes = NULL;
    MUIMB(MUIMasterBase)->ClassCount = 0;
    MUIMB(MUIMasterBase)->ClassSpace = 0;
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
//    &_MUI_Image_desc,
//    &_MUI_Dataspace_desc,
//    &_MUI_Configdata_desc,
    &_MUI_Text_desc,
    &_MUI_Numeric_desc,
    &_MUI_Slider_desc,
    &_MUI_String_desc,
    &_MUI_Boopsi_desc,
    &_MUI_Prop_desc,
    &_MUI_Scrollbar_desc,
    &_MUI_Register_desc,
    &_MUI_Menuitem_desc
};

#define NUM_BUILTINS  sizeof(builtins) / sizeof(struct __MUIBuiltinClass *)

/*
 * metaDispatcher - puts h_Data in A6 and calls real dispatcher
 */

#ifdef _AROS
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
__asm ULONG metaDispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
{
    __asm ULONG (*entry)(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg) =
	(__asm ULONG (*)(register __a0 struct IClass *, register __a2 Object *, register __a1 Msg))cl->cl_Dispatcher.h_SubEntry;

    putreg(REG_A6,(long)cl->cl_Dispatcher.h_Data);
    return entry(cl,obj,msg);
}
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

    cl->cl_Dispatcher.h_Entry = (HOOKFUNC)metaDispatcher;
    cl->cl_Dispatcher.h_SubEntry = desc->dispatcher;
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


/**************************************************************************
 Converts a Rawkey to a vanillakey
**************************************************************************/
ULONG ConvertKey(struct IntuiMessage *imsg)
{
   struct InputEvent event;
   UBYTE code = 0;
   event.ie_NextEvent    = NULL;
   event.ie_Class        = IECLASS_RAWKEY;
   event.ie_SubClass     = 0;
   event.ie_Code         = imsg->Code;
   event.ie_Qualifier    = imsg->Qualifier;
   event.ie_EventAddress = (APTR *) *((ULONG *)imsg->IAddress);
   MapRawKey(&event, &code, 1, NULL);
   return code;
}

/**************************************************************************
...
**************************************************************************/
ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
  return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}


/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <libraries/mui.h>

#include <proto/intuition.h>

#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "zunestuff.h"

/****************************************************************
 Create a simple label
*****************************************************************/
Object *MakeLabel(STRPTR str)
{
  return (MUI_MakeObject(MUIO_Label, str, 0));
}

/****************************************************************
 Create a button (within the cycle chain)
*****************************************************************/
Object *MakeButton(STRPTR str)
{
    Object *obj = MUI_MakeObject(MUIO_Button, str);
    if (obj)
    {
        SetAttrs(obj,
	     MUIA_CycleChain, 1,
	     TAG_DONE);
    }
    return obj;
}


/****************************************************************
 Easy getting an attributes value
*****************************************************************/
LONG xget(Object * obj, ULONG attr)
{
    LONG x = 0;
    get(obj, attr, &x);
    return x;
}

Object *MakeBackgroundPopimage(void)
{
    return NewObject(CL_ImageClipboard->mcc_Class, NULL,
		     MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
		     MUIA_Draggable, TRUE,
		     MUIA_CycleChain, 1,
		     MUIA_Window_Title, "Adjust Background",
		     TAG_DONE);
}

Object *MakePopframe(void)
{
    return NewObject(CL_FrameClipboard->mcc_Class, NULL,
		     MUIA_Draggable, TRUE,
		     MUIA_CycleChain, 1,
		     MUIA_Window_Title, "Adjust Frame",
		     TAG_DONE);
}

ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
    return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}


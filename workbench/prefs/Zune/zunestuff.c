/*
    Copyright � 2002, The AROS Development Team. 
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
 Create a button (within the cycle chain)
*****************************************************************/
Object *MakeButton(CONST_STRPTR str)
{
    Object *obj = MUI_MakeObject(MUIO_Button, (IPTR)str);
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
#ifdef __AROS__
LONG xget(Object * obj, ULONG attr)
{
    LONG x = 0;
    get(obj, attr, &x);
    return x;
}
#endif

Object *MakeSpacingSlider (void)
{
    Object *obj = MUI_MakeObject(MUIO_Slider, (IPTR)"", 0, 9);
    if (obj)
	set(obj, MUIA_CycleChain, 1);
    return obj;
}

Object *MakeCycle (CONST_STRPTR label, CONST_STRPTR entries[])
{
    Object *obj = MUI_MakeObject(MUIO_Cycle, (IPTR)label, (IPTR)entries);
    if (obj)
	set(obj, MUIA_CycleChain, 1);
    return obj;
}

Object *MakeCheck (CONST_STRPTR label)
{
    Object *obj = MUI_MakeObject(MUIO_Checkmark, (IPTR)label);
    if (obj)
	set(obj, MUIA_CycleChain, 1);
    return obj;
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

#ifdef __AROS__

ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
    return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}

#endif

/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <libraries/mui.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>

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
    return MUI_NewObject(MUIC_Popimage,
		     MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
		     MUIA_CycleChain, 1,
		     MUIA_Window_Title, (IPTR)"Adjust Background",
		     TAG_DONE);
}

Object *MakePopframe(void)
{
    return MUI_NewObject(MUIC_Popframe,
		     MUIA_CycleChain, 1,
		     MUIA_Window_Title, (IPTR)"Adjust Frame",
		     TAG_DONE);
}

Object *MakePoppen(void)
{
    return MUI_NewObject(MUIC_Poppen,
		     MUIA_CycleChain, 1,
		     MUIA_Window_Title, (IPTR)"Adjust Pen",
		     TAG_DONE);
}

Object *MakePopfont(Object **store, BOOL fixed)
{
    Object *string;
    
    if (store == NULL) store = &string;
    
    return PopaslObject,
        MUIA_Popasl_Type,              ASL_FontRequest,
        ASLFO_MaxHeight,               100,
        ASLFO_FixedWidthOnly,          fixed ? TRUE : FALSE,
        MUIA_Popstring_String, (IPTR) *store = TextObject,
            TextFrame,
            MUIA_Background, MUII_TextBack,
        End,
        MUIA_Popstring_Button, (IPTR)  PopButton(MUII_PopUp),
    End;
}

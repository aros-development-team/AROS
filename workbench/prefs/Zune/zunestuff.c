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

Object *MakeString(void)
{
    return StringObject,
	StringFrame,
	MUIA_CycleChain, 1,
        End;
}

Object *MakePopfont(BOOL fixed)
{
    return PopaslObject,
        MUIA_Popasl_Type,              ASL_FontRequest,
        ASLFO_MaxHeight,               100,
        ASLFO_FixedWidthOnly,          fixed ? TRUE : FALSE,
        MUIA_Popstring_String, (IPTR)  MakeString(),
        MUIA_Popstring_Button, (IPTR)  PopButton(MUII_PopUp),
    End;
}

void SliderToConfig (Object *slider, Object *configdata, ULONG cfg)
{
    DoMethod(configdata, MUIM_Configdata_SetULong, cfg,
	     XGET(slider, MUIA_Numeric_Value));
}

void CheckmarkToConfig (Object *checkmark, Object *configdata, ULONG cfg)
{
    DoMethod(configdata, MUIM_Configdata_SetULong, cfg,
	     XGET(checkmark, MUIA_Selected));
}

void FrameToConfig (Object *popframe, Object *configdata, ULONG cfg)
{
    CONST_STRPTR str;

    str = (CONST_STRPTR)XGET(popframe, MUIA_Framedisplay_Spec);
    DoMethod(configdata, MUIM_Configdata_SetFramespec, cfg, (IPTR)str);
}

void PenToConfig (Object *poppen, Object *configdata, ULONG cfg)
{
    CONST_STRPTR str;

    str = (CONST_STRPTR)XGET(poppen, MUIA_Pendisplay_Spec);
    DoMethod(configdata, MUIM_Configdata_SetPenspec, cfg, (IPTR)str);
}

void StringToConfig (Object *string, Object *configdata, ULONG cfg)
{
    CONST_STRPTR str;

    str = (CONST_STRPTR)XGET(string, MUIA_String_Contents);
    DoMethod(configdata, MUIM_Configdata_SetString, cfg, (IPTR)str);
}

void CycleToConfig (Object *cycle, Object *configdata, ULONG cfg)
{
    DoMethod(configdata, MUIM_Configdata_SetULong, cfg,
	     XGET(cycle, MUIA_Cycle_Active));
}

void ConfigToSlider (Object *configdata, ULONG cfg, Object *slider)
{
    setslider(slider, DoMethod(configdata, MUIM_Configdata_GetULong, cfg));
}

void ConfigToCheckmark (Object *configdata, ULONG cfg, Object *checkmark)
{
    setcheckmark(checkmark, DoMethod(configdata, MUIM_Configdata_GetULong, cfg));
}

void ConfigToFrame (Object *configdata, ULONG cfg, Object *popframe)
{
    CONST_STRPTR spec;

    spec = (CONST_STRPTR)DoMethod(configdata, MUIM_Configdata_GetString, cfg);
    set(popframe, MUIA_Framedisplay_Spec, (IPTR)spec);
}

void ConfigToPen (Object *configdata, ULONG cfg, Object *poppen)
{
    STRPTR spec;

    spec = (STRPTR)DoMethod(configdata, MUIM_Configdata_GetString, cfg);
    set(poppen, MUIA_Pendisplay_Spec, (IPTR)spec);
}

void ConfigToCycle (Object *configdata, ULONG cfg, Object *cycle)
{
    setcycle(cycle, DoMethod(configdata, MUIM_Configdata_GetULong, cfg));
}

void ConfigToString (Object *configdata, ULONG cfg, Object *string)
{
    setstring(string, DoMethod(configdata, MUIM_Configdata_GetString, cfg));
}

#ifdef __amigaos4__
Object *VARARGS68K DoSuperNewTags(struct IClass *cl, Object *obj, void *dummy, ...)
{
    va_list argptr;
    struct TagItem *tagList;
    va_startlinear(argptr, dummy);
    tagList = va_getlinearva(argptr, struct TagItem *);
    obj = (Object*)DoSuperMethod(cl,obj,OM_NEW,tagList,dummy);
    va_end(argptr);
    return obj;
}
#endif

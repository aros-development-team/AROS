/*
    Copyright © 2002-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#include <libraries/mui.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/asl.h>

#include "zunestuff.h"


/****************************************************************
 aslfilerequest for load/save - buffer overflow safe
*****************************************************************/
long aslfilerequest(char *msg,char *dirpart,char *filepart,char *fullname, struct TagItem *tags)
{

/* msg=a name to show
   tags=can be 0 or some additional tags
   dirpart= a pointer to a buffer of 500 bytes that recieve the selected directory
   filepart= a pointer to a buffer of 500 bytes that recieve the selected filename
   fullname= a pointer to a buffer of 1000 bytes that recieve the selected full filename */
     
    struct FileRequester *fr;
    struct Library *AslBase;
    AslBase = OpenLibrary("asl.library", 37L);

    if (AslBase)
    {
        struct TagItem frtags[] =
        {
    
        { ASLFR_TitleText,          (IPTR)msg },
        { ASLFR_InitialDrawer,      (IPTR)dirpart},
        { ASLFR_InitialFile,        (IPTR)filepart},
        { TAG_MORE,                 (IPTR)tags }
        
        }; 
  
    
        if ( (int)(fr = (struct FileRequester *) AllocAslRequest(ASL_FileRequest, frtags) ) )
        {
            if (AslRequest(fr, NULL))
            {
                      
                strncpy(dirpart,fr->rf_Dir,498);
                strncpy(filepart,fr->rf_File,498);
                strncpy(fullname,dirpart,498);
                AddPart(fullname,filepart,998);
                FreeAslRequest(fr);
                CloseLibrary(AslBase);
        
                return 1;
            } 

            if (AslBase) CloseLibrary(AslBase);

            return 0;
        } 
    

    }

    return 0;
   
}

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
		     MUIA_Window_Title, (IPTR) _(MSG_ADJUST_BACKGROUND),
		     TAG_DONE);
}

Object *MakePopframe(void)
{
    return MUI_NewObject(MUIC_Popframe,
		     MUIA_CycleChain, 1,
		     MUIA_Window_Title, (IPTR) _(MSG_ADJUST_FRAME),
		     TAG_DONE);
}

Object *MakePoppen(void)
{
    return MUI_NewObject(MUIC_Poppen,
		     MUIA_CycleChain, 1,
		     MUIA_Window_Title, (IPTR) _(MSG_ADJUST_PEN),
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

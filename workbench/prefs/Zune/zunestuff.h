#ifndef _ZUNE_ZUNESTUFF_H
#define _ZUNE_ZUNESTUFF_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/classusr.h>
#include <libraries/asl.h>

Object *MakeButton (CONST_STRPTR str);
Object *MakeCycle (CONST_STRPTR label, CONST_STRPTR entries[]);
Object *MakeCheck (CONST_STRPTR label);
Object *MakeSpacingSlider (void);
Object *MakeBackgroundPopimage(void);
Object *MakePopframe(void);
Object *MakePoppen(void);
Object *MakeString(void);
Object *MakePopfont(BOOL fixed);

void SliderToConfig (Object *slider, Object *configdata, ULONG cfg);
void CheckmarkToConfig (Object *checkmark, Object *configdata, ULONG cfg);
void FrameToConfig (Object *popframe, Object *configdata, ULONG cfg);
void PenToConfig (Object *poppen, Object *configdata, ULONG cfg);
void CycleToConfig (Object *cycle, Object *configdata, ULONG cfg);
void StringToConfig (Object *string, Object *configdata, ULONG cfg);

void ConfigToSlider (Object *configdata, ULONG cfg, Object *slider);
void ConfigToCheckmark (Object *configdata, ULONG cfg, Object *checkmark);
void ConfigToFrame (Object *configdata, ULONG cfg, Object *popframe);
void ConfigToPen (Object *configdata, ULONG cfg, Object *poppen);
void ConfigToCycle (Object *configdata, ULONG cfg, Object *cycle);
void ConfigToString (Object *configdata, ULONG cfg, Object *string);

#ifndef __GNUC__
LONG XGET(Object * obj, ULONG attr);
#endif

#define getstring(obj) (char*)XGET(obj,MUIA_String_Contents)
#define FindFont(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)

#ifdef __amigaos4__
Object *VARARGS68K DoSuperNewTags(struct IClass *cl, Object *obj, void *dummy, ...);
#endif

#endif /* _ZUNE_ZUNESTUFF_H */

/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _ZUNE_ZUNESTUFF_H
#define _ZUNE_ZUNESTUFF_H

Object *MakeButton (CONST_STRPTR str);
Object *MakeCycle (CONST_STRPTR label, CONST_STRPTR entries[]);
Object *MakeCheck (CONST_STRPTR label);
Object *MakeSpacingSlider (void);
Object *MakeBackgroundPopimage(void);
Object *MakePopframe(void);
ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...);

LONG xget(Object * obj, ULONG attr);

#define getstring(obj) (char*)xget(obj,MUIA_String_Contents)
#define FindFont(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)

extern struct MUI_CustomClass *CL_ImageClipboard;
extern struct MUI_CustomClass *CL_FrameClipboard;


#endif

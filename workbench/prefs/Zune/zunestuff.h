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
Object *MakePopfont(Object **string, BOOL fixed);

#ifndef __GNUC__
LONG XGET(Object * obj, ULONG attr);
#endif

#define getstring(obj) (char*)XGET(obj,MUIA_String_Contents)
#define FindFont(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)

#endif /* _ZUNE_ZUNESTUFF_H */

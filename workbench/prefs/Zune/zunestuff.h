/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _ZUNE_ZUNESTUFF_H
#define _ZUNE_ZUNESTUFF_H

Object *MakeLabel(STRPTR str);
Object *MakeButton(STRPTR str);
LONG xget(Object * obj, ULONG attr);

#define getstring(obj) (char*)xget(obj,MUIA_String_Contents)

#endif

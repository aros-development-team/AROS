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
    LONG x;
    get(obj, attr, &x);
    return x;
}


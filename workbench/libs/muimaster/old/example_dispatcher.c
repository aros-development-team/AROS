/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <exec/types.h>
#include <intuition/classes.h>
#include <aros/asmcall.h>
#include <stdio.h>

AROS_UFP3S(IPTR, rootDispatcher,
    AROS_UFPA(Class  *,  cl,  A0),
    AROS_UFPA(Object *, obj, A2),
    AROS_UFPA(Msg     , msg, A1)
);

int main(void)
{
  AROS_UFC3(IPTR, rootDispatcher,
    AROS_UFPA(Class  *, (Class  *)1, A0),
    AROS_UFPA(Object *, (Object *)2, A2),
    AROS_UFPA(Msg     , (Msg     )3, A1)
  );

  return 0;
}

AROS_UFH3S(IPTR, rootDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
  printf("cl  = %p\n", cl);
  printf("obj = %p\n", obj);
  printf("msg = %p\n", msg);
  return 0;
}


/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: support function for the detach.o startup file. 
          Probably this is not the right place where to keep it,
	  but it has to stay in a library, and making a library
	  just for this file seemed overkill to me.
*/

#include <dos/dos.h>

int  __detached_manages_detach = 1;

extern LONG __detached_return_value;
extern struct Process *__detacher_process;
extern void __Detach(LONG retval);

void Detach(void)
{
    __Detach(RETURN_OK);
}




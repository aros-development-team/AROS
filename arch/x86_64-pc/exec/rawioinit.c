/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: functions for serial RawIOInit/RawPutChar
    Lang: english

    Note: serial io from "PC-intern" examples
*/
#include <aros/config.h>
#include <proto/exec.h>
#include <asm/io.h>

#include <bootconsole.h>

AROS_LH0(void, RawIOInit,
	 struct ExecBase *, SysBase, 84, Exec)
{
   AROS_LIBFUNC_INIT

   /* Re-initialize bootconsole's serial port with current values */
   serial_Init(NULL);

   AROS_LIBFUNC_EXIT
}

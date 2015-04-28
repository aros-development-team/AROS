/*
 * Extra data for m68k-all
 */
#ifndef __EXEC_PLATFORM_H
#define __EXEC_PLATFORM_H

#include <exec/types.h>

struct Exec_PlatformData
{
	APTR  realRawDoFmt;     /* AOS 3.1 locale.library workaround - see
	                           exec.library/SetFunction() */
	struct TagItem *BootMsg;
};

#define GET_THIS_TASK           (SysBase->ThisTask)
#define SET_THIS_TASK(x)        (SysBase->ThisTask=(x))

#endif

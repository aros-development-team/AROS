/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>
#include <locale.h>
#include <process.h>
#include <pwd.h>
#include <grp.h>

#include <ioerr2errno.h>    /* FIXME: Should this really be public? */
#include "signalhack.h"     /* FIXME: Use <signal.h> when it is finished */

#include "arosc_init.h"

extern struct aroscbase *AROS_SLIB_ENTRY(open,arosc)();
extern BPTR AROS_SLIB_ENTRY(close,arosc)();
extern BPTR AROS_SLIB_ENTRY(expunge,arosc)();
extern int AROS_SLIB_ENTRY(null,arosc)();

void *const arosc_functable[]=
{
    &AROS_SLIB_ENTRY(open,arosc),
    &AROS_SLIB_ENTRY(close,arosc),
    &AROS_SLIB_ENTRY(expunge,arosc),
    &AROS_SLIB_ENTRY(null,arosc),
#define SYSTEM_CALL(name, x...)  &name,
#include <sys/syscall.def>
#undef SYSTEM_CALL
    (void *)-1
};


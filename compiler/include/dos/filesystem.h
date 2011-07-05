#ifndef DOS_FILESYSTEM_H
#define DOS_FILESYSTEM_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS specific structures and definitions for filesystems.
    Lang: english
*/

#ifndef EXEC_IO_H
#   include <exec/io.h>
#endif
#ifndef EXEC_TASKS_H
#   include <exec/tasks.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef DOS_FILEHANDLER_H
#   include <dos/filehandler.h>
#endif
#ifndef DOS_EXALL_H
#   include <dos/exall.h>
#endif

#define FCM_COOKED	0
#define FCM_RAW		(1<<0)
#define FCM_NOECHO  (1<<1)

#endif /* DOS_FILESYSTEM_H */

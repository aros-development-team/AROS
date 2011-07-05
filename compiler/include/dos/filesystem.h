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

/* io_FileMode for FSA_OPEN, FSA_OPEN_FILE and FSA_FILE_MODE. These are flags
   and may be OR'ed. Note that not all filesystems support all flags. */
#define FMF_LOCK     (1L<<0) /* Lock exclusively. */
#define FMF_EXECUTE  (1L<<1) /* Open for executing. */
/* At least one of the following two flags must be specified. Otherwise expect
   strange things to happen. */
#define FMF_WRITE    (1L<<2)  /* Open for writing. */
#define FMF_READ     (1L<<3)  /* Open for reading. */
#define FMF_CREATE   (1L<<4)  /* Create file if it doesn't exist. */
#define FMF_CLEAR    (1L<<5)  /* Truncate file on open. */
#define FMF_RAW      (1L<<6)  /* Switch cooked to raw and vice versa. */
#define FMF_NONBLOCK (1L<<7)  /* Don't block Open() in case it would
                                 and return an error in case Write()/Read()
				 would block */
#define FMF_APPEND   (1L<<8)  /* Every write will happen always at the end
                                 of the file */

#define FMF_AMIGADOS (1L<<9 | 1L<<31) /* Identifies the old AmigaDOS modes:
					 - bit 9 is the first bit set in the MODE_#? modes
					 - bit 31 is the first bit set in ACCESS_#? modes
				      */
#define FMF_MODE_OLDFILE   (FMF_AMIGADOS | FMF_WRITE | FMF_READ)
#define FMF_MODE_READWRITE (FMF_MODE_OLDFILE | FMF_CREATE)
#define FMF_MODE_NEWFILE   (FMF_MODE_READWRITE | FMF_LOCK | FMF_CLEAR)

#if 0
/* io_MountMode for FSA_MOUNT_MODE. These are flags and may be OR'ed. */
#define MMF_READ	(1L<<0) /* Mounted for reading. */
#define MMF_WRITE	(1L<<1) /* Mounted for writing. */
#define MMF_READ_CACHE	(1L<<2) /* Read cache enabled. */
#define MMF_WRITE_CACHE (1L<<3) /* Write cache enabled. */
#define MMF_OFFLINE	(1L<<4) /* Filesystem currently does not use the
                                   device. */
#define MMF_LOCKED	(1L<<5) /* Mount mode is password protected. */
#endif

#endif /* DOS_FILESYSTEM_H */

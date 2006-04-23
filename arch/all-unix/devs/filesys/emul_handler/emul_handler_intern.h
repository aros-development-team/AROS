#ifndef __EMUL_HANDLER_INTERN_H
#define __EMUL_HANDLER_INTERN_H
/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

/* AROS includes */
#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <hidd/hidd.h>

/* POSIX includes */
#define timeval sys_timeval
#include <dirent.h>
#include <sys/types.h>
#undef timeval

struct emulbase
{
    struct Device		  device;
    				/* nlorentz: Cal it ev_std* because std* is reserved */
    struct Unit       		* eb_stdin;
    struct Unit       		* eb_stdout;
    struct Unit       		* eb_stderr;
    struct ExecBase  		* sysbase;
    struct SignalSemaphore 	  sem;
    struct SignalSemaphore	  memsem;
    char    	    	    	* current_volume;
    APTR			  mempool;
    HIDD			  unixio;
    BPTR 			  seglist;
};


struct filehandle
{
    char * name;     /* full name including pathname                 */
    int    type;     /* type can either be FHD_FILE or FHD_DIRECTORY */
    char * pathname; /* if type == FHD_FILE then you'll find the pathname here */
    long   dirpos;   /* and how to reach it via seekdir(.,dirpos) here. */
    DIR  * DIR;      /* both of these vars will be filled in by examine *only* (at the moment) */
    char * volume;
    char * volumename;
    long   fd;
};
#define FHD_FILE      0
#define FHD_DIRECTORY 1


/* Support functions */
static LONG err_u2a(void);
static LONG makefilename(struct emulbase *, char **dest, STRPTR path, STRPTR filename);

/* Actions */
static LONG read_softlink(struct emulbase *, struct filehandle *, STRPTR, ULONG);

#endif /* __EMUL_HANDLER_INTERN_H */

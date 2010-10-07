#ifndef __EMUL_HANDLER_INTERN_H
#define __EMUL_HANDLER_INTERN_H
/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

#ifndef NATIVE
/* AROS includes */
#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <hidd/hidd.h>

/* POSIX includes */
#define timeval sys_timeval
//#include <dirent.h>
//#include <sys/types.h>
#undef timeval

struct emulbase
{
    struct Device		  device;
    				/* nlorentz: Cal it eb_std* because std* is reserved */
    struct Unit       		* eb_stdin;
    struct Unit       		* eb_stdout;
    struct Unit       		* eb_stderr;
    struct SignalSemaphore 	  sem;
    struct SignalSemaphore	  memsem;
    char    	    	    	* current_volume;
    APTR			  mempool;
    HIDD			  unixio;
};


struct filehandle
{
    char * name;     /* full name including pathname                 */
    int    type;     /* type can either be FHD_FILE or FHD_DIRECTORY */
    char * pathname; /* if type == FHD_FILE then you'll find the pathname here */
    long   dirpos;   /* and how to reach it via seekdir(.,dirpos) here. */
    void  * DIR;      /* both of these vars will be filled in by examine *only* (at the moment) */
    char * volume;
    char * volumename;
    long   fd;
};
#define FHD_FILE      0
#define FHD_DIRECTORY 1


/* Support functions */
static LONG makefilename(struct emulbase *, char **dest, STRPTR path, STRPTR filename);

/* Actions */
static LONG read_softlink(struct emulbase *, struct filehandle *, STRPTR, ULONG);

#endif /* NATIVE */


#define EHND_Dummy              (TAG_USER + 0x03210000)
#define EHND_OpenImpl			(EHND_Dummy+1)
#define EHND_CloseImpl			(EHND_Dummy+2)
#define EHND_OpenDirImpl			(EHND_Dummy+3)
#define EHND_CloseDirImpl			(EHND_Dummy+4)
#define EHND_StatImpl			(EHND_Dummy+5)
#define EHND_LStatImpl			(EHND_Dummy+6)
#define EHND_CheckDirImpl			(EHND_Dummy+7)
#define EHND_DirNameImpl			(EHND_Dummy+8)
#define EHND_TellDirImpl			(EHND_Dummy+9)
#define EHND_SeekDirImpl			(EHND_Dummy+10)
#define EHND_RewindDirImpl			(EHND_Dummy+11)
#define EHND_DeleteImpl			(EHND_Dummy+12)
#define EHND_RenameImpl			(EHND_Dummy+13)
#define EHND_GetEnvImpl			(EHND_Dummy+14)
#define EHND_GetHomeImpl			(EHND_Dummy+15)
#define EHND_GetCWDImpl			(EHND_Dummy+16)
#define EHND_ClosePWImpl			(EHND_Dummy+17)
#define EHND_StatFSImpl			(EHND_Dummy+18) 
#define EHND_ChDirImpl			(EHND_Dummy+19) 
#define EHND_IsattyImpl			(EHND_Dummy+20) 
#define EHND_LinkImpl			(EHND_Dummy+21) 
#define EHND_LSeekImpl			(EHND_Dummy+22) 
#define EHND_ChmodImpl			(EHND_Dummy+23) 
#define EHND_SymLinkImpl			(EHND_Dummy+24) 
#define EHND_MKDirImpl			(EHND_Dummy+25) 
#define EHND_ReadImpl			(EHND_Dummy+26)
#define EHND_ReadLinkImpl			(EHND_Dummy+27)
#define EHND_WriteImpl			(EHND_Dummy+28)


#endif /* __EMUL_HANDLER_INTERN_H */

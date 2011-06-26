#ifndef RESOURCES_EMUL_H
#define RESOURCES_EMUL_H
/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

#include <resources/emul_host.h>

struct filehandle
{
    struct FileHandle fh;
    char * hostname;		/* full host pathname (includes volume root prefix) */
    char * name;		/* full AROS name including pathname		    */
    int    type;		/* type flags, see below		       	    */
    char * volumename;		/* volume name					    */
    void * fd;			/* Object itself				    */
    struct DosList *dl;		/* Volume node					    */
    struct PlatformHandle ph;	/* Platform-specific data			    */
    unsigned int locks;         /* Number of open locks */
};

/* type flags */
#define FHD_FILE      0x01
#define FHD_DIRECTORY 0x02
#define FHD_STDIO     0x80

struct emulbase
{
    struct Library eb_Lib;
    			      /* nlorentz: Cal it eb_std* because std* is reserved */
    struct filehandle  	     *eb_stdin;
    struct filehandle 	     *eb_stdout;
    struct filehandle 	     *eb_stderr;
    APTR		      mempool;
    APTR		      ReadIRQ;
    APTR		      HostLibBase;
    APTR		      KernelBase;
    struct Emul_PlatformData  pdata;	/* Platform-specific portion */
};

#endif /* RESOURCES_EMUL_H */

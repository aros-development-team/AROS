
/*
** readargs.c
**
** (c) 1998-2011 Guido Mersmann
** (c) 2011 The AROS Development Team.
*/

/*************************************************************************/

#define SOURCENAME "readargs.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/dos.h>

#include "functions.h"
#include "icon.h"
#include "macros.h"
#include "readargs.h"
#include "version.h"
#include "wbstartup.h"

/*************************************************************************/

static char *readargs_template = "DEVICE,UNIT/N,TO/K,LIST/S,LOWERCYL/S,PREFIX=PRE/K,MAXTRANSFER=MT/K,MOUNTDOS/S,MOUNTNTFS/S,MOUNTEXT/S,MOUNTSGIX/S";
static struct RDArgs *readargs_args;

IPTR readargs_array[ARGS_SIZEOF];

static char devicename[DEVICENAME_SIZEOF];    /* local copy of device name */
static char deviceprefix[DEVICENAME_SIZEOF];  /* local copy of device prefix */

/* setup defaults */

#ifdef __MORPHOS__
char    DEFAULT_DEVICE[] = "ide.device";
#else
 #ifdef __amigaos4__
char    DEFAULT_DEVICE[] = "a1ide.device";
 #else
char    DEFAULT_DEVICE[] = "amithlon.device";
 #endif
#endif

#define DEFAULT_UNIT 0L
char    DEFAULT_PREFIX[] = "GGD";
char    DEFAULT_MAXTRANSFER[] = "0xfffe00";
#define DEFAULT_LOWERCYL  0L
#define DEFAULT_MOUNTDOS  0L
#define DEFAULT_MOUNTNTFS 0L
#define DEFAULT_MOUNTSGIX 0L
#define DEFAULT_MOUNTEXT  0L

/*************************************************************************/

/* /// ReadArgs_ReadArgs()
**
*/

/*************************************************************************/

BOOL ReadArgs_ReadArgs( void )
{
struct DiskObject *dob;

    if( !wbmessage ) {

        if( (readargs_args = ReadArgs(readargs_template, readargs_array, NULL)) ) {

	    if (!readargs_array[ARG_DEVICE])
	    	readargs_array[ARG_DEVICE] = (IPTR)DEFAULT_DEVICE;
	    if (!readargs_array[ARG_UNIT])
	        readargs_array[ARG_UNIT] = DEFAULT_UNIT;
	    if (!readargs_array[ARG_PREFIX])
	        readargs_array[ARG_PREFIX] = (IPTR)DEFAULT_PREFIX;

			readargs_array[ARG_MAXTRANSFER] = String_HexToLongPreFix( (readargs_array[ARG_MAXTRANSFER] ? (STRPTR) readargs_array[ARG_MAXTRANSFER] : (STRPTR) DEFAULT_MAXTRANSFER), 8 );

        } else {
			PrintFault( IoErr(), app_appname);
            return(FALSE);
        }

    } else {

		if( (dob = Icon_GetPutDiskObject( NULL ) ) ) {

            readargs_array[ ARG_DEVICE ]      = (IPTR)&devicename[0];
            readargs_array[ ARG_PREFIX ]      = (IPTR)&deviceprefix[0];

            String_CopySize( Icon_ToolTypeGetString( dob, "DEVICE", DEFAULT_DEVICE ), &devicename[0], DEVICENAME_SIZEOF );
            String_CopySize( Icon_ToolTypeGetString( dob, "PREFIX", DEFAULT_PREFIX ), &deviceprefix[0], DEVICENAME_SIZEOF );

            readargs_array[ARG_MAXTRANSFER] = String_HexToLongPreFix( Icon_ToolTypeGetString( dob, "MAXTRANSFER", DEFAULT_MAXTRANSFER ),8);
            readargs_array[ARG_UNIT]        = Icon_ToolTypeGetInteger( dob, "UNIT",      DEFAULT_UNIT );
            readargs_array[ARG_LOWERCYL]    = Icon_ToolTypeGetBool(    dob, "LOWERCYL",  DEFAULT_LOWERCYL );
            readargs_array[ARG_MOUNTDOS]    = Icon_ToolTypeGetBool(    dob, "MOUNTDOS",  DEFAULT_MOUNTDOS );
            readargs_array[ARG_MOUNTNTFS]   = Icon_ToolTypeGetBool(    dob, "MOUNTNTFS", DEFAULT_MOUNTNTFS  );
            readargs_array[ARG_MOUNTSGIX]   = Icon_ToolTypeGetBool(    dob, "MOUNTSGIX", DEFAULT_MOUNTSGIX );
            readargs_array[ARG_MOUNTEXT]    = Icon_ToolTypeGetBool(    dob, "MOUNTEXT",  DEFAULT_MOUNTEXT  );
        }
    }
return(TRUE);
}
/* \\\ */
/* /// ReadArgs_FreeArgs()
**
*/

/*************************************************************************/

void ReadArgs_FreeArgs( void )
{
        if( readargs_args ) {
            FreeArgs( readargs_args);
        }
}
/* \\\ */


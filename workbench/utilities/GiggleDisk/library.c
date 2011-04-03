
/*
** library.c
**
** (c) 1998-2011 Guido Mersmann
**
** This file contains functions to open and close libraries by simply
** using an array. This is more efficiant and easier to maintain than
** using tons of combined if( OpenLibrary()) calls.
**
*/

/*************************************************************************/

#define SOURCENAME "library.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/exec.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/mui.h>

#include "library.h"
#include "requester.h"

/*************************************************************************/

#ifdef __amigaos4__

struct Library        *DOSBase;
struct Library        *UtilityBase;
struct IntuitionBase  *IntuitionBase;
struct Library        *IconBase;

struct DOSIFace       *IDOS;
struct UtilityIFace   *IUtility;
struct IntuitionIFace *IIntuition;
struct IconIFace      *IIcon;

#else

struct ExecBase       *SysBase;
struct DosLibrary     *DOSBase;
struct UtilityBase    *UtilityBase;
struct IntuitionBase  *IntuitionBase;
struct Library        *IconBase;

#endif

libstruct LibraryArray[]={
	LIBMACRO( "dos.library", DOSBase, 0, 0, IDOS),
	LIBMACRO( "intuition.library", IntuitionBase, 0, 0, IIntuition),
	LIBMACRO( "utility.library", UtilityBase, 0, 0, IUtility),
	LIBMACRO( "icon.library", IconBase, 0, 0, IIcon),
	{NULL,0,0,NULL}
};

/*************************************************************************/

/* /// Libraries_Open()
**
*/

/*************************************************************************/

BOOL Libraries_Open( void )
{
ULONG i = 0;
BOOL result = FALSE;

    while( LibraryArray[i].Name != NULL ) {
		if( ((*(LibraryArray[i].LibBase)) = (APTR) OpenLibrary( LibraryArray[i].Name,LibraryArray[i].Version ) ) ) {
#ifdef __amigaos4__
			if( ((*(LibraryArray[i].IBase)) = (APTR)GetInterface( (*(LibraryArray[i].LibBase)), "main", 1, NULL ) ) ) {
				i++;
				continue;
			} else {
				CloseLibrary( (APTR) (*(LibraryArray[i].IBase ) ) );
				(*(LibraryArray[i].IBase)) = NULL;
			}
#else
			i++;
			continue;
#endif
		}
		if( !( LibraryArray[i].Flags & LIBFLAGSF_NOTNEEDED ) ) {
#ifndef __AROS__
//#warning "Fix Library args if AROS compiler is complete"
            requester_args[0] = (long) LibraryArray[i].Name;
            requester_args[1] = (long) LibraryArray[i].Version;
#endif
            result = TRUE;
            break;
		}
		i++;
	}
	return(result);
}
/* \\\ */

/* /// Libraries_Close()
**
*/

/*************************************************************************/

void Libraries_Close( void )
{
ULONG i = 0;

     while( LibraryArray[i].Name != NULL ) {
#ifdef __amigaos4__
		if( (*(LibraryArray[i].IBase ) ) ) {
			DropInterface( (APTR) (*(LibraryArray[i].IBase ) ) );
			//(*(LibraryArray[i].IBase)) = NULL; /* removed because of startup.o */
        }
#endif
		if( (*(LibraryArray[i].LibBase ) ) ) {
			CloseLibrary( (struct Library *) (*(LibraryArray[i].LibBase ) ) );
			//(*(LibraryArray[i].LibBase)) = NULL; /* removed because of startup.o */
        }
        i++;
    }
}
/* \\\ */



#ifndef LIBRARIES_H
#define LIBRARIES_H 1

/*************************************************************************/

#include <exec/libraries.h>
#include <proto/exec.h>

/*************************************************************************/

/* /// "typedef libstruct" */

typedef struct {
    char* Name;
    ULONG Version;
    ULONG Flags;
	APTR *LibBase;
#ifdef __amigaos4__
	APTR *IBase;
#endif
} libstruct;

#define LIBFLAGSF_NOTNEEDED 1

#ifndef __amigaos4__
#define LIBMACRO(name,base,version,flags, ibase) {name, version, flags, (APTR*) &base}
#else
#define LIBMACRO(name,base,version,flags, ibase) {name, version, flags, (APTR*) &base, (APTR*) &ibase}
#endif

/* THIS is an example how to use the libarray within the application

libstruct LibraryArray[]={
	LIBMACRO( "dos.library"      , DOSBase      , 37, 0                  , IDOS),
	LIBMACRO( "asl.library"      , AslBase      , 0 , LIBFLAGSF_NOTNEEDED, IAsl),
	LIBMACRO( "intuition.library", IntuitionBase, 0 , 0                  , IIntuition),
    {NULL,0,0,NULL}
};*/

/* \\\ */

/*
** Prototypes
*/

BOOL Libraries_Open( void );
void Libraries_Close( void );

/*************************************************************************/

#endif /* LIBRARIES_H */


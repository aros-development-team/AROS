#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Data typing - must be included before any other file.
    Lang: english

    Original by iaint.
    This version of exec/types.h does not contain any of the definitions
    of obsolete types (eg SHORT, CPTR, ...). If your code contains any
    of these they should have been changed a long time ago.
*/

#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif

typedef void *				APTR;	/* memory pointer */
typedef unsigned long			IPTR;

typedef   signed long			LONG;	/* signed 32-bit value */
typedef unsigned long			ULONG;	/* unsigned 32-bit value */

typedef   signed long long		QUAD;	/* signed 64-bit value */
typedef unsigned long long		UQUAD;	/* unsigned 64-bit-value */

typedef   signed short			WORD;	/* signed 16-bit value */
typedef unsigned short			UWORD;	/* unsigned 16-bit value */

typedef signed char 			BYTE;	/* signed 8-bit value */
typedef unsigned char			UBYTE;	/* unsigned 8-bit value */

typedef unsigned long			STACKIPTR;

typedef   signed long			STACKLONG;   /* signed 32-bit value */
typedef unsigned long			STACKULONG;  /* unsigned 32-bit value */

typedef   signed long long		STACKQUAD;   /* signed 64-bit value */
typedef unsigned long long		STACKUQUAD;  /* unsigned 64-bit-value */

typedef   signed int			STACKWORD;   /* signed 16-bit value */
typedef unsigned int			STACKUWORD;  /* unsigned 16-bit value */

typedef signed int			STACKBYTE;   /* signed 8-bit value */
typedef unsigned int			STACKUBYTE;  /* unsigned 8-bit value */

typedef double				STACKFLOAT;  /* signed 32-bit floating point value */

typedef UBYTE * STRPTR;	/* Pointer to string (NULL terminated) */

typedef short   BOOL;	/* A Boolean value */

typedef float   FLOAT;	/* 32bit IEEE floating point value */

typedef double  DOUBLE;	/* 64bit IEEE floating point value */

#ifndef FALSE
#   define FALSE   0L
#endif

#ifndef TRUE
#   define TRUE    1L
#endif

#ifndef NULL
#   define NULL    0L
#endif

#ifndef VOID
#   define VOID    void
#endif

#define GLOBAL	 extern
#define IMPORT	 extern
#define STATIC	 static
#define REGISTER register

/*
    Minimum support library version. AROS doesn't have system libraries
    below V40
*/
#define LIBRARY_MINIMUM 40

/*
    The current version of the includes. Do not use this value in calls
    to OpenLibrary(). Some system libraries may not be at this version. */
#define INCLUDE_VERSION 40

#endif /* exec/types.h */

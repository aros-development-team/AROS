#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/11/18 09:23:56  aros
    Fixed typo (missing U)

    Revision 1.2  1996/10/14 11:16:25  digulla
    Protect all typedefs with #define __typedef_*

    Revision 1.1  1996/09/13 17:55:10  digulla
    AROS special include file exec/types.h. Supports QUAD and IPTR


    Desc: Data typing - must be included before any other file.
    Lang: english

    This version of exec/types.h does not contain any of the definitions
    of obsolete types (eg SHORT, CPTR, ...). If your code contains any
    of these they should have been changed a long time ago.
*/
#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif

/*************************************
 ***** Basic Data types 	 *****
 *************************************/

#ifndef __typedef_APTR
#define __typedef_APTR
typedef void *		    APTR;   /* memory pointer */
#endif

/* An integer which can store a pointer */
#ifndef __typedef_IPTR
#   define __typedef_IPTR
#   ifdef AROS_IPTR_TYPE
	typedef AROS_IPTR_TYPE		    IPTR;
#   else
	typedef unsigned long		    IPTR;
#   endif
#endif

/* Distinguish between 64 and 32bit systems */
#ifndef __typedef_LONG
#define __typedef_LONG
#ifdef AROS_32BIT_TYPE
typedef   signed AROS_32BIT_TYPE    LONG;   /* signed 32-bit value */
typedef unsigned AROS_32BIT_TYPE    ULONG;  /* unsigned 32-bit value */
#else
typedef   signed long		    LONG;   /* signed 32-bit value */
typedef unsigned long		    ULONG;  /* unsigned 32-bit value */
#endif
#endif

#ifndef __typedef_QUAD
#define __typedef_QUAD
#ifdef AROS_64BIT_TYPE
typedef   signed AROS_64BIT_TYPE    QUAD;   /* signed 64-bit value */
typedef unsigned AROS_64BIT_TYPE    UQUAD;  /* unsigned 64-bit-value */
#else
#ifdef __GNUC__
typedef   signed long long	    QUAD;   /* signed 64-bit value */
typedef unsigned long long	    UQUAD;  /* unsigned 64-bit-value */
#else
typedef struct {  LONG high, low }  QUAD;   /* signed 64-bit value */
typedef struct { ULONG high, low }  UQUAD;  /* unsigned 64-bit value */
#endif /* __GNUC__ */
#endif /* AROS_64BIT_TYPE */
#endif

#ifndef __typedef_WORD
#define __typedef_WORD
#ifdef AROS_16BIT_TYPE
typedef   signed AROS_16BIT_TYPE    WORD;   /* signed 64-bit value */
typedef unsigned AROS_16BIT_TYPE    UWORD;  /* unsigned 64-bit-value */
#else
typedef   signed short		    WORD;   /* signed 16-bit value */
typedef unsigned short		    UWORD;  /* unsigned 16-bit value */
#endif
#endif

#ifndef __typedef_BYTE
#define __typedef_BYTE
typedef signed char		    BYTE;   /* signed 8-bit value */
typedef unsigned char		    UBYTE;  /* unsigned 8-bit value */
#endif


/*************************************
 ***** Other interesting types	 *****
 *************************************/

#ifndef __typedef_STRPTR
#define __typedef_STRPTR
typedef UBYTE * STRPTR;     /* Pointer to string (NULL terminated) */
#endif

#ifndef __typedef_BOOL
#define __typedef_BOOL
typedef short	BOOL;	    /* A Boolean value */
#endif

#ifndef __typedef_FLOAT
#define __typedef_FLOAT
typedef float	FLOAT;	    /* 32bit IEEE floating point value */
#endif

#ifndef __typedef_DOUBLE
#define __typedef_DOUBLE
typedef double	DOUBLE;     /* 64bit IEEE floating point value */
#endif


/*************************************
 ***** Some useful definitions	 *****
 *************************************/

#if !defined(FALSE)
#define FALSE	0L
#endif

#if !defined(TRUE)
#define TRUE	1L
#endif

#if !defined(NULL)
#define NULL	((void *)0L)
#endif

#if !defined(VOID)
#define VOID	void
#endif

#define GLOBAL	 extern
#define IMPORT	 extern
#define STATIC	 static
#define REGISTER register

/*
    Minimum support library version. AROS doesn´t have system libraries
    below V40
*/
#define LIBRARY_MINIMUM 40

/*
    The current version of the includes. Do not use this value in calls
    to OpenLibrary(). Some system libraries may not be at this version. */
#define INCLUDE_VERSION 40

#endif /* exec/types.h */

#ifndef OOP_TYPES_H
#define OOP_TYPES_H

/*
    Copyright © 1995-96, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Data typing - must be included before any other file.
    Lang: english

*/

/*************************************
 ***** Basic Data types 	 *****
 *************************************/

#ifndef __typedef_APTR
#   define __typedef_APTR
    typedef void *				APTR;	/* memory pointer */
#endif

/* An integer which can store a pointer */
#ifndef __typedef_IPTR
#   define __typedef_IPTR
#   ifdef AROS_IPTR_TYPE
	typedef AROS_IPTR_TYPE			IPTR;
#   else
	typedef unsigned long			IPTR;
#   endif
#endif

/* Distinguish between 64 and 32bit systems */
#ifndef __typedef_LONG
#   define __typedef_LONG
#   ifdef AROS_32BIT_TYPE
	typedef   signed AROS_32BIT_TYPE	LONG;	/* signed 32-bit value */
	typedef unsigned AROS_32BIT_TYPE	ULONG;	/* unsigned 32-bit value */
#   else
	typedef   signed long			LONG;	/* signed 32-bit value */
	typedef unsigned long			ULONG;	/* unsigned 32-bit value */
#   endif
#endif

#ifndef __typedef_QUAD
#   define __typedef_QUAD
#   ifdef AROS_64BIT_TYPE
	typedef   signed AROS_64BIT_TYPE	QUAD;	/* signed 64-bit value */
	typedef unsigned AROS_64BIT_TYPE	UQUAD;	/* unsigned 64-bit-value */
#   else
#	ifdef __GNUC__
	    typedef   signed long long		QUAD;	/* signed 64-bit value */
	    typedef unsigned long long		UQUAD;	/* unsigned 64-bit-value */
#	else
	    typedef struct {  LONG high, low; }	QUAD;	/* signed 64-bit value */
	    typedef struct { ULONG high, low; }	UQUAD;	/* unsigned 64-bit value */
#	endif /* __GNUC__ */
#   endif /* AROS_64BIT_TYPE */
#endif

#ifndef __typedef_WORD
#   define __typedef_WORD
#   ifdef AROS_16BIT_TYPE
	typedef   signed AROS_16BIT_TYPE	WORD;	/* signed 64-bit value */
	typedef unsigned AROS_16BIT_TYPE	UWORD;	/* unsigned 64-bit-value */
#   else
	typedef   signed short			WORD;	/* signed 16-bit value */
	typedef unsigned short			UWORD;	/* unsigned 16-bit value */
#   endif
#endif

#ifndef __typedef_BYTE
#   define __typedef_BYTE
    typedef signed char 			BYTE;	/* signed 8-bit value */
    typedef unsigned char			UBYTE;	/* unsigned 8-bit value */
#endif

/* An integer on the stack which can store a pointer */
#ifndef __typedef_STACKIPTR
#   define __typedef_STACKIPTR
#   ifdef AROS_IPTR_STACKTYPE
	typedef AROS_IPTR_STACKTYPE		STACKIPTR;
#   else
	typedef unsigned long			STACKIPTR;
#   endif
#endif

/* Distinguish between 64 and 32bit systems on the stack */
#ifndef __typedef_STACKLONG
#   define __typedef_STACKLONG
#   ifdef AROS_32BIT_STACKTYPE
	typedef   signed AROS_32BIT_STACKTYPE	STACKLONG;   /* signed 32-bit value */
	typedef unsigned AROS_32BIT_STACKTYPE	STACKULONG;  /* unsigned 32-bit value */
#   else
	typedef   signed long			STACKLONG;   /* signed 32-bit value */
	typedef unsigned long			STACKULONG;  /* unsigned 32-bit value */
#   endif
#endif

#ifndef __typedef_STACKQUAD
#   define __typedef_STACKQUAD
#   ifdef AROS_64BIT_STACKTYPE
	typedef   signed AROS_64BIT_STACKTYPE	STACKQUAD;   /* signed 64-bit value */
	typedef unsigned AROS_64BIT_STACKTYPE	STACKUQUAD;  /* unsigned 64-bit-value */
#   else
#	ifdef __GNUC__
	    typedef   signed long long		STACKQUAD;   /* signed 64-bit value */
	    typedef unsigned long long		STACKUQUAD;  /* unsigned 64-bit-value */
#	else
	    typedef struct {  LONG high, low; }	STACKQUAD;   /* signed 64-bit value */
	    typedef struct { ULONG high, low; }	STACKUQUAD;  /* unsigned 64-bit value */
#	endif /* __GNUC__ */
#   endif /* AROS_64BIT_STACKTYPE */
#endif

#ifndef __typedef_STACKWORD
#   define __typedef_STACKWORD
#   ifdef AROS_16BIT_STACKTYPE
	typedef   signed AROS_16BIT_STACKTYPE	STACKWORD;   /* signed 64-bit value */
	typedef unsigned AROS_16BIT_STACKTYPE	STACKUWORD;  /* unsigned 64-bit-value */
#   else
	typedef   signed int			STACKWORD;   /* signed 16-bit value */
	typedef unsigned int			STACKUWORD;  /* unsigned 16-bit value */
#   endif
#endif

#ifndef __typedef_STACKBYTE
#   define __typedef_STACKBYTE
    typedef signed int				STACKBYTE;   /* signed 8-bit value */
    typedef unsigned int			STACKUBYTE;  /* unsigned 8-bit value */
#endif

#ifndef __typedef_STACKFLOAT
#   define __typedef_STACKFLOAT
#   ifdef AROS_FLOAT_STACKTYPE
	typedef AROS_FLOAT_STACKTYPE		STACKFLOAT;  /* signed 32-bit floating point value */
#   else
	typedef double				STACKFLOAT;  /* signed 32-bit floating point value */
#   endif
#endif


/*************************************
 ***** Other interesting types	 *****
 *************************************/

#ifndef __typedef_STRPTR
#   define __typedef_STRPTR
    typedef UBYTE * STRPTR;	/* Pointer to string (NULL terminated) */
#endif

#ifndef __typedef_BOOL
#   define __typedef_BOOL
    typedef short   BOOL;	/* A Boolean value */
#endif

#ifndef __typedef_FLOAT
#   define __typedef_FLOAT
    typedef float   FLOAT;	/* 32bit IEEE floating point value */
#endif

#ifndef __typedef_DOUBLE
#   define __typedef_DOUBLE
    typedef double  DOUBLE;	/* 64bit IEEE floating point value */
#endif


/*************************************
 ***** Some useful definitions	 *****
 *************************************/

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


#endif /* OOP_TYPES_H */

/*
    Copyright © 1995-96, The AROS Development Team. All rights reserved.
    Debugging macros.

    This include file can be included several times !
*/

#include <stdio.h>

extern int _SD_SpacesWritten;

#ifndef DEBUG
#   define DEBUG 0
#endif
#ifndef SDEBUG
#   define SDEBUG 0
#endif


/* Remove all macros. They get new values each time this file is
    included */
#undef D
#undef DB2
#undef ReturnVoid
#undef ReturnPtr
#undef ReturnStr
#undef ReturnInt
#undef ReturnXInt
#undef ReturnFloat
#undef ReturnSpecial
#undef ReturnBool

/*  Macros for "stair debugging" */
#undef SDInit
#undef EnterFunc
#undef Indent
#undef ExitFunc

#if SDEBUG

#   ifndef SDEBUG_INDENT
#	define SDEBUG_INDENT 2
#   endif

/* This is some new macros for making debug output more readable,
** by indenting for each functioncall made.
** Usage: Call the SDInit() macro before anything else in your main().
** Start the functions you want to debug with EnterFunc(bug("something"))
** and ALWAYS match these with a Returnxxxx type macro
** at the end of the func.
** Inside the func you can use the normal D(bug()) macro.
** 
** To enable the macros, just add a #define SDEBUG 1
*/

/* User macro */
#define EnterFunc(x) {   			\
	int _written;				\
   	for (_written = 0; _written < _SD_SpacesWritten; _written ++) printf(" "); \
   	_SD_SpacesWritten += SDEBUG_INDENT;		} \
	x

/* User macro. Add into start of your main() routine */
#   define SDInit	\
	int _SD_SpacesWritten = 0


/* Internal */
#   define Indent {   		\
				\
	int _written;		\
   	for (_written = 0; _written < _SD_SpacesWritten; _written ++) printf(" "); }
   	
/* Internal */
#define ExitFunc { 				\
	int _written;				\
   	_SD_SpacesWritten -= SDEBUG_INDENT;		\
   	for (_written = 0; _written < _SD_SpacesWritten; _written ++) printf(" "); } 


#else

#   define SDInit
#   define Indent
#   define EnterFunc(x) D(x)
#   define ExitFunc

#endif

#if DEBUG
#   define D(x)     Indent x

#   if DEBUG > 1
#	define DB2(x)    x
#   else
#	define DB2(x)    /* eps */
#   endif



    /* return-macros. NOTE: I make a copy of the value in __aros_val, because
       the return-value might have side effects (like return x++;). */
#   define ReturnVoid(name)         { ExitFunc printf ("Exit " name "()\n"); return; }
#   define ReturnPtr(name,type,val) {  type __aros_val = (type)val; \
				    ExitFunc printf ("Exit " name "=%08lx\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnStr(name,type,val) { type __aros_val = (type)val; \
				    ExitFunc printf ("Exit " name "=\"%s\"\n", \
				    __aros_val); return __aros_val; }
#   define ReturnInt(name,type,val) { type __aros_val = (type)val; \
				    ExitFunc printf ("Exit " name "=%ld\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnXInt(name,type,val) { type __aros_val = (type)val; \
				    ExitFunc printf ("Exit " name "=%lx\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnFloat(name,type,val) { type __aros_val = (type)val; \
				    ExitFunc printf ("Exit " name "=%g\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnSpecial(name,type,val,fmt) { type __aros_val = (type)val; \
				    ExitFunc printf ("Exit " name "=" fmt "\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnBool(name,val)     { BOOL __aros_val = (val != 0); \
				    ExitFunc printf ("Exit " name "=%s\n", \
				    __aros_val ? "TRUE" : "FALSE"); \
				    return __aros_val; }
#else /* !DEBUG */
#   define D(x)     /* eps */
#   define DB2(x)     /* eps */

#   define ReturnVoid(name)                 return
#   define ReturnPtr(name,type,val)         return val
#   define ReturnStr(name,type,val)         return val
#   define ReturnInt(name,type,val)         return val
#   define ReturnXInt(name,type,val)        return val
#   define ReturnFloat(name,type,val)       return val
#   define ReturnSpecial(name,type,val,fmt) return val
#   define ReturnBool(name,val)             return val
#endif /* DEBUG */

#ifndef DEBUG_H
#define DEBUG_H

#define bug	 printf

#endif /* DEBUG_H */

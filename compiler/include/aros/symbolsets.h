#ifndef _AROS_SYMBOLSETS_H
#define _AROS_SYMBOLSETS_H

/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Symbol sets support
    Lang: english
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/asmcall.h>

struct libraryset
{
    STRPTR name;
    const ULONG  *versionptr;
    void   **baseptr;
};

extern int set_call_funcs(const void *set[], int direction, int test_fail);
extern int set_open_libraries(void);
extern void set_close_libraries(void);
extern
AROS_UFH3(int, set_call_libfuncs,
	  AROS_UFHA(void**, set, A0),
	  AROS_UFHA(int, order, D0),
	  AROS_UFHA(void*, libbase, A6)
	 );

#define SETNAME(set) __##set##_LIST__

#define DECLARESET(set) \
extern const void * SETNAME(set)[] __attribute__((weak));

#define DEFINESET(set) \
const void * SETNAME(set)[] __attribute__((weak))={0,0};

#define ADD2SET(symbol, _set, pri)\
	static typeof(symbol) * __aros_set_##_set##_##symbol __attribute__((section(".aros.set." #_set "." #pri))) __unused = &symbol;

#define SETELEM(symbol, _set)                  \
({                                             \
    extern void *__aros_set_##_set##_##symbol; \
    &__aros_set_##_set##_##symbol;             \
})

/*
    ctors and dtors sets are used by c++ to store static constructors/destructors
    pointers in them.

    The functions have to be invoked respectively right before the program enters
    the main() function and right after the program exits the main() function.
*/
#define ADD2CTORS(symbol, pri) ADD2SET(symbol, ctors, pri)
#define ADD2DTORS(symbol, pri) ADD2SET(symbol, dtors, pri)

/*
    init and exit sets are like the ctors and dtors sets, except that they take
    precedence on them, that is functions stored in the init set are called
    before the ones stored in the ctors set, and functions stored in the
    exit set are called after the ones stored in the dtors set.

    Moreover, init functions return an int, instead of void like the ctor functions.
    This return value is equal to 0 if the function returned with success, otherwise
    it holds an error code.
*/

#define ADD2INIT(symbol, pri) ADD2SET(symbol, init, pri)
#define ADD2EXIT(symbol, pri) ADD2SET(symbol, exit, pri)

/*
    initlib, expungelib, openlib, closelib sets are similar to the init and exit
    sets, only they are specific to the initialisation phase of libraries.
    initlib/expungelib are only called when a library is loaded/unloaded from
    memory. openlib/closelib is called every time a library is opened/closed.
    The library base is passed to each of the functions present in the sets as
    the first argument in the A6 register. To define a function for one of these
    sets the AROS_SET_LIBFUNC macro can be used.
    
    The return argument is also an int, and just like with the init/exit sets, 
    a zero value indicates an (de)initialization failure, a one indicates succes.
    The calling of the functions in the initlib and the openlib sets stops when
    the first zero value is met. The functions in the expungelib and the
    closelib are always called.

    These sets have a lower priority then the INIT and the CTORS sets. So when
    a library is loaded first the init set is called, then the ctors set and
    finally the initlib set.
*/

#define AROS_SET_LIBFUNC(funcname, libbasetype, libbase) \
    AROS_UFH1(int, funcname, \
	      AROS_UFHA(libbasetype *, libbase, A6) \
	     )
#define ADD2INITLIB(symbol, pri)    ADD2SET(symbol, initlib, pri)
#define ADD2EXPUNGELIB(symbol, pri) ADD2SET(symbol, expungelib, pri)
#define ADD2OPENLIB(symbol, pri)    ADD2SET(symbol, openlib, pri)
#define ADD2CLOSELIB(symbol, pri)   ADD2SET(symbol, closelib, pri)

/* this macro generates the necessary symbols to open and close automatically
   a library. An error message will be shown if the library cannot be open.  */
#define ADD2LIBS(name, ver, btype, bname)                \
btype bname;                                             \
                                                         \
AROS_IMPORT_ASM_SYM(__includelibrarieshandling);         \
                                                         \
const ULONG bname##_version __attribute__((weak)) = ver; \
                                                         \
struct libraryset libraryset_##bname =                   \
{                                                        \
     name, &bname##_version, (void **)&bname             \
};                                                       \
ADD2SET(libraryset_##bname, libs, 0) 

#define ASKFORLIBVERSION(bname, ver) \
const ULONG bname##_version = ver

/* Traverse the set from the first element to the last one, or vice versa,
   depending on the value of 'direction': >=0 means first -> last, <0 means
   last -> first. 
   
   set       - the symbol set
   direction - first->last | last->first
   pos       - integer variable holding the current position in the set
   elem      - variable of the same type of the elements in the set. holds the
               current element.  */
#define ForeachElementInSet(set, direction, pos, elem)                       \
for                                                                          \
(                                                                            \
    pos = (direction >= 0) ? 1 : ((int *)(set))[0];                          \
    elem = (void *)(set)[pos], (direction >= 0) ? elem != NULL : (pos) != 0; \
    pos += (direction >= 0) ? 1 : -1                                         \
) 

/* You must use this macro in the part of your program which handles symbol sets, 
   or else your program won't link when symbolsets are used.
   This is so to ensure that symbolsets are properly handled and thus things get
   properly initialized. */
#define THIS_PROGRAM_HANDLES_SYMBOLSETS \
    AROS_MAKE_ASM_SYM(__this_program_requires_symbol_sets_handling, 0);
    
#endif

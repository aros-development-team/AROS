#ifndef _AROS_SYMBOLSETS_H
#define _AROS_SYMBOLSETS_H

/*
    Copyright � 1995-2002, The AROS Development Team. All rights reserved.
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
    int   (*postopenfunc)(void);
    void  (*preclosefunc)(void);
};

extern int set_call_funcs(void *set[], int order);
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
extern void * SETNAME(set)[] __attribute__((weak));

#define DEFINESET(set) \
void * SETNAME(set)[] __attribute__((weak))={0,0};

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
    
    The return argument is also an int but the opposite to the init and exit sets.
    A zero value indicates an (de)initialization failure, a one indicates succes.
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

/*
  this macro generates the necessary symbols to open and close automatically
  a library. It will make an error message be showed if the library cannot
  be open
*/
#define ADD2LIBS(name, ver, pri, btype, bname, postopenfunc, preclosefunc)   \
btype bname;                                                                         \
extern int __includelibrarieshandling;                                               \
static const int *__setincludelibrarieshandling __unused = &__includelibrarieshandling; \
const ULONG bname##_version __attribute__((weak)) = ver;                     \
struct libraryset libraryset_##bname =                                       \
{                                                                            \
     name, &bname##_version, (void **)&bname, postopenfunc, preclosefunc     \
};                                                                           \
ADD2SET(libraryset_##bname, libs, pri)

#define ASKFORLIBVERSION(bname, ver) \
const ULONG bname##_version = ver

/* some already allocated priorities for library opening/closing */
#define LIBSET_EXEC_PRI          0
#define LIBSET_DOS_PRI           1
#define LIBSET_KEYMAP_PRI        2
#define LIBSET_INTUITION_PRI     3
#define LIBSET_LAYERS_PRI        4
#define LIBSET_GRAPHICS_PRI      5
#define LIBSET_UTILITY_PRI       6
#define LIBSET_IFFPARSE_PRI      7
#define LIBSET_CYBERGRAPHICS_PRI 8
#define LIBSET_DISKFONT_PRI      9
#define LIBSET_LOCALE_PRI        10
#define LIBSET_ASL_PRI           11
#define LIBSET_GADTOOLS_PRI 	 12
#define LIBSET_REALTIME_PRI 	 13
#define LIBSET_ICON_PRI 	 14
#define LIBSET_WORKBENCH_PRI 	 15
#define LIBSET_DATATYPES_PRI 	 16
#define LIBSET_COMMODITIES_PRI 	 17

#define LIBSET_AROSC_PRI         20

/* User priorities starts from here */
#define LIBSET_USER_PRI      100  /*An enough hight value, I think... */

#endif

#ifndef _AROS_SYMBOLSETS_H
#define _AROS_SYMBOLSETS_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Symbol sets support
    Lang: english
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <aros/asmcall.h>

#include <stddef.h>

struct libraryset
{
    CONST_STRPTR name;
    const LONG *versionptr;
    void  **baseptr;
};

struct rellibraryset
{
    CONST_STRPTR name;
    const LONG * const versionptr;
    const IPTR * const baseoffsetptr;
};

#define SETNAME(set) __##set##_LIST__

#define DECLARESET(set) \
extern const void * const SETNAME(set)[] __attribute__((weak));

#define DEFINESET(set) \
const void * const SETNAME(set)[] __attribute__((weak))={0,0};

#define ADD2SET(symbol, _set, pri)\
	static const void * const __aros_set_##_set##_##symbol __attribute__((__section__(".aros.set." #_set "." #pri))) __used = (void *)&symbol;

#define SETELEM(symbol, _set)                        \
({                                                   \
    extern const void * const __aros_set_##_set##_##symbol; \
    &__aros_set_##_set##_##symbol;                   \
})

/*
    ctors and dtors sets are used by c++ to store static constructors/destructors
    pointers in them.

    The functions have to be invoked respectively right before the program enters
    the main() function and right after the program exits the main() function.
*/
#define ADD2CTORS(symbol, pri) ADD2SET(symbol, CTORS, pri)
#define ADD2DTORS(symbol, pri) ADD2SET(symbol, DTORS, pri)

/*
    init and exit sets are like the ctors and dtors sets, except that they take
    precedence on them, that is functions stored in the init set are called
    before the ones stored in the ctors set, and functions stored in the
    exit set are called after the ones stored in the dtors set.

    Moreover, init functions return an int, instead of void like the ctor functions.
    This return value is equal to 0 if the function returned with success, otherwise
    it holds an error code.
*/

#define ADD2INIT(symbol, pri) ADD2SET(symbol, INIT, pri)
#define ADD2EXIT(symbol, pri) ADD2SET(symbol, EXIT, pri)

/*
    initlib, expungelib, openlib, closelib sets are similar to the init and exit
    sets, only they are specific to the initialisation phase of libraries.
    initlib/expungelib are only called when a library is loaded/unloaded from
    memory. openlib/closelib is called every time a library is opened/closed.
    The library base is passed to each of the functions present in the sets as
    the first argument.
    
    The return argument is also an int, and just like with the init/exit sets, 
    a zero value indicates an (de)initialization failure, a one indicates succes.
    The calling of the functions in the initlib and the openlib sets stops when
    the first zero value is met. The functions in the expungelib and the
    closelib are always called.

    These sets have a lower priority then the INIT and the CTORS sets. So when
    a library is loaded first the init set is called, then the ctors set and
    finally the initlib set.
*/

#define ADD2INITLIB(symbol, pri)    ADD2SET(symbol, INITLIB, pri)
#define ADD2EXPUNGELIB(symbol, pri) ADD2SET(symbol, EXPUNGELIB, pri)
#define ADD2OPENLIB(symbol, pri)    ADD2SET(symbol, OPENLIB, pri)
#define ADD2CLOSELIB(symbol, pri)   ADD2SET(symbol, CLOSELIB, pri)
#define ADD2OPENDEV(symbol, pri)    ADD2SET(symbol, OPENDEV, pri)
#define ADD2CLOSEDEV(symbol, pri)   ADD2SET(symbol, CLOSEDEV, pri)

/* this macro generates the necessary symbols to open and close automatically
   a library. An error message will be shown if the library cannot be opened.  */
#define AROS_LIBSET(name, btype, bname)                        \
btype bname;                                                   \
extern const LONG const __aros_libreq_##bname __attribute__((weak)); \
                                                               \
AROS_IMPORT_ASM_SYM(int, dummy, __includelibrarieshandling);   \
                                                               \
static const struct libraryset const __aros_libset_##bname =         \
{                                                              \
     name, &__aros_libreq_##bname, (void *)&bname              \
};                                                             \
ADD2SET(__aros_libset_##bname, LIBS, 0) 

/* NOTE: Users of this symbolset will need to provide:
 * 
 * const IPTR __aros_rellib_offset_##bname = offsetof(struct MyBase, my_bname);
 * AROS_IMPORT_ASM_SYM(void *, _##bname, __aros_rellib_base_##bname);
 */
#define AROS_RELLIBSET(name, btype, bname)                           \
const ULONG const __aros_rellib_base_##bname = 0;                    \
extern const IPTR const __aros_rellib_offset_##bname;                \
extern const LONG const __aros_libreq_##bname __attribute__((weak)); \
                                                                     \
AROS_IMPORT_ASM_SYM(int, dummy, __includerellibrarieshandling);      \
                                                                     \
static const struct rellibraryset const __aros_rellibset_##bname =   \
{                                                                    \
     name, &__aros_libreq_##bname, &__aros_rellib_offset_##bname     \
};                                                                   \
ADD2SET(__aros_rellibset_##bname, RELLIBS, 0) 


#define ADD2LIBS(name, ver, btype, bname) \
AROS_LIBSET(name, btype, bname)           \
const LONG const __aros_libreq_##bname = ver;

#define AROS_LIBREQ(bname, ver) \
    asm volatile ( \
                  ".global __aros_libreq_" #bname "." #ver "\n" \
                  "__aros_libreq_" #bname "." #ver "=" #ver);

#define SETRELLIBOFFSET(bname, libbasetype, fname) \
const IPTR __aros_rellib_offset_##bname = offsetof(libbasetype, fname); \
AROS_IMPORT_ASM_SYM(void *, _##bname, __aros_rellib_base_##bname);

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
    pos = (direction >= 0) ? 1 : ((long *)(set))[0];                         \
    elem = (void *)(set)[pos], (direction >= 0) ? elem != NULL : (pos) != 0; \
    pos += (direction >= 0) ? 1 : -1                                         \
) 

/* You must use this macro in the part of your program which handles symbol sets, 
   or else your program won't link when symbolsets are used.
   This is so to ensure that symbolsets are properly handled and thus things get
   properly initialized. */
#define THIS_PROGRAM_HANDLES_SYMBOLSET(x) \
    AROS_MAKE_ASM_SYM(int, __##x##__symbol_set_handler_missing, __##x##__symbol_set_handler_missing, 0); \
    AROS_EXPORT_ASM_SYM(__##x##__symbol_set_handler_missing);
    
#endif

/* Function prototypes from autoinit and libinit */
extern int _set_call_funcs(const void * const set[], int direction, int test_fail, struct ExecBase *sysBase);
#define set_call_funcs(set, dir, fail) _set_call_funcs(set, dir, fail, SysBase)
extern int set_call_libfuncs(const void * const *set, int order, int test_fail, void *libbase);
extern int set_call_devfuncs
(
    const void * const *set,
    int order,
    int test_fail,
    void *libbase,
    void *ioreq,
    IPTR unitnum,
    ULONG flags
);

DECLARESET(LIBS)
DECLARESET(RELLIBS)

#define set_open_libraries() set_open_libraries_list(SETNAME(LIBS))
#define set_open_rellibraries(base) set_open_rellibraries_list(base,SETNAME(RELLIBS))
#define set_close_libraries() set_close_libraries_list(SETNAME(LIBS))
#define set_close_rellibraries(base) set_close_rellibraries_list(base,SETNAME(RELLIBS))
#define set_open_libraries_list(list) _set_open_libraries_list(list, SysBase)
#define set_open_rellibraries_list(base, list) _set_open_rellibraries_list(base, list, SysBase)
#define set_close_libraries_list(list) _set_close_libraries_list(list, SysBase)
#define set_close_rellibraries_list(base, list) _set_close_rellibraries_list(base, list, SysBase)
extern int _set_open_libraries_list(const void * const list[], struct ExecBase *sysBase);
extern int _set_open_rellibraries_list(APTR base, const void * const list[], struct ExecBase *sysBase);
extern void _set_close_libraries_list(const void * const list[], struct ExecBase *sysBase);
extern void _set_close_rellibraries_list(APTR base, const void * const list[], struct ExecBase *sysBase);

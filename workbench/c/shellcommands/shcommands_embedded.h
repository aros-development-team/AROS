#ifndef SHCOMMANDS_EMBEDDED_H
#define SHCOMMANDS_EMBEDDED_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>

#define _stringify(x) #x
#define stringify(x) _stringify(x)

#define SHArg(name) __shargs[SHA_##name]

#define __SHA_ENUM(abbr, name, modf, def) SHA_##name
#define __SHA_DEF(abbr, name, modf, def) (IPTR)(def)
#define __SHA_OPT(abbr, name, modf, def) \
     stringify(abbr) stringify(name) stringify(modf)


struct shcommand
{
    STRPTR      name;
    ULONG      (*command)();
    ULONG       numargs;
    const IPTR *defaults;
    STRPTR      templ;
};

#define __AROS_SH_ARGS(name, numargs, defl, templ)                   \
static ULONG name##_main(IPTR *, struct ExecBase *SysBase,           \
                         struct DosLibrary *);                       \
static const IPTR __##name##_##defaults[numargs] = defl;             \
struct shcommand __##name##_##shcommand =                            \
{                                                                    \
    stringify(name),                                                 \
    name##_main,                                                     \
    numargs,                                                         \
    __##name##_##defaults,                                           \
    templ                                                            \
};                                                                   \
                                                                     \
ADD2SET(__##name##_##shcommand, __SHCOMMANDS__, 0);                  \
							             \
static ULONG name##_main(IPTR *__shargs, struct ExecBase *SysBase,   \
                         struct DosLibrary *DOSBase)                 \
{

#define AROS_SHCOMMAND_INIT

#define AROS_SHCOMMAND_EXIT \
    }                       \
    return RETURN_OK;       \
}


#define AROS_SH0(name, version)                     \
ULONG name##_main(struct DOSBase *DOSBase);         \
struct struct shcommand __##name##_##shcommand =    \
{                                                   \
    stringify(name),                                \
    name##_main,                                    \
    NULL,                                           \
    NULL                                            \
};                                                  \
                                                    \
ADD2SET(__##name##_##shcommand, __SHCOMMANDS__, 0); \
						    \
ULONG name##_main(struct DOSBase *DOSBase)          \
{

#define __DEF(x...) {x}

#define AROS_SH1(name, version, a1)               \
    __AROS_SH_ARGS(name, 1, __DEF(__SHA_DEF(a1)), \
                            __SHA_OPT(a1))        \
    {                                             \
        enum {__SHA_ENUM(a1)};

#define AROS_SH2(name, version, a1, a2)                          \
    __AROS_SH_ARGS(name, 2, __DEF(__SHA_DEF(a1), __SHA_DEF(a2)), \
                            __SHA_OPT(a1) "," __SHA_OPT(a2))     \
    {                                                            \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)};

#define AROS_SH3(name, version, a1, a2, a3)                     \
    __AROS_SH_ARGS(name, 3, __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3)),                     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
	  	            __SHA_OPT(a3))                      \
    {                                                           \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2),                   \
	      __SHA_ENUM(a3)};

#define AROS_SH4(name, version, a1, a2, a3, a4)                 \
    __AROS_SH_ARGS(name, 4, __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3), __SHA_DEF(a4)),      \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
		            __SHA_OPT(a3) "," __SHA_OPT(a4))    \
    {                                                           \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2),                   \
	      __SHA_ENUM(a3), __SHA_ENUM(a4)};

#define AROS_SH5(name, version, a1, a2, a3, a4, a5)             \
    __AROS_SH_ARGS(name, 5, __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3), __SHA_DEF(a4),       \
		            __SHA_DEF(a5)),                     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
                            __SHA_OPT(a3) "," __SHA_OPT(a4) "," \
		            __SHA_OPT(a5))                      \
    {                                                           \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2), __SHA_ENUM(a3),   \
	      __SHA_ENUM(a4), __SHA_ENUM(a5)};                  \


#define AROS_SHA(abbr, name, modf, def) abbr,name,modf,def

#endif

#ifndef SHCOMMANDS_EMBEDDED_H
#define SHCOMMANDS_EMBEDDED_H

#include <dos/dos.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <aros/asmcall.h>

#define _stringify(x) #x
#define stringify(x) _stringify(x)

#define SHArg(name) __shargs[SHA_##name]
#define SHReturn(code) return (code)

#define __SHA_ENUM(abbr, name, modf, def) SHA_##name
#define __SHA_DEF(abbr, name, modf, def) (IPTR)(def)
#define __SHA_OPT(abbr, name, modf, def) \
     stringify(abbr) stringify(name) stringify(modf)


#define __AROS_SH_ARGS(name, numargs, defl, templ)             \
static ULONG name##_main(IPTR *, struct ExecBase *SysBase,     \
                         struct DosLibrary *);                 \
AROS_UFH3(LONG, entry,                                         \
    AROS_UFHA(char *,argstr,A0),                               \
    AROS_UFHA(ULONG,argsize,D0),                               \
    AROS_UFHA(struct ExecBase *,SysBase,A6)                    \
)                                                              \
{                                                              \
    AROS_USERFUNC_INIT                                         \
							       \
    LONG __retcode = RETURN_FAIL;                              \
    IPTR __shargs[numargs] = defl;                             \
    struct RDArgs *__rda;                                      \
    struct DosLibrary *DOSBase =                               \
        (struct DosLibrary *) OpenLibrary(DOSNAME, 37);        \
                                                               \
    if (!DOSBase)                                              \
    {                                                          \
        SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);              \
	goto __exit;                                           \
    }                                                          \
                                                               \
    __rda = ReadArgs(templ, __shargs, NULL);                   \
					                       \
    if (!__rda)                                                \
    {                                                          \
        PrintFault(IoErr(), stringify(name));                  \
	goto __exit;                                           \
    }                                                          \
							       \
    __retcode = name##_main(__shargs, SysBase, DOSBase);       \
    							       \
__exit:                                                        \
    if (__rda) FreeArgs(__rda);                                \
    if (DOSBase) CloseLibrary((struct Library *)DOSBase);      \
                                                               \
    return __retcode;                                          \
							       \
    AROS_USERFUNC_EXIT                                         \
}                                                              \
                                                               \
static ULONG name##_main(IPTR *__shargs, struct ExecBase *SysBase,   \
                         struct DosLibrary *DOSBase)                 \
{                                                                    \

#define AROS_SHCOMMAND_INIT

#define AROS_SHCOMMAND_EXIT \
    }                       \
}


#define AROS_SH0(name, version, date)                          \
static ULONG name##_main(struct ExecBase *SysBase,             \
                         struct DosLibrary *);                 \
AROS_UFH3(LONG, entry,                                         \
    AROS_UFHA(char *,argstr,A0),                               \
    AROS_UFHA(ULONG,argsize,D0),                               \
    AROS_UFHA(struct ExecBase *,SysBase,A6)                    \
)                                                              \
{                                                              \
    AROS_USERFUNC_INIT                                         \
							       \
    LONG __retcode = RETURN_FAIL;                              \
    struct DosLibrary *DOSBase =                               \
        (struct DosLibrary *) OpenLibrary(DOSNAME, 0);         \
                                                               \
    if (!DOSBase)                                              \
    {                                                          \
        SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);              \
	goto __exit;                                           \
    }                                                          \
                                                               \
    __retcode = name##_main(SysBase, DOSBase);                 \
    							       \
__exit:                                                        \
    if (DOSBase) CloseLibrary((struct Library *)DOSBase);      \
                                                               \
    return __retcode;                                          \
							       \
    AROS_USERFUNC_EXIT                                         \
}                                                              \
                                                               \
static ULONG name##_main(struct ExecBase *SysBase,             \
                         struct DosLibrary *DOSBase)           \
{

#define __DEF(x...) {x}

#define AROS_SH1(name, version, date, a1)         \
    __AROS_SH_ARGS(name, 1, __DEF(__SHA_DEF(a1)), \
                            __SHA_OPT(a1))        \
    {                                             \
        enum {__SHA_ENUM(a1)};

#define AROS_SH2(name, version, date, a1, a2)                    \
    __AROS_SH_ARGS(name, 2, __DEF(__SHA_DEF(a1), __SHA_DEF(a2)), \
                            __SHA_OPT(a1) "," __SHA_OPT(a2))     \
    {                                                            \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)};

#define AROS_SH3(name, version, date, a1, a2, a3)               \
    __AROS_SH_ARGS(name, 3, __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3)),                     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
	  	            __SHA_OPT(a3))                      \
    {                                                           \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)                    \
	      __SHA_ENUM(a3)};

#define AROS_SH4(name, version, date, a1, a2, a3, a4)           \
    __AROS_SH_ARGS(name, 4, __DEF(__SHA_DEF(a1), __SHA_DEF(a2)  \
                            __SHA_DEF(a3), __SHA_DEF(a4)),      \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
		            __SHA_OPT(a3) "," __SHA_OPT(a4))    \
    {                                                           \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)                    \
	      __SHA_ENUM(a3), __SHA_ENUM(a4)};

#define AROS_SH5(name, version, date, a1, a2, a3, a4, a5)       \
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

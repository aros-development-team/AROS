#ifndef SHCOMMANDS_EMBEDDED_H
#define SHCOMMANDS_EMBEDDED_H

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <aros/asmcall.h>

#define _stringify(x) #x
#define stringify(x) _stringify(x)

#define SHArg(name) ((SHA_##name##_type)__shargs[SHA_##name])

#define __SHA_ENUM(type, abbr, name, modf, def) SHA_##name
#define __SHA_DEF(type, abbr, name, modf, def) (IPTR)(def)
#define __SHA_OPT(type, abbr, name, modf, def) \
     stringify(abbr) stringify(name) stringify(modf)
#define __SHA_TYPEDEF(type, abbr, name, modf, def) \
     typedef type SHA_##name##_type



#define __AROS_SH_ARGS(name, version, numargs, defl, templ)    \
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
    struct RDArgs *__rda = NULL;                               \
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
static const char name##_version[] = "$VER: "                        \
                                     stringify(name) " "             \
				     stringify(version) " "          \
				     "(" __DATE__ ")\n";  \
                                                                     \
static ULONG name##_main(IPTR *__shargs, struct ExecBase *SysBase,   \
                         struct DosLibrary *DOSBase)                 \
{                                                                    \

#define AROS_SHCOMMAND_INIT

#define AROS_SHCOMMAND_EXIT \
    }                       \
}

#define __DEF(x...) {x}

#define AROS_SH0(name, version)                   \
    __AROS_SH_ARGS(name, version, 1, {0}, "")     \
    {                                             \

#define AROS_SH1(name, version, a1)               \
    __AROS_SH_ARGS(name, version, 1,              \
                            __DEF(__SHA_DEF(a1)), \
                            __SHA_OPT(a1))        \
    {                                             \
	__SHA_TYPEDEF(a1);                        \
	enum {__SHA_ENUM(a1)};

#define AROS_SH2(name, version, a1, a2)                          \
    __AROS_SH_ARGS(name, version, 2,                             \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2)), \
                            __SHA_OPT(a1) "," __SHA_OPT(a2))     \
    {                                                            \
	__SHA_TYPEDEF(a1);                                       \
	__SHA_TYPEDEF(a2);                                       \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)};

#define AROS_SH3(name, version, a1, a2, a3)                     \
    __AROS_SH_ARGS(name, version, 3,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3)),                     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
	  	            __SHA_OPT(a3))                      \
    {                                                           \
	__SHA_TYPEDEF(a1);                                      \
	__SHA_TYPEDEF(a2);                                      \
	__SHA_TYPEDEF(a3);                                      \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)                    \
	      __SHA_ENUM(a3)};

#define AROS_SH4(name, version, a1, a2, a3, a4)                 \
    __AROS_SH_ARGS(name, version, 4,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2)  \
                            __SHA_DEF(a3), __SHA_DEF(a4)),      \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
		            __SHA_OPT(a3) "," __SHA_OPT(a4))    \
    {                                                           \
	__SHA_TYPEDEF(a1);                                      \
	__SHA_TYPEDEF(a2);                                      \
	__SHA_TYPEDEF(a3);                                      \
	__SHA_TYPEDEF(a4);                                      \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)                    \
	      __SHA_ENUM(a3), __SHA_ENUM(a4)};

#define AROS_SH5(name, version, a1, a2, a3, a4, a5)             \
    __AROS_SH_ARGS(name, version, 5,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3), __SHA_DEF(a4),       \
		            __SHA_DEF(a5)),                     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
                            __SHA_OPT(a3) "," __SHA_OPT(a4) "," \
		            __SHA_OPT(a5))                      \
    {                                                           \
	__SHA_TYPEDEF(a1);                                      \
	__SHA_TYPEDEF(a2);                                      \
	__SHA_TYPEDEF(a3);                                      \
	__SHA_TYPEDEF(a4);                                      \
	__SHA_TYPEDEF(a5);                                      \
	enum {__SHA_ENUM(a1), __SHA_ENUM(a2), __SHA_ENUM(a3),   \
	      __SHA_ENUM(a4), __SHA_ENUM(a5)};

#define AROS_SH10(name, version, a1, a2, a3, a4, a5,             \
                                 a6, a7, a8, a9, a10)            \
    __AROS_SH_ARGS(name, version, 10,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2),  \
                            __SHA_DEF(a3), __SHA_DEF(a4),        \
		            __SHA_DEF(a5), __SHA_DEF(a6),        \
			    __SHA_DEF(a7), __SHA_DEF(a8),        \
			    __SHA_DEF(a9), __SHA_DEF(a10)),      \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) ","  \
                            __SHA_OPT(a3) "," __SHA_OPT(a4) ","  \
		            __SHA_OPT(a5) "," __SHA_OPT(a6) ","  \
			    __SHA_OPT(a7) "," __SHA_OPT(a8) ","  \
			    __SHA_OPT(a9) "," __SHA_OPT(a10))    \
    {                                                            \
	__SHA_TYPEDEF(a1);                                       \
	__SHA_TYPEDEF(a2);                                       \
	__SHA_TYPEDEF(a3);                                       \
	__SHA_TYPEDEF(a4);                                       \
	__SHA_TYPEDEF(a5);                                       \
	__SHA_TYPEDEF(a6);                                       \
	__SHA_TYPEDEF(a7);                                       \
	__SHA_TYPEDEF(a8);                                       \
	__SHA_TYPEDEF(a9);                                       \
	__SHA_TYPEDEF(a10);                                      \
	enum {__SHA_ENUM(a1), __SHA_ENUM(a2), __SHA_ENUM(a3),    \
	      __SHA_ENUM(a4), __SHA_ENUM(a5), __SHA_ENUM(a6),    \
	      __SHA_ENUM(a7), __SHA_ENUM(a8), __SHA_ENUM(a9),    \
	      __SHA_ENUM(a10)};

#define AROS_SH11(name, version, a1, a2, a3, a4, a5,             \
                                 a6, a7, a8, a9, a10,            \
				 a11)                            \
    __AROS_SH_ARGS(name, version, 11,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2),  \
                            __SHA_DEF(a3), __SHA_DEF(a4),        \
		            __SHA_DEF(a5), __SHA_DEF(a6),        \
			    __SHA_DEF(a7), __SHA_DEF(a8),        \
			    __SHA_DEF(a9), __SHA_DEF(a10),        \
			    __SHA_DEF(a11)),                     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) ","  \
                            __SHA_OPT(a3) "," __SHA_OPT(a4) ","  \
		            __SHA_OPT(a5) "," __SHA_OPT(a6) ","  \
			    __SHA_OPT(a7) "," __SHA_OPT(a8) ","  \
			    __SHA_OPT(a9) "," __SHA_OPT(a10) "." \
			    __SHA_OPT(a11))                      \
    {                                                            \
	__SHA_TYPEDEF(a1);                                       \
	__SHA_TYPEDEF(a2);                                       \
	__SHA_TYPEDEF(a3);                                       \
	__SHA_TYPEDEF(a4);                                       \
	__SHA_TYPEDEF(a5);                                       \
	__SHA_TYPEDEF(a6);                                       \
	__SHA_TYPEDEF(a7);                                       \
	__SHA_TYPEDEF(a8);                                       \
	__SHA_TYPEDEF(a9);                                       \
	__SHA_TYPEDEF(a10);                                      \
	__SHA_TYPEDEF(a11);                                      \
	enum {__SHA_ENUM(a1), __SHA_ENUM(a2), __SHA_ENUM(a3),    \
	      __SHA_ENUM(a4), __SHA_ENUM(a5), __SHA_ENUM(a6),    \
	      __SHA_ENUM(a7), __SHA_ENUM(a8), __SHA_ENUM(a9),    \
	      __SHA_ENUM(a10), __SHA_ENUM(a11)};


#define AROS_SHA(type, abbr, name, modf, def) type,abbr,name,modf,def

#endif

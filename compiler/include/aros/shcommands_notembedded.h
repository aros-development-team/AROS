/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef SHCOMMANDS_EMBEDDED_H
#define SHCOMMANDS_EMBEDDED_H

#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <aros/asmcall.h>

#define _stringify(x) #x
#define stringify(x) _stringify(x)

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
#    define AMIGANATIVETRICK            \
     asm("\t.text                  \n"  \
         "\tmove.l	4.w,a6     \n"  \
	 "\tjra		__entry(pc) \n");
#else
#    define AMIGANATIVETRICK
#endif

#if SH_GLOBAL_DOSBASE
#    define DECLARE_DOSBase_global extern struct DosLibrary *DOSBase;
#    define DEFINE_DOSBase_global struct DosLibrary *DOSBase;
#    define DEFINE_DOSBase_local
#else
#    define DECLARE_DOSBase_global
#    define DEFINE_DOSBase_local struct DosLibrary *DOSBase;
#    define DEFINE_DOSBase_global
#endif

#define CALL_main(name) name##_main(__shargs, argstr, SysBase, DOSBase)
#define DECLARE_main(name)                                    \
    static ULONG name##_main(IPTR *__shargs, char * __argstr, \
                             struct ExecBase *SysBase,        \
			     struct DosLibrary *DOSBase)
#define DEFINE_main(name) DECLARE_main(name)

#define SHArg(name) (*(SHA_##name##_type *)&__shargs[SHA_##name])
#define SHArgLine() __argstr

#define __SHA_ENUM(type, abbr, name, modf, def, help) SHA_##name
#define __SHA_DEF(type, abbr, name, modf, def, help) (IPTR)(def)
#define __SHA_OPT(type, abbr, name, modf, def, help) \
     stringify(abbr) stringify(name) stringify(modf)
#define __SHA_TYPEDEF(type, abbr, name, modf, def, help) \
     typedef type SHA_##name##_type


#define __AROS_SH_ARGS(name, version, numargs, defl, templ, help) \
AMIGANATIVETRICK                                               \
DECLARE_main(name);                                            \
DECLARE_DOSBase_global                                         \
                                                               \
AROS_UFH3(__unused static LONG, _entry,                        \
    AROS_UFHA(char *,argstr,A0),                               \
    AROS_UFHA(ULONG,argsize,D0),                               \
    AROS_UFHA(struct ExecBase *,SysBase,A6)                    \
)                                                              \
{                                                              \
    AROS_USERFUNC_INIT                                         \
							       \
    DEFINE_DOSBase_local                                       \
                                                               \
    LONG __retcode = RETURN_FAIL;                              \
    IPTR __shargs[numargs] = defl;                             \
    struct RDArgs *__rda  = NULL;                              \
    struct RDArgs *__rda2 = NULL;                              \
							       \
    DOSBase = (struct DosLibrary *) OpenLibrary(DOSNAME, 37);  \
                                                               \
    if (!DOSBase)                                              \
    {                                                          \
        SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);              \
	goto __exit;                                           \
    }                                                          \
							       \
    if (help[0])                                               \
    {                                                          \
        __rda2 = (struct RDArgs *)AllocDosObject(DOS_RDARGS, NULL);             \
	if (!__rda2)                                           \
	{                                                      \
            PrintFault(IoErr(), stringify(name));              \
	    goto __exit;                                       \
	}                                                      \
	__rda2->RDA_ExtHelp = help;                            \
    }                                                          \
							       \
    __rda = ReadArgs(templ, __shargs, __rda2);                 \
					                       \
    if (!__rda)                                                \
    {                                                          \
        PrintFault(IoErr(), stringify(name));                  \
	goto __exit;                                           \
    }                                                          \
							       \
    __retcode = CALL_main(name);                               \
    							       \
__exit:                                                        \
    if (__rda) FreeArgs(__rda);                                \
    if (help[0] && __rda2) FreeDosObject(DOS_RDARGS, __rda2);  \
    if (DOSBase) CloseLibrary((struct Library *)DOSBase);      \
                                                               \
    return __retcode;                                          \
                                                               \
    AROS_USERFUNC_EXIT                                         \
}                                                              \
                                                               \
DEFINE_DOSBase_global                                          \
							       \
__used static const UBYTE name##_version[] = "$VER: "          \
                                 stringify(name) " "           \
		                 stringify(version) " "        \
				 "(" __DATE__ ")\n";           \
                                                               \
DEFINE_main(name)                                              \
{

#define AROS_SHCOMMAND_INIT

#define AROS_SHCOMMAND_EXIT \
    }                       \
}

#define __DEF(x...) {x}

#define __AROS_SH0(name, version, help)             \
    __AROS_SH_ARGS(name, version, 1, {0}, "", help) \
    {                                               \

#define __AROS_SH1(name, version, help, a1)       \
    __AROS_SH_ARGS(name, version, 1,              \
                            __DEF(__SHA_DEF(a1)), \
                            __SHA_OPT(a1),        \
			    help)                 \
    {                                             \
	__SHA_TYPEDEF(a1);                        \
	enum {__SHA_ENUM(a1)};

#define __AROS_SH2(name, version, help, a1, a2)                  \
    __AROS_SH_ARGS(name, version, 2,                             \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2)), \
                            __SHA_OPT(a1) "," __SHA_OPT(a2),     \
                            help)                                \
    {                                                            \
	__SHA_TYPEDEF(a1);                                       \
	__SHA_TYPEDEF(a2);                                       \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)};

#define __AROS_SH3(name, version, help, a1, a2, a3)             \
    __AROS_SH_ARGS(name, version, 3,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3)),                     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
	  	            __SHA_OPT(a3),                      \
			    help)                               \
    {                                                           \
	__SHA_TYPEDEF(a1);                                      \
	__SHA_TYPEDEF(a2);                                      \
	__SHA_TYPEDEF(a3);                                      \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2),                   \
	      __SHA_ENUM(a3)};

#define __AROS_SH4(name, version, help, a1, a2, a3, a4)         \
    __AROS_SH_ARGS(name, version, 4,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3), __SHA_DEF(a4)),      \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
		            __SHA_OPT(a3) "," __SHA_OPT(a4),    \
			    help)                               \
    {                                                           \
	__SHA_TYPEDEF(a1);                                      \
	__SHA_TYPEDEF(a2);                                      \
	__SHA_TYPEDEF(a3);                                      \
	__SHA_TYPEDEF(a4);                                      \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2),                   \
	      __SHA_ENUM(a3), __SHA_ENUM(a4)};

#define __AROS_SH5(name, version, help, a1, a2, a3, a4, a5)     \
    __AROS_SH_ARGS(name, version, 5,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3), __SHA_DEF(a4),       \
		            __SHA_DEF(a5)),                     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
                            __SHA_OPT(a3) "," __SHA_OPT(a4) "," \
		            __SHA_OPT(a5),                      \
			    help)                               \
    {                                                           \
	__SHA_TYPEDEF(a1);                                      \
	__SHA_TYPEDEF(a2);                                      \
	__SHA_TYPEDEF(a3);                                      \
	__SHA_TYPEDEF(a4);                                      \
	__SHA_TYPEDEF(a5);                                      \
	enum {__SHA_ENUM(a1), __SHA_ENUM(a2), __SHA_ENUM(a3),   \
	      __SHA_ENUM(a4), __SHA_ENUM(a5)};

#define __AROS_SH6(name, version, help, a1, a2, a3, a4, a5, a6) \
    __AROS_SH_ARGS(name, version, 6,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3), __SHA_DEF(a4),       \
		            __SHA_DEF(a5), __SHA_DEF(a6)),      \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
                            __SHA_OPT(a3) "," __SHA_OPT(a4) "," \
		            __SHA_OPT(a5) "," __SHA_OPT(a6),    \
			    help)                               \
    {                                                           \
	__SHA_TYPEDEF(a1);                                      \
	__SHA_TYPEDEF(a2);                                      \
	__SHA_TYPEDEF(a3);                                      \
	__SHA_TYPEDEF(a4);                                      \
	__SHA_TYPEDEF(a5);                                      \
	__SHA_TYPEDEF(a6);                                      \
	enum {__SHA_ENUM(a1), __SHA_ENUM(a2), __SHA_ENUM(a3),   \
	      __SHA_ENUM(a4), __SHA_ENUM(a5), __SHA_ENUM(a6)};

#define __AROS_SH7(name, version, help, a1, a2, a3, a4, a5,     \
                                        a6, a7)                 \
    __AROS_SH_ARGS(name, version, 7,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                            __SHA_DEF(a3), __SHA_DEF(a4),       \
		            __SHA_DEF(a5), __SHA_DEF(a6),       \
			    __SHA_DEF(a7)),                     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
                            __SHA_OPT(a3) "," __SHA_OPT(a4) "," \
		            __SHA_OPT(a5) "," __SHA_OPT(a6) "," \
			    __SHA_OPT(a7),                      \
			    help)                               \
    {                                                           \
	__SHA_TYPEDEF(a1);                                      \
	__SHA_TYPEDEF(a2);                                      \
	__SHA_TYPEDEF(a3);                                      \
	__SHA_TYPEDEF(a4);                                      \
	__SHA_TYPEDEF(a5);                                      \
	__SHA_TYPEDEF(a6);                                      \
	__SHA_TYPEDEF(a7);                                      \
	enum {__SHA_ENUM(a1), __SHA_ENUM(a2), __SHA_ENUM(a3),   \
	      __SHA_ENUM(a4), __SHA_ENUM(a5), __SHA_ENUM(a6),   \
	      __SHA_ENUM(a7)};

#define __AROS_SH10(name, version, help, a1, a2, a3, a4, a5,     \
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
			    __SHA_OPT(a9) "," __SHA_OPT(a10),    \
			    help)                                \
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

#define __AROS_SH11(name, version, help, a1, a2, a3, a4, a5,     \
                                 a6, a7, a8, a9, a10,            \
				 a11)                            \
    __AROS_SH_ARGS(name, version, 11,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2),  \
                            __SHA_DEF(a3), __SHA_DEF(a4),        \
		            __SHA_DEF(a5), __SHA_DEF(a6),        \
			    __SHA_DEF(a7), __SHA_DEF(a8),        \
			    __SHA_DEF(a9), __SHA_DEF(a10),       \
			    __SHA_DEF(a11)),                     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) ","  \
                            __SHA_OPT(a3) "," __SHA_OPT(a4) ","  \
		            __SHA_OPT(a5) "," __SHA_OPT(a6) ","  \
			    __SHA_OPT(a7) "," __SHA_OPT(a8) ","  \
			    __SHA_OPT(a9) "," __SHA_OPT(a10) "." \
			    __SHA_OPT(a11), help)                \
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

#define __AROS_SH12(name, version, help, a1, a2, a3, a4, a5,     \
                                 a6, a7, a8, a9, a10,            \
				 a11, a12)                       \
    __AROS_SH_ARGS(name, version, 12,                            \
                            __DEF(__SHA_DEF(a1), __SHA_DEF(a2),  \
                            __SHA_DEF(a3), __SHA_DEF(a4),        \
		            __SHA_DEF(a5), __SHA_DEF(a6),        \
			    __SHA_DEF(a7), __SHA_DEF(a8),        \
			    __SHA_DEF(a9), __SHA_DEF(a10),       \
			    __SHA_DEF(a11), __SHA_DEF(a12)),     \
                            __SHA_OPT(a1) "," __SHA_OPT(a2) ","  \
                            __SHA_OPT(a3) "," __SHA_OPT(a4) ","  \
		            __SHA_OPT(a5) "," __SHA_OPT(a6) ","  \
			    __SHA_OPT(a7) "," __SHA_OPT(a8) ","  \
			    __SHA_OPT(a9) "," __SHA_OPT(a10) "." \
			    __SHA_OPT(a11) "," __SHA_OPT(a12),   \
			    help)                                \
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
	__SHA_TYPEDEF(a12);                                      \
	enum {__SHA_ENUM(a1), __SHA_ENUM(a2), __SHA_ENUM(a3),    \
	      __SHA_ENUM(a4), __SHA_ENUM(a5), __SHA_ENUM(a6),    \
	      __SHA_ENUM(a7), __SHA_ENUM(a8), __SHA_ENUM(a9),    \
	      __SHA_ENUM(a10), __SHA_ENUM(a11), __SHA_ENUM(a12)};

#define AROS_SHA(type, abbr, name, modf, def) type,abbr,name,modf,def,""
#define AROS_SHAH(type, abbr, name, modf, def, help) type,abbr,name,modf,def,"\t" __SHA_OPT(type,abbr,name,modf,def,help) "\t\t" help "\n"

#define __AROS_SHA(type, abbr, name, modf, def, help) type,abbr,name,modf,def,help

#define AROS_SH12(name, version, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) \
        __AROS_SH12(name, version, "", __AROS_SHA(a1), __AROS_SHA(a2), __AROS_SHA(a3), __AROS_SHA(a4), __AROS_SHA(a5), __AROS_SHA(a6), __AROS_SHA(a7), __AROS_SHA(a8), __AROS_SHA(a9), __AROS_SHA(a10), __AROS_SHA(a11), __AROS_SHA(a12))
#define AROS_SH11(name, version, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
        __AROS_SH11(name, version, "", __AROS_SHA(a1), __AROS_SHA(a2), __AROS_SHA(a3), __AROS_SHA(a4), __AROS_SHA(a5), __AROS_SHA(a6), __AROS_SHA(a7), __AROS_SHA(a8), __AROS_SHA(a9), __AROS_SHA(a10), __AROS_SHA(a11))
#define AROS_SH10(name, version, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)\
        __AROS_SH10(name, version, "",  __AROS_SHA(a1), __AROS_SHA(a2), __AROS_SHA(a3), __AROS_SHA(a4), __AROS_SHA(a5), __AROS_SHA(a6), __AROS_SHA(a7), __AROS_SHA(a8), __AROS_SHA(a9), __AROS_SHA(a10))
#define AROS_SH9(name, version, a1, a2, a3, a4, a5, a6, a7, a8, a9) \
        __AROS_SH9(name, version, "",  __AROS_SHA(a1), __AROS_SHA(a2), __AROS_SHA(a3), __AROS_SHA(a4), __AROS_SHA(a5), __AROS_SHA(a6), __AROS_SHA(a7), __AROS_SHA(a8), __AROS_SHA(a9))
#define AROS_SH8(name, version, a1, a2, a3, a4, a5, a6, a7, a8) \
        __AROS_SH8(name, version, "",  __AROS_SHA(a1), __AROS_SHA(a2), __AROS_SHA(a3), __AROS_SHA(a4), __AROS_SHA(a5), __AROS_SHA(a6), __AROS_SHA(a7), __AROS_SHA(a8))
#define AROS_SH7(name, version, a1, a2, a3, a4, a5, a6, a7) \
        __AROS_SH7(name, version, "",  __AROS_SHA(a1), __AROS_SHA(a2), __AROS_SHA(a3), __AROS_SHA(a4), __AROS_SHA(a5), __AROS_SHA(a6), __AROS_SHA(a7))
#define AROS_SH6(name, version, a1, a2, a3, a4, a5, a6) \
        __AROS_SH6(name, version, "",  __AROS_SHA(a1), __AROS_SHA(a2), __AROS_SHA(a3), __AROS_SHA(a4), __AROS_SHA(a5), __AROS_SHA(a6))
#define AROS_SH5(name, version, a1, a2, a3, a4, a5) \
        __AROS_SH5(name, version, "",  __AROS_SHA(a1), __AROS_SHA(a2), __AROS_SHA(a3), __AROS_SHA(a4), __AROS_SHA(a5))
#define AROS_SH4(name, version, a1, a2, a3, a4) \
        __AROS_SH4(name, version, "",  __AROS_SHA(a1), __AROS_SHA(a2), __AROS_SHA(a3), __AROS_SHA(a4))
#define AROS_SH3(name, version, a1, a2, a3) \
        __AROS_SH3(name, version, "",  __AROS_SHA(a1), __AROS_SHA(a2), __AROS_SHA(a3))
#define AROS_SH2(name, version, a1, a2) \
        __AROS_SH2(name, version, "",  __AROS_SHA(a1), __AROS_SHA(a2))
#define AROS_SH1(name, version, a1) \
        __AROS_SH1(name, version, "",  __AROS_SHA(a1))
#define AROS_SH0(name, version) \
        __AROS_SH0(name, version, "")

#define __SH_HELP(name, help) stringify(name) ": " help "\n"
#define __SHA_HELP(type, abbr, name, modf, def, help) help

#define AROS_SH1H(name, version, help, a1) \
        __AROS_SH1(name, version, __SH_HELP(name, help) __SHA_HELP(a1),  __AROS_SHA(a1))
#define AROS_SH0H(name, version, help) \
        __AROS_SH0(name, version, __SH_HELP(name, help))

#endif /* SHCOMMANDS_NOTEMBEDDED_H */

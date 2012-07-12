#ifndef SHCOMMANDS_EMBEDDED_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

/* This defines many of the macros we will be using.
 */
#include <aros/shcommands_notembedded.h>

DECLARESET(SHCOMMANDS)

struct shcommand
{
    STRPTR      sh_Name;
    IPTR        sh_Command;
    ULONG       sh_NumArgs;
    STRPTR      sh_Template;
};

/* We only need to override __AROS_SH_ARGS */
#undef __AROS_SH_ARGS
#define __AROS_SH_ARGS(name, version, numargs, defl, templ, help)       \
static ULONG name##_main(CONST_STRPTR , IPTR *,                \
                         struct ExecBase *SysBase,             \
                         struct DosLibrary *);                 \
AROS_UFP2S(LONG, name##_entry,                                 \
    AROS_UFPA(char *,argstr,A0),                               \
    AROS_UFPA(ULONG,argsize,D0)                                \
);                                                             \
static const struct shcommand __##name##_##shcommand =         \
{                                                              \
    .sh_Name = stringify(name),                                \
    .sh_Command = (IPTR)name##_entry,                          \
    .sh_NumArgs = numargs,                                     \
    .sh_Template = templ                                       \
};                                                             \
                                                               \
ADD2SET(__##name##_##shcommand, SHCOMMANDS, 0);                \
							       \
AROS_UFH2S(LONG, name##_entry,                                 \
    AROS_UFHA(char *,argstr,A0),                               \
    AROS_UFHA(ULONG,argsize,D0)                                \
)                                                              \
{                                                              \
    AROS_USERFUNC_INIT                                         \
    extern struct ExecBase *SysBase;                           \
    APTR DOSBase;                                              \
                                                               \
    LONG __retcode = RETURN_FAIL;                              \
    IPTR __shargs[numargs] = defl;                             \
    struct RDArgs *__rda  = NULL;                              \
    struct RDArgs *__rda2 = NULL;                              \
							       \
    DOSBase = OpenLibrary(DOSNAME, 0);                         \
                                                               \
    if (!DOSBase)                                              \
    {                                                          \
        /* Can't use SetIOErr(), since DOSBase is not open! */ \
        ((struct Process *)FindTask(NULL))->pr_Result2 =       \
                              ERROR_INVALID_RESIDENT_LIBRARY;  \
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
    __retcode = name##_main(argstr, __shargs, SysBase, DOSBase); \
    							       \
__exit:                                                        \
    if (__rda) FreeArgs(__rda);                                \
    if (help[0] && __rda2) FreeDosObject(DOS_RDARGS, __rda2);  \
    if (DOSBase) CloseLibrary(DOSBase);                        \
                                                               \
    return __retcode;                                          \
                                                               \
    AROS_USERFUNC_EXIT                                         \
}                                                              \
static ULONG name##_main(CONST_STRPTR __argstr,                \
                         IPTR *__shargs,                       \
                         struct ExecBase *SysBase,             \
                         struct DosLibrary *DOSBase)           \
{

#endif

#ifndef _AROS_SYMBOLSETS_H
#define _AROS_SYMBOLSETS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Symbol sets support
    Lang: english
*/

#include <exec/types.h>
#include <exec/libraries.h>

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


#define SETNAME(set) __##set##_SET__

#define DECLARESET(set) \
extern void * SETNAME(set)[] __attribute__((weak));

#define DEFINESET(set) \
void * SETNAME(set)[] __attribute__((weak))={0,0};

#define ADD2SET(symbol, set, pri)\
	const int __aros_set_##pri##_##set##_element_##symbol;

#define ADD2INIT(symbol, pri)                                              \
	ADD2SET(symbol, __INIT_SET__, pri)

#define ADD2EXIT(symbol, pri)                                              \
	ADD2SET(symbol, __EXIT_SET__, pri)


/*
  this macro generates the necessary symbols to open and close automatically
  a library. It will make an error message be showed if the library cannot
  be open
*/
#define ADD2LIBS(name, ver, pri, btype, bname, postopenfunc, preclosefunc)   \
btype bname = NULL;                                                                  \
extern int __includelibrarieshandling;                                               \
const int *__setincludelibrarieshandling __attribute__((weak)) = &__includelibrarieshandling; \
const ULONG bname##_version __attribute__((weak)) = ver;                     \
struct libraryset libraryset_##bname =                                       \
{                                                                            \
     name, &bname##_version, &bname, postopenfunc, preclosefunc              \
};                                                                           \
ADD2SET(libraryset_##bname, __LIBS_SET__, pri)

#define ASKFORLIBVERSION(bname, ver) \
const ULONG bname##_version = ver

/* some already allocated priorities for library opening/closing */
#define LIBSET_EXEC_PRI          0
#define LIBSET_DOS_PRI           1
#define LIBSET_INTUITION_PRI     2
#define LIBSET_LAYERS_PRI        3
#define LIBSET_GRAPHICS_PRI      4
#define LIBSET_UTILITY_PRI       5
#define LIBSET_IFFPARSE_PRI      6
#define LIBSET_CYBERGRAPHICS_PRI 7
#define LIBSET_DISKFONT_PRI      8
#define LIBSET_LOCALE_PRI        9
#define LIBSET_ASL_PRI           10
#define LIBSET_GADTOOLS_PRI 	 11
#define LIBSET_REALTIME_PRI 	 12
#define LIBSET_ICON_PRI 	 13
#define LIBSET_WORKBENCH_PRI 	 14
#define LIBSET_DATATYPES_PRI 	 15

#define LIBSET_AROSC_PRI         20

/* User priorities starts from here */
#define LIBSET_USER_PRI      100  /*An enough hight value, I think... */

#endif

#ifndef _AROS_SYMBOLSETS_H
#define _AROS_SYMBOLSETS_H

/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Symbol sets support
    Lang: english
*/

#include <exec/types.h>
#include <exec/libraries.h>

struct libraryset
{
    STRPTR name;
    ULONG  *versionptr;
    void   **baseptr;
    int   (*postopenfunc)(void);
    void  (*preclosefunc)(void);
};

extern int set_call_funcs(void *set[], int order);
extern int set_open_libraries(struct libraryset *set[]);
extern void set_close_libraries(struct libraryset *set[]);


#define SETNAME(set) __##set##_SET__

#define DECLARESET(set) \
extern void * SETNAME(set)[]

#define DEFINESET(set) \
void * SETNAME(set)[] __attribute__((weak))={0,0}

#define ADD2SET(symbol, set, pri)\
	const int __aros_set_##pri##_##set##_element_##symbol;

#define ADD2INIT(symbol, pri)\
	ADD2SET(symbol, __INIT_SET__, pri)

#define ADD2EXIT(symbol, pri)\
	ADD2SET(symbol, __EXIT_SET__, pri)

#define ADD2LIBS(name, ver, pri, btype, bname, postopenfunc, preclosefunc) \
btype bname;                                                               \
const ULONG bname##_version __attribute__((weak)) = ver;                   \
struct libraryset libraryset_##bname =                                     \
{                                                                          \
     name, &bname##_version, &bname, postopenfunc, preclosefunc            \
};                                                                         \
ADD2SET(libraryset_##bname, __LIBS_SET__, pri)

#define ASKFORLIBVERSION(bname, ver) \
const ULONG bname##_version = ver

/* some already allocated priorities for library opening/closing */
#define LIBSET_EXEC_PRI      0
#define LIBSET_DOS_PRI       1
#define LIBSET_INTUITION_PRI 2
#define LIBSET_UTILITY_PRI   5
#define LIBSET_AROSC_PRI     20

/* User priorities starts from here */
#define LIBSET_USER_PRI      100  /*An enough hight value, I think... */

#endif

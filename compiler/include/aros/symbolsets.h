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
    ULONG  version;
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

#define ADDLIB2SET(name, version, btype, bname, postopenfunc, preclosefunc) \
btype bname;                                                                \
struct libraryset libraryset_##bname =                                      \
{                                                                           \
     name, version, &bname, postopenfunc, preclosefunc                      \
};                                                                          \
ADD2SET(libraryset_##bname, __LIBS_SET__, 0)

#endif

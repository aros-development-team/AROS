#ifndef CLIB_AROS_PROTOS_H
#define CLIB_AROS_PROTOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for aros.library
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP1(ULONG, ArosInquireA,
    AROS_LPA(struct TagItem *, taglist, A0),
    struct ArosBase *, ArosBase, 5, Aros)


#endif /* CLIB_AROS_PROTOS_H */

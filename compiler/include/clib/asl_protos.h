#ifndef CLIB_ASL_PROTOS_H
#define CLIB_ASL_PROTOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for asl.library
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
AROS_LP2(APTR, AllocAslRequest,
    AROS_LPA(ULONG           , reqType, D0),
    AROS_LPA(struct TagItem *, tagList, A0),
    struct Library *, AslBase, 8, Asl)

AROS_LP0(struct FileRequester *, AllocFileRequest,
    struct Library *, AslBase, 5, Asl)

AROS_LP2(BOOL, AslRequest,
    AROS_LPA(APTR            , requester, A0),
    AROS_LPA(struct TagItem *, tagList, A1),
    struct Library *, AslBase, 10, Asl)

AROS_LP1(void, FreeAslRequest,
    AROS_LPA(APTR, requester, A0),
    struct Library *, AslBase, 9, Asl)

AROS_LP1(void, FreeFileRequest,
    AROS_LPA(struct FileRequester *, fileReq, A0),
    struct Library *, AslBase, 6, Asl)

AROS_LP1(BOOL, RequestFile,
    AROS_LPA(struct FileRequester *, fileReq, A0),
    struct Library *, AslBase, 7, Asl)


#endif /*  */

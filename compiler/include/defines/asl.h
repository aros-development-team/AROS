#ifndef DEFINES_ASL_H
#define DEFINES_ASL_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define AllocAslRequest(reqType, tagList) \
    AROS_LC2(APTR, AllocAslRequest, \
    AROS_LCA(ULONG           , reqType, D0), \
    AROS_LCA(struct TagItem *, tagList, A0), \
    struct Library *, AslBase, 8, Asl)

#define AllocFileRequest() \
    AROS_LC0(struct FileRequester *, AllocFileRequest, \
    struct Library *, AslBase, 5, Asl)

#define AslRequest(requester, tagList) \
    AROS_LC2(BOOL, AslRequest, \
    AROS_LCA(APTR            , requester, A0), \
    AROS_LCA(struct TagItem *, tagList, A1), \
    struct Library *, AslBase, 10, Asl)

#define FreeAslRequest(requester) \
    AROS_LC1(void, FreeAslRequest, \
    AROS_LCA(APTR, requester, A0), \
    struct Library *, AslBase, 9, Asl)

#define FreeFileRequest(fileReq) \
    AROS_LC1(void, FreeFileRequest, \
    AROS_LCA(struct FileRequester *, fileReq, A0), \
    struct Library *, AslBase, 6, Asl)

#define RequestFile(fileReq) \
    AROS_LC1(BOOL, RequestFile, \
    AROS_LCA(struct FileRequester *, fileReq, A0), \
    struct Library *, AslBase, 7, Asl)


#endif /* DEFINES_ASL_H */

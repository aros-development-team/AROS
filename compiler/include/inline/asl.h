#ifndef _INLINE_ASL_H
#define _INLINE_ASL_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef ASL_BASE_NAME
#define ASL_BASE_NAME AslBase
#endif

#define AllocAslRequest(reqType, tagList) \
	LP2(0x30, APTR, AllocAslRequest, unsigned long, reqType, d0, struct TagItem *, tagList, a0, \
	, ASL_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AllocAslRequestTags(a0, tags...) \
	({ULONG _tags[] = { tags }; AllocAslRequest((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define AllocFileRequest() \
	LP0(0x1e, struct FileRequester *, AllocFileRequest, \
	, ASL_BASE_NAME)

#define AslRequest(requester, tagList) \
	LP2(0x3c, BOOL, AslRequest, APTR, requester, a0, struct TagItem *, tagList, a1, \
	, ASL_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AslRequestTags(a0, tags...) \
	({ULONG _tags[] = { tags }; AslRequest((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define FreeAslRequest(requester) \
	LP1NR(0x36, FreeAslRequest, APTR, requester, a0, \
	, ASL_BASE_NAME)

#define FreeFileRequest(fileReq) \
	LP1NR(0x24, FreeFileRequest, struct FileRequester *, fileReq, a0, \
	, ASL_BASE_NAME)

#define RequestFile(fileReq) \
	LP1(0x2a, BOOL, RequestFile, struct FileRequester *, fileReq, a0, \
	, ASL_BASE_NAME)

#endif /* _INLINE_ASL_H */

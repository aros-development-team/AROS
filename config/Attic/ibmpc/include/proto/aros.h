#ifndef PROTO_AROS_H
#define PROTO_AROS_H

#ifndef INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef AROS_BASE_NAME
#define AROS_BASE_NAME ArosBase
#endif

#define ArosInquireA(taglist) \
	LP1(0x14, ULONG, ArosInquireA, struct TagItem *, taglist, \
	, AROS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define ArosInquire(tags...) \
	({ULONG _tags[] = { tags }; ArosInquireA((struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#endif /* PROTO_AROS_H */

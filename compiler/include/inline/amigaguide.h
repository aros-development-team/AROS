#ifndef _INLINE_AMIGAGUIDE_H
#define _INLINE_AMIGAGUIDE_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef AMIGAGUIDE_BASE_NAME
#define AMIGAGUIDE_BASE_NAME AmigaGuideBase
#endif

#define AddAmigaGuideHostA(h, name, attrs) \
	LP3(0x8a, APTR, AddAmigaGuideHostA, struct Hook *, h, a0, STRPTR, name, d0, struct TagItem *, attrs, a1, \
	, AMIGAGUIDE_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AddAmigaGuideHost(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; AddAmigaGuideHostA((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define AmigaGuideSignal(cl) \
	LP1(0x48, ULONG, AmigaGuideSignal, APTR, cl, a0, \
	, AMIGAGUIDE_BASE_NAME)

#define CloseAmigaGuide(cl) \
	LP1NR(0x42, CloseAmigaGuide, APTR, cl, a0, \
	, AMIGAGUIDE_BASE_NAME)

#define ExpungeXRef() \
	LP0NR(0x84, ExpungeXRef, \
	, AMIGAGUIDE_BASE_NAME)

#define GetAmigaGuideAttr(tag, cl, storage) \
	LP3(0x72, LONG, GetAmigaGuideAttr, Tag, tag, d0, APTR, cl, a0, ULONG *, storage, a1, \
	, AMIGAGUIDE_BASE_NAME)

#define GetAmigaGuideMsg(cl) \
	LP1(0x4e, struct AmigaGuideMsg *, GetAmigaGuideMsg, APTR, cl, a0, \
	, AMIGAGUIDE_BASE_NAME)

#define GetAmigaGuideString(id) \
	LP1(0xd2, STRPTR, GetAmigaGuideString, long, id, d0, \
	, AMIGAGUIDE_BASE_NAME)

#define LoadXRef(lock, name) \
	LP2(0x7e, LONG, LoadXRef, BPTR, lock, a0, STRPTR, name, a1, \
	, AMIGAGUIDE_BASE_NAME)

#define LockAmigaGuideBase(handle) \
	LP1(0x24, LONG, LockAmigaGuideBase, APTR, handle, a0, \
	, AMIGAGUIDE_BASE_NAME)

#define OpenAmigaGuideA(nag, attrs) \
	LP2(0x36, APTR, OpenAmigaGuideA, struct NewAmigaGuide *, nag, a0, struct TagItem *, attrs, a1, \
	, AMIGAGUIDE_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define OpenAmigaGuide(a0, tags...) \
	({ULONG _tags[] = { tags }; OpenAmigaGuideA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define OpenAmigaGuideAsyncA(nag, attrs) \
	LP2(0x3c, APTR, OpenAmigaGuideAsyncA, struct NewAmigaGuide *, nag, a0, struct TagItem *, attrs, d0, \
	, AMIGAGUIDE_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define OpenAmigaGuideAsync(a0, tags...) \
	({ULONG _tags[] = { tags }; OpenAmigaGuideAsyncA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define RemoveAmigaGuideHostA(hh, attrs) \
	LP2(0x90, LONG, RemoveAmigaGuideHostA, APTR, hh, a0, struct TagItem *, attrs, a1, \
	, AMIGAGUIDE_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define RemoveAmigaGuideHost(a0, tags...) \
	({ULONG _tags[] = { tags }; RemoveAmigaGuideHostA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define ReplyAmigaGuideMsg(amsg) \
	LP1NR(0x54, ReplyAmigaGuideMsg, struct AmigaGuideMsg *, amsg, a0, \
	, AMIGAGUIDE_BASE_NAME)

#define SendAmigaGuideCmdA(cl, cmd, attrs) \
	LP3(0x66, LONG, SendAmigaGuideCmdA, APTR, cl, a0, STRPTR, cmd, d0, struct TagItem *, attrs, d1, \
	, AMIGAGUIDE_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SendAmigaGuideCmd(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; SendAmigaGuideCmdA((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define SendAmigaGuideContextA(cl, attrs) \
	LP2(0x60, LONG, SendAmigaGuideContextA, APTR, cl, a0, struct TagItem *, attrs, d0, \
	, AMIGAGUIDE_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SendAmigaGuideContext(a0, tags...) \
	({ULONG _tags[] = { tags }; SendAmigaGuideContextA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define SetAmigaGuideAttrsA(cl, attrs) \
	LP2(0x6c, LONG, SetAmigaGuideAttrsA, APTR, cl, a0, struct TagItem *, attrs, a1, \
	, AMIGAGUIDE_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SetAmigaGuideAttrs(a0, tags...) \
	({ULONG _tags[] = { tags }; SetAmigaGuideAttrsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define SetAmigaGuideContextA(cl, id, attrs) \
	LP3(0x5a, LONG, SetAmigaGuideContextA, APTR, cl, a0, unsigned long, id, d0, struct TagItem *, attrs, d1, \
	, AMIGAGUIDE_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SetAmigaGuideContext(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; SetAmigaGuideContextA((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define UnlockAmigaGuideBase(key) \
	LP1NR(0x2a, UnlockAmigaGuideBase, long, key, d0, \
	, AMIGAGUIDE_BASE_NAME)

#endif /* _INLINE_AMIGAGUIDE_H */

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_BULLET_H
#define _INLINE_BULLET_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef BULLET_BASE_NAME
#define BULLET_BASE_NAME BulletBase
#endif

#define CloseEngine(glyphEngine) \
	LP1NR(0x24, CloseEngine, struct GlyphEngine *, glyphEngine, a0, \
	, BULLET_BASE_NAME)

#define ObtainInfoA(glyphEngine, tagList) \
	LP2(0x30, ULONG, ObtainInfoA, struct GlyphEngine *, glyphEngine, a0, struct TagItem *, tagList, a1, \
	, BULLET_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define ObtainInfo(a0, tags...) \
	({ULONG _tags[] = { tags }; ObtainInfoA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define OpenEngine() \
	LP0(0x1e, struct GlyphEngine *, OpenEngine, \
	, BULLET_BASE_NAME)

#define ReleaseInfoA(glyphEngine, tagList) \
	LP2(0x36, ULONG, ReleaseInfoA, struct GlyphEngine *, glyphEngine, a0, struct TagItem *, tagList, a1, \
	, BULLET_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define ReleaseInfo(a0, tags...) \
	({ULONG _tags[] = { tags }; ReleaseInfoA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define SetInfoA(glyphEngine, tagList) \
	LP2(0x2a, ULONG, SetInfoA, struct GlyphEngine *, glyphEngine, a0, struct TagItem *, tagList, a1, \
	, BULLET_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SetInfo(a0, tags...) \
	({ULONG _tags[] = { tags }; SetInfoA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#endif /* _INLINE_BULLET_H */

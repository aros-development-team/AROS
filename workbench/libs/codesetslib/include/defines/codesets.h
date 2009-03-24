/* Automatically generated header! Do not edit! */

#ifndef _INLINE_CODESETS_H
#define _INLINE_CODESETS_H

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef CODESETS_BASE_NAME
#define CODESETS_BASE_NAME CodesetsBase
#endif /* !CODESETS_BASE_NAME */

#define CodesetsConvertUTF32toUTF16(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) __CodesetsConvertUTF32toUTF16_WB(CODESETS_BASE_NAME, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags)
#define __CodesetsConvertUTF32toUTF16_WB(___base, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF32toUTF16, \
	AROS_LCA(const UTF32 **, (___sourceStart), A0), \
	AROS_LCA(const UTF32 *, (___sourceEnd), A1), \
	AROS_LCA(UTF16 **, (___targetStart), A2), \
	AROS_LCA(UTF16 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, (___base), 6, Codesets)

#define CodesetsConvertUTF16toUTF32(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) __CodesetsConvertUTF16toUTF32_WB(CODESETS_BASE_NAME, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags)
#define __CodesetsConvertUTF16toUTF32_WB(___base, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF16toUTF32, \
	AROS_LCA(const  UTF16 **, (___sourceStart), A0), \
	AROS_LCA(const UTF16 *, (___sourceEnd), A1), \
	AROS_LCA(UTF32 **, (___targetStart), A2), \
	AROS_LCA(UTF32 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, (___base), 7, Codesets)

#define CodesetsConvertUTF16toUTF8(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) __CodesetsConvertUTF16toUTF8_WB(CODESETS_BASE_NAME, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags)
#define __CodesetsConvertUTF16toUTF8_WB(___base, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF16toUTF8, \
	AROS_LCA(const UTF16 **, (___sourceStart), A0), \
	AROS_LCA(const UTF16 *, (___sourceEnd), A1), \
	AROS_LCA(UTF8 **, (___targetStart), A2), \
	AROS_LCA(UTF8 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, (___base), 8, Codesets)

#define CodesetsIsLegalUTF8(___source, ___length) __CodesetsIsLegalUTF8_WB(CODESETS_BASE_NAME, ___source, ___length)
#define __CodesetsIsLegalUTF8_WB(___base, ___source, ___length) \
	AROS_LC2(BOOL, CodesetsIsLegalUTF8, \
	AROS_LCA(const UTF8 *, (___source), A0), \
	AROS_LCA(ULONG, (___length), D0), \
	struct Library *, (___base), 9, Codesets)

#define CodesetsIsLegalUTF8Sequence(___source, ___sourceEnd) __CodesetsIsLegalUTF8Sequence_WB(CODESETS_BASE_NAME, ___source, ___sourceEnd)
#define __CodesetsIsLegalUTF8Sequence_WB(___base, ___source, ___sourceEnd) \
	AROS_LC2(BOOL, CodesetsIsLegalUTF8Sequence, \
	AROS_LCA(const UTF8 *, (___source), A0), \
	AROS_LCA(const UTF8 *, (___sourceEnd), A1), \
	struct Library *, (___base), 10, Codesets)

#define CodesetsConvertUTF8toUTF16(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) __CodesetsConvertUTF8toUTF16_WB(CODESETS_BASE_NAME, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags)
#define __CodesetsConvertUTF8toUTF16_WB(___base, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF8toUTF16, \
	AROS_LCA(const UTF8 **, (___sourceStart), A0), \
	AROS_LCA(const UTF8 *, (___sourceEnd), A1), \
	AROS_LCA(UTF16 **, (___targetStart), A2), \
	AROS_LCA(UTF16 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, (___base), 11, Codesets)

#define CodesetsConvertUTF32toUTF8(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) __CodesetsConvertUTF32toUTF8_WB(CODESETS_BASE_NAME, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags)
#define __CodesetsConvertUTF32toUTF8_WB(___base, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF32toUTF8, \
	AROS_LCA(const UTF32 **, (___sourceStart), A0), \
	AROS_LCA(const UTF32 *, (___sourceEnd), A1), \
	AROS_LCA(UTF8 **, (___targetStart), A2), \
	AROS_LCA(UTF8 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, (___base), 12, Codesets)

#define CodesetsConvertUTF8toUTF32(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) __CodesetsConvertUTF8toUTF32_WB(CODESETS_BASE_NAME, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags)
#define __CodesetsConvertUTF8toUTF32_WB(___base, ___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF8toUTF32, \
	AROS_LCA(const UTF8 **, (___sourceStart), A0), \
	AROS_LCA(const UTF8 *, (___sourceEnd), A1), \
	AROS_LCA(UTF32 **, (___targetStart), A2), \
	AROS_LCA(UTF32 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, (___base), 13, Codesets)

#define CodesetsSetDefaultA(___name, ___attrs) __CodesetsSetDefaultA_WB(CODESETS_BASE_NAME, ___name, ___attrs)
#define __CodesetsSetDefaultA_WB(___base, ___name, ___attrs) \
	AROS_LC2(struct codeset *, CodesetsSetDefaultA, \
	AROS_LCA(STRPTR, (___name), A0), \
	AROS_LCA(struct TagItem *, (___attrs), A1), \
	struct Library *, (___base), 14, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsSetDefault(___name, ___attrs, ...) __CodesetsSetDefault_WB(CODESETS_BASE_NAME, ___name, ___attrs, ## __VA_ARGS__)
#define __CodesetsSetDefault_WB(___base, ___name, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsSetDefaultA_WB((___base), (___name), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsFreeA(___obj, ___attrs) __CodesetsFreeA_WB(CODESETS_BASE_NAME, ___obj, ___attrs)
#define __CodesetsFreeA_WB(___base, ___obj, ___attrs) \
	AROS_LC2NR(void, CodesetsFreeA, \
	AROS_LCA(APTR, (___obj), A0), \
	AROS_LCA(struct TagItem *, (___attrs), A1), \
	struct Library *, (___base), 15, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsFree(___obj, ___attrs, ...) __CodesetsFree_WB(CODESETS_BASE_NAME, ___obj, ___attrs, ## __VA_ARGS__)
#define __CodesetsFree_WB(___base, ___obj, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsFreeA_WB((___base), (___obj), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsSupportedA(___attrs) __CodesetsSupportedA_WB(CODESETS_BASE_NAME, ___attrs)
#define __CodesetsSupportedA_WB(___base, ___attrs) \
	AROS_LC1(STRPTR *, CodesetsSupportedA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, (___base), 16, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsSupported(___attrs, ...) __CodesetsSupported_WB(CODESETS_BASE_NAME, ___attrs, ## __VA_ARGS__)
#define __CodesetsSupported_WB(___base, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsSupportedA_WB((___base), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsFindA(___name, ___attrs) __CodesetsFindA_WB(CODESETS_BASE_NAME, ___name, ___attrs)
#define __CodesetsFindA_WB(___base, ___name, ___attrs) \
	AROS_LC2(struct codeset *, CodesetsFindA, \
	AROS_LCA(STRPTR, (___name), A0), \
	AROS_LCA(struct TagItem *, (___attrs), A1), \
	struct Library *, (___base), 17, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsFind(___name, ___attrs, ...) __CodesetsFind_WB(CODESETS_BASE_NAME, ___name, ___attrs, ## __VA_ARGS__)
#define __CodesetsFind_WB(___base, ___name, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsFindA_WB((___base), (___name), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsFindBestA(___attrs) __CodesetsFindBestA_WB(CODESETS_BASE_NAME, ___attrs)
#define __CodesetsFindBestA_WB(___base, ___attrs) \
	AROS_LC1(struct codeset *, CodesetsFindBestA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, (___base), 18, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsFindBest(___attrs, ...) __CodesetsFindBest_WB(CODESETS_BASE_NAME, ___attrs, ## __VA_ARGS__)
#define __CodesetsFindBest_WB(___base, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsFindBestA_WB((___base), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsUTF8Len(___str) __CodesetsUTF8Len_WB(CODESETS_BASE_NAME, ___str)
#define __CodesetsUTF8Len_WB(___base, ___str) \
	AROS_LC1(ULONG, CodesetsUTF8Len, \
	AROS_LCA(const UTF8 *, (___str), A0), \
	struct Library *, (___base), 19, Codesets)

#define CodesetsUTF8ToStrA(___attrs) __CodesetsUTF8ToStrA_WB(CODESETS_BASE_NAME, ___attrs)
#define __CodesetsUTF8ToStrA_WB(___base, ___attrs) \
	AROS_LC1(STRPTR, CodesetsUTF8ToStrA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, (___base), 20, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsUTF8ToStr(___attrs, ...) __CodesetsUTF8ToStr_WB(CODESETS_BASE_NAME, ___attrs, ## __VA_ARGS__)
#define __CodesetsUTF8ToStr_WB(___base, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsUTF8ToStrA_WB((___base), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsUTF8CreateA(___attrs) __CodesetsUTF8CreateA_WB(CODESETS_BASE_NAME, ___attrs)
#define __CodesetsUTF8CreateA_WB(___base, ___attrs) \
	AROS_LC1(UTF8 *, CodesetsUTF8CreateA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, (___base), 21, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsUTF8Create(___attrs, ...) __CodesetsUTF8Create_WB(CODESETS_BASE_NAME, ___attrs, ## __VA_ARGS__)
#define __CodesetsUTF8Create_WB(___base, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsUTF8CreateA_WB((___base), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsEncodeB64A(___attrs) __CodesetsEncodeB64A_WB(CODESETS_BASE_NAME, ___attrs)
#define __CodesetsEncodeB64A_WB(___base, ___attrs) \
	AROS_LC1(ULONG, CodesetsEncodeB64A, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, (___base), 22, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsEncodeB64(___attrs, ...) __CodesetsEncodeB64_WB(CODESETS_BASE_NAME, ___attrs, ## __VA_ARGS__)
#define __CodesetsEncodeB64_WB(___base, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsEncodeB64A_WB((___base), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsDecodeB64A(___attrs) __CodesetsDecodeB64A_WB(CODESETS_BASE_NAME, ___attrs)
#define __CodesetsDecodeB64A_WB(___base, ___attrs) \
	AROS_LC1(ULONG, CodesetsDecodeB64A, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, (___base), 23, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsDecodeB64(___attrs, ...) __CodesetsDecodeB64_WB(CODESETS_BASE_NAME, ___attrs, ## __VA_ARGS__)
#define __CodesetsDecodeB64_WB(___base, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsDecodeB64A_WB((___base), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsStrLenA(___str, ___attrs) __CodesetsStrLenA_WB(CODESETS_BASE_NAME, ___str, ___attrs)
#define __CodesetsStrLenA_WB(___base, ___str, ___attrs) \
	AROS_LC2(ULONG, CodesetsStrLenA, \
	AROS_LCA(STRPTR, (___str), A0), \
	AROS_LCA(struct TagItem *, (___attrs), A1), \
	struct Library *, (___base), 24, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsStrLen(___str, ___attrs, ...) __CodesetsStrLen_WB(CODESETS_BASE_NAME, ___str, ___attrs, ## __VA_ARGS__)
#define __CodesetsStrLen_WB(___base, ___str, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsStrLenA_WB((___base), (___str), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsIsValidUTF8(___str) __CodesetsIsValidUTF8_WB(CODESETS_BASE_NAME, ___str)
#define __CodesetsIsValidUTF8_WB(___base, ___str) \
	AROS_LC1(BOOL, CodesetsIsValidUTF8, \
	AROS_LCA(STRPTR, (___str), A0), \
	struct Library *, (___base), 25, Codesets)

#define CodesetsFreeVecPooledA(___pool, ___mem, ___attrs) __CodesetsFreeVecPooledA_WB(CODESETS_BASE_NAME, ___pool, ___mem, ___attrs)
#define __CodesetsFreeVecPooledA_WB(___base, ___pool, ___mem, ___attrs) \
	AROS_LC3NR(void, CodesetsFreeVecPooledA, \
	AROS_LCA(APTR, (___pool), A0), \
	AROS_LCA(APTR, (___mem), A1), \
	AROS_LCA(struct TagItem *, (___attrs), A2), \
	struct Library *, (___base), 26, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsFreeVecPooled(___pool, ___mem, ___attrs, ...) __CodesetsFreeVecPooled_WB(CODESETS_BASE_NAME, ___pool, ___mem, ___attrs, ## __VA_ARGS__)
#define __CodesetsFreeVecPooled_WB(___base, ___pool, ___mem, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsFreeVecPooledA_WB((___base), (___pool), (___mem), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsConvertStrA(___attrs) __CodesetsConvertStrA_WB(CODESETS_BASE_NAME, ___attrs)
#define __CodesetsConvertStrA_WB(___base, ___attrs) \
	AROS_LC1(STRPTR, CodesetsConvertStrA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, (___base), 27, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsConvertStr(___attrs, ...) __CodesetsConvertStr_WB(CODESETS_BASE_NAME, ___attrs, ## __VA_ARGS__)
#define __CodesetsConvertStr_WB(___base, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsConvertStrA_WB((___base), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsListCreateA(___attrs) __CodesetsListCreateA_WB(CODESETS_BASE_NAME, ___attrs)
#define __CodesetsListCreateA_WB(___base, ___attrs) \
	AROS_LC1(struct codesetList *, CodesetsListCreateA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, (___base), 28, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsListCreate(___attrs, ...) __CodesetsListCreate_WB(CODESETS_BASE_NAME, ___attrs, ## __VA_ARGS__)
#define __CodesetsListCreate_WB(___base, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsListCreateA_WB((___base), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsListDeleteA(___attrs) __CodesetsListDeleteA_WB(CODESETS_BASE_NAME, ___attrs)
#define __CodesetsListDeleteA_WB(___base, ___attrs) \
	AROS_LC1(BOOL, CodesetsListDeleteA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, (___base), 29, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsListDelete(___tag1, ...) __CodesetsListDelete_WB(CODESETS_BASE_NAME, ___tag1, ## __VA_ARGS__)
#define __CodesetsListDelete_WB(___base, ___tag1, ...) \
	({IPTR _tags[] = { (IPTR) ___tag1, ## __VA_ARGS__ }; __CodesetsListDeleteA_WB((___base), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsListAddA(___codesetsList, ___attrs) __CodesetsListAddA_WB(CODESETS_BASE_NAME, ___codesetsList, ___attrs)
#define __CodesetsListAddA_WB(___base, ___codesetsList, ___attrs) \
	AROS_LC2(BOOL, CodesetsListAddA, \
	AROS_LCA(struct codesetList *, (___codesetsList), A0), \
	AROS_LCA(struct TagItem *, (___attrs), A1), \
	struct Library *, (___base), 30, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsListAdd(___codesetsList, ___attrs, ...) __CodesetsListAdd_WB(CODESETS_BASE_NAME, ___codesetsList, ___attrs, ## __VA_ARGS__)
#define __CodesetsListAdd_WB(___base, ___codesetsList, ___attrs, ...) \
	({IPTR _tags[] = { (IPTR) ___attrs, ## __VA_ARGS__ }; __CodesetsListAddA_WB((___base), (___codesetsList), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsListRemoveA(___attrs) __CodesetsListRemoveA_WB(CODESETS_BASE_NAME, ___attrs)
#define __CodesetsListRemoveA_WB(___base, ___attrs) \
	AROS_LC1(BOOL, CodesetsListRemoveA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, (___base), 31, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsListRemove(___tag1, ...) __CodesetsListRemove_WB(CODESETS_BASE_NAME, ___tag1, ## __VA_ARGS__)
#define __CodesetsListRemove_WB(___base, ___tag1, ...) \
	({IPTR _tags[] = { (IPTR) ___tag1, ## __VA_ARGS__ }; __CodesetsListRemoveA_WB((___base), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#endif /* !_INLINE_CODESETS_H */

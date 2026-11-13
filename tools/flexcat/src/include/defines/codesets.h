/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_CODESETS_H
#define _INLINE_CODESETS_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef CODESETS_BASE_NAME
#define CODESETS_BASE_NAME CodesetsBase
#endif /* !CODESETS_BASE_NAME */

#define CodesetsConvertUTF32toUTF16(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF32toUTF16, \
	AROS_LCA(const UTF32 **, (___sourceStart), A0), \
	AROS_LCA(const UTF32 *, (___sourceEnd), A1), \
	AROS_LCA(UTF16 **, (___targetStart), A2), \
	AROS_LCA(UTF16 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, CODESETS_BASE_NAME, 6, Codesets)

#define CodesetsConvertUTF16toUTF32(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF16toUTF32, \
	AROS_LCA(const  UTF16 **, (___sourceStart), A0), \
	AROS_LCA(const UTF16 *, (___sourceEnd), A1), \
	AROS_LCA(UTF32 **, (___targetStart), A2), \
	AROS_LCA(UTF32 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, CODESETS_BASE_NAME, 7, Codesets)

#define CodesetsConvertUTF16toUTF8(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF16toUTF8, \
	AROS_LCA(const UTF16 **, (___sourceStart), A0), \
	AROS_LCA(const UTF16 *, (___sourceEnd), A1), \
	AROS_LCA(UTF8 **, (___targetStart), A2), \
	AROS_LCA(UTF8 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, CODESETS_BASE_NAME, 8, Codesets)

#define CodesetsIsLegalUTF8(___source, ___length) \
	AROS_LC2(BOOL, CodesetsIsLegalUTF8, \
	AROS_LCA(const UTF8 *, (___source), A0), \
	AROS_LCA(ULONG, (___length), D0), \
	struct Library *, CODESETS_BASE_NAME, 9, Codesets)

#define CodesetsIsLegalUTF8Sequence(___source, ___sourceEnd) \
	AROS_LC2(BOOL, CodesetsIsLegalUTF8Sequence, \
	AROS_LCA(const UTF8 *, (___source), A0), \
	AROS_LCA(const UTF8 *, (___sourceEnd), A1), \
	struct Library *, CODESETS_BASE_NAME, 10, Codesets)

#define CodesetsConvertUTF8toUTF16(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF8toUTF16, \
	AROS_LCA(const UTF8 **, (___sourceStart), A0), \
	AROS_LCA(const UTF8 *, (___sourceEnd), A1), \
	AROS_LCA(UTF16 **, (___targetStart), A2), \
	AROS_LCA(UTF16 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, CODESETS_BASE_NAME, 11, Codesets)

#define CodesetsConvertUTF32toUTF8(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF32toUTF8, \
	AROS_LCA(const UTF32 **, (___sourceStart), A0), \
	AROS_LCA(const UTF32 *, (___sourceEnd), A1), \
	AROS_LCA(UTF8 **, (___targetStart), A2), \
	AROS_LCA(UTF8 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, CODESETS_BASE_NAME, 12, Codesets)

#define CodesetsConvertUTF8toUTF32(___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags) \
	AROS_LC5(ULONG, CodesetsConvertUTF8toUTF32, \
	AROS_LCA(const UTF8 **, (___sourceStart), A0), \
	AROS_LCA(const UTF8 *, (___sourceEnd), A1), \
	AROS_LCA(UTF32 **, (___targetStart), A2), \
	AROS_LCA(UTF32 *, (___targetEnd), A3), \
	AROS_LCA(ULONG, (___flags), D0), \
	struct Library *, CODESETS_BASE_NAME, 13, Codesets)

#define CodesetsSetDefaultA(___name, ___attrs) \
	AROS_LC2(struct codeset *, CodesetsSetDefaultA, \
	AROS_LCA(STRPTR, (___name), A0), \
	AROS_LCA(struct TagItem *, (___attrs), A1), \
	struct Library *, CODESETS_BASE_NAME, 14, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsSetDefault(___name, ___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsSetDefaultA((___name), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsFreeA(___obj, ___attrs) \
	AROS_LC2NR(void, CodesetsFreeA, \
	AROS_LCA(APTR, (___obj), A0), \
	AROS_LCA(struct TagItem *, (___attrs), A1), \
	struct Library *, CODESETS_BASE_NAME, 15, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsFree(___obj, ___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsFreeA((___obj), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsSupportedA(___attrs) \
	AROS_LC1(STRPTR *, CodesetsSupportedA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, CODESETS_BASE_NAME, 16, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsSupported(___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsSupportedA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsFindA(___name, ___attrs) \
	AROS_LC2(struct codeset *, CodesetsFindA, \
	AROS_LCA(STRPTR, (___name), A0), \
	AROS_LCA(struct TagItem *, (___attrs), A1), \
	struct Library *, CODESETS_BASE_NAME, 17, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsFind(___name, ___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsFindA((___name), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsFindBestA(___attrs) \
	AROS_LC1(struct codeset *, CodesetsFindBestA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, CODESETS_BASE_NAME, 18, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsFindBest(___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsFindBestA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsUTF8Len(___str) \
	AROS_LC1(ULONG, CodesetsUTF8Len, \
	AROS_LCA(const UTF8 *, (___str), A0), \
	struct Library *, CODESETS_BASE_NAME, 19, Codesets)

#define CodesetsUTF8ToStrA(___attrs) \
	AROS_LC1(STRPTR, CodesetsUTF8ToStrA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, CODESETS_BASE_NAME, 20, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsUTF8ToStr(___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsUTF8ToStrA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsUTF8CreateA(___attrs) \
	AROS_LC1(UTF8 *, CodesetsUTF8CreateA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, CODESETS_BASE_NAME, 21, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsUTF8Create(___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsUTF8CreateA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsEncodeB64A(___attrs) \
	AROS_LC1(ULONG, CodesetsEncodeB64A, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, CODESETS_BASE_NAME, 22, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsEncodeB64(___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsEncodeB64A((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsDecodeB64A(___attrs) \
	AROS_LC1(ULONG, CodesetsDecodeB64A, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, CODESETS_BASE_NAME, 23, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsDecodeB64(___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsDecodeB64A((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsStrLenA(___str, ___attrs) \
	AROS_LC2(ULONG, CodesetsStrLenA, \
	AROS_LCA(STRPTR, (___str), A0), \
	AROS_LCA(struct TagItem *, (___attrs), A1), \
	struct Library *, CODESETS_BASE_NAME, 24, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsStrLen(___str, ___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsStrLenA((___str), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsIsValidUTF8(___str) \
	AROS_LC1(BOOL, CodesetsIsValidUTF8, \
	AROS_LCA(STRPTR, (___str), A0), \
	struct Library *, CODESETS_BASE_NAME, 25, Codesets)

#define CodesetsFreeVecPooledA(___pool, ___mem, ___attrs) \
	AROS_LC3NR(void, CodesetsFreeVecPooledA, \
	AROS_LCA(APTR, (___pool), A0), \
	AROS_LCA(APTR, (___mem), A1), \
	AROS_LCA(struct TagItem *, (___attrs), A2), \
	struct Library *, CODESETS_BASE_NAME, 26, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsFreeVecPooled(___pool, ___mem, ___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsFreeVecPooledA((___pool), (___mem), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsConvertStrA(___attrs) \
	AROS_LC1(STRPTR, CodesetsConvertStrA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, CODESETS_BASE_NAME, 27, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsConvertStr(___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsConvertStrA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsListCreateA(___attrs) \
	AROS_LC1(struct codesetList *, CodesetsListCreateA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, CODESETS_BASE_NAME, 28, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsListCreate(___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsListCreateA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsListDeleteA(___attrs) \
	AROS_LC1(BOOL, CodesetsListDeleteA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, CODESETS_BASE_NAME, 29, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsListDelete(___tag1, ...) \
	({_sfdc_vararg _tags[] = { ___tag1, __VA_ARGS__ }; CodesetsListDeleteA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsListAddA(___codesetsList, ___attrs) \
	AROS_LC2(BOOL, CodesetsListAddA, \
	AROS_LCA(struct codesetList *, (___codesetsList), A0), \
	AROS_LCA(struct TagItem *, (___attrs), A1), \
	struct Library *, CODESETS_BASE_NAME, 30, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsListAdd(___codesetsList, ___attrs, ...) \
	({_sfdc_vararg _tags[] = { ___attrs, __VA_ARGS__ }; CodesetsListAddA((___codesetsList), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CodesetsListRemoveA(___attrs) \
	AROS_LC1(BOOL, CodesetsListRemoveA, \
	AROS_LCA(struct TagItem *, (___attrs), A0), \
	struct Library *, CODESETS_BASE_NAME, 31, Codesets)

#ifndef NO_INLINE_STDARG
#define CodesetsListRemove(___tag1, ...) \
	({_sfdc_vararg _tags[] = { ___tag1, __VA_ARGS__ }; CodesetsListRemoveA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#endif /* !_INLINE_CODESETS_H */

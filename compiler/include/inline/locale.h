#ifndef _INLINE_LOCALE_H
#define _INLINE_LOCALE_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef LOCALE_BASE_NAME
#define LOCALE_BASE_NAME LocaleBase
#endif

#define CloseCatalog(catalog) \
	LP1NR(0x24, CloseCatalog, struct Catalog *, catalog, a0, \
	, LOCALE_BASE_NAME)

#define CloseLocale(locale) \
	LP1NR(0x2a, CloseLocale, struct Locale *, locale, a0, \
	, LOCALE_BASE_NAME)

#define ConvToLower(locale, character) \
	LP2(0x30, ULONG, ConvToLower, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define ConvToUpper(locale, character) \
	LP2(0x36, ULONG, ConvToUpper, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define FormatDate(locale, fmtTemplate, date, putCharFunc) \
	LP4NR(0x3c, FormatDate, struct Locale *, locale, a0, STRPTR, fmtTemplate, a1, struct DateStamp *, date, a2, struct Hook *, putCharFunc, a3, \
	, LOCALE_BASE_NAME)

#define FormatString(locale, fmtTemplate, dataStream, putCharFunc) \
	LP4(0x42, APTR, FormatString, struct Locale *, locale, a0, STRPTR, fmtTemplate, a1, APTR, dataStream, a2, struct Hook *, putCharFunc, a3, \
	, LOCALE_BASE_NAME)

#define GetCatalogStr(catalog, stringNum, defaultString) \
	LP3(0x48, STRPTR, GetCatalogStr, struct Catalog *, catalog, a0, long, stringNum, d0, STRPTR, defaultString, a1, \
	, LOCALE_BASE_NAME)

#define GetLocaleStr(locale, stringNum) \
	LP2(0x4e, STRPTR, GetLocaleStr, struct Locale *, locale, a0, unsigned long, stringNum, d0, \
	, LOCALE_BASE_NAME)

#define IsAlNum(locale, character) \
	LP2(0x54, BOOL, IsAlNum, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define IsAlpha(locale, character) \
	LP2(0x5a, BOOL, IsAlpha, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define IsCntrl(locale, character) \
	LP2(0x60, BOOL, IsCntrl, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define IsDigit(locale, character) \
	LP2(0x66, BOOL, IsDigit, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define IsGraph(locale, character) \
	LP2(0x6c, BOOL, IsGraph, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define IsLower(locale, character) \
	LP2(0x72, BOOL, IsLower, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define IsPrint(locale, character) \
	LP2(0x78, BOOL, IsPrint, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define IsPunct(locale, character) \
	LP2(0x7e, BOOL, IsPunct, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define IsSpace(locale, character) \
	LP2(0x84, BOOL, IsSpace, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define IsUpper(locale, character) \
	LP2(0x8a, BOOL, IsUpper, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define IsXDigit(locale, character) \
	LP2(0x90, BOOL, IsXDigit, struct Locale *, locale, a0, unsigned long, character, d0, \
	, LOCALE_BASE_NAME)

#define OpenCatalogA(locale, name, tags) \
	LP3(0x96, struct Catalog *, OpenCatalogA, struct Locale *, locale, a0, STRPTR, name, a1, struct TagItem *, tags, a2, \
	, LOCALE_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define OpenCatalog(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; OpenCatalogA((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define OpenLocale(name) \
	LP1(0x9c, struct Locale *, OpenLocale, STRPTR, name, a0, \
	, LOCALE_BASE_NAME)

#define ParseDate(locale, date, fmtTemplate, getCharFunc) \
	LP4(0xa2, BOOL, ParseDate, struct Locale *, locale, a0, struct DateStamp *, date, a1, STRPTR, fmtTemplate, a2, struct Hook *, getCharFunc, a3, \
	, LOCALE_BASE_NAME)

#define StrConvert(locale, string, buffer, bufferSize, type) \
	LP5(0xae, ULONG, StrConvert, struct Locale *, locale, a0, STRPTR, string, a1, APTR, buffer, a2, unsigned long, bufferSize, d0, unsigned long, type, d1, \
	, LOCALE_BASE_NAME)

#define StrnCmp(locale, string1, string2, length, type) \
	LP5(0xb4, LONG, StrnCmp, struct Locale *, locale, a0, STRPTR, string1, a1, STRPTR, string2, a2, long, length, d0, unsigned long, type, d1, \
	, LOCALE_BASE_NAME)

#endif /* _INLINE_LOCALE_H */

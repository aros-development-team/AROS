#ifndef DEFINES_LOCALE_H
#define DEFINES_LOCALE_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define CloseCatalog(catalog) \
    AROS_LC1(void, CloseCatalog, \
    AROS_LCA(struct Catalog *, catalog, A0), \
    struct LocaleBase *, LocaleBase, 6, Locale)

#define CloseLocale(locale) \
    AROS_LC1(void, CloseLocale, \
    AROS_LCA(struct Locale *, locale, A0), \
    struct LocaleBase *, LocaleBase, 7, Locale)

#define ConvToLower(locale, character) \
    AROS_LC2(ULONG, ConvToLower, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 8, Locale)

#define ConvToUpper(locale, character) \
    AROS_LC2(ULONG, ConvToUpper, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 9, Locale)

#define FormatDate(locale, fmtTemplate, date, putCharFunc) \
    AROS_LC4(void, FormatDate, \
    AROS_LCA(struct Locale    *, locale, A0), \
    AROS_LCA(STRPTR            , fmtTemplate, A1), \
    AROS_LCA(struct DateStamp *, date, A2), \
    AROS_LCA(struct Hook      *, putCharFunc, A3), \
    struct LocaleBase *, LocaleBase, 10, Locale)

#define GetCatalogStr(catalog, stringNum, defaultString) \
    AROS_LC3(STRPTR, GetCatalogStr, \
    AROS_LCA(struct Catalog *, catalog, A0), \
    AROS_LCA(LONG            , stringNum, D0), \
    AROS_LCA(STRPTR          , defaultString, A1), \
    struct LocaleBase *, LocaleBase, 12, Locale)

#define GetLocaleStr(locale, stringNum) \
    AROS_LC2(STRPTR, GetLocaleStr, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , stringNum, D0), \
    struct LocaleBase *, LocaleBase, 13, Locale)

#define IsAlNum(locale, character) \
    AROS_LC2(BOOL, IsAlNum, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 14, Locale)

#define IsAlpha(locale, character) \
    AROS_LC2(BOOL, IsAlpha, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 15, Locale)

#define IsCntrl(locale, character) \
    AROS_LC2(BOOL, IsCntrl, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 16, Locale)

#define IsDigit(locale, character) \
    AROS_LC2(BOOL, IsDigit, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 17, Locale)

#define IsGraph(locale, character) \
    AROS_LC2(BOOL, IsGraph, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 18, Locale)

#define IsLower(locale, character) \
    AROS_LC2(BOOL, IsLower, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 19, Locale)

#define IsPrint(locale, character) \
    AROS_LC2(BOOL, IsPrint, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 20, Locale)

#define IsPunct(locale, character) \
    AROS_LC2(BOOL, IsPunct, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 21, Locale)

#define IsSpace(locale, character) \
    AROS_LC2(BOOL, IsSpace, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 22, Locale)

#define IsUpper(locale, character) \
    AROS_LC2(BOOL, IsUpper, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 23, Locale)

#define IsXDigit(locale, character) \
    AROS_LC2(BOOL, IsXDigit, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(ULONG          , character, D0), \
    struct LocaleBase *, LocaleBase, 24, Locale)

#define OpenCatalogA(locale, name, tags) \
    AROS_LC3(struct Catalog *, OpenCatalogA, \
    AROS_LCA(struct Locale  *, locale, A0), \
    AROS_LCA(STRPTR          , name, A1), \
    AROS_LCA(struct TagItem *, tags, A2), \
    struct LocaleBase *, LocaleBase, 25, Locale)

#define OpenLocale(name) \
    AROS_LC1(struct Locale *, OpenLocale, \
    AROS_LCA(STRPTR, name, A0), \
    struct LocaleBase *, LocaleBase, 26, Locale)

#define PrefsUpdate(newLocale) \
    AROS_LC1(struct Locale *, PrefsUpdate, \
    AROS_LCA(struct Locale *, newLocale, A0), \
    struct LocaleBase *, LocaleBase, 28, Locale)

#define StrConvert(locale, string, buffer, bufferSize, type) \
    AROS_LC5(ULONG, StrConvert, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(STRPTR         , string, A1), \
    AROS_LCA(APTR           , buffer, A2), \
    AROS_LCA(ULONG          , bufferSize, D0), \
    AROS_LCA(ULONG          , type, D1), \
    struct LocaleBase *, LocaleBase, 29, Locale)

#define StrnCmp(locale, string1, string2, length, type) \
    AROS_LC5(LONG, StrnCmp, \
    AROS_LCA(struct Locale *, locale, A0), \
    AROS_LCA(STRPTR         , string1, A1), \
    AROS_LCA(STRPTR         , string2, A2), \
    AROS_LCA(LONG           , length, D0), \
    AROS_LCA(ULONG          , type, D1), \
    struct LocaleBase *, LocaleBase, 30, Locale)


#endif /* DEFINES_LOCALE_H */

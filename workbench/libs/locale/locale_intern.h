/*
    Copyright (C) 1995-1998 AROS
    $Id$

    Desc: Internal definitions for the locale.library.
    Lang: english
*/


#ifndef EXEC_EXECBASE_H
#include <exec/execbase.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif
#ifndef DOS_DOSEXTENS_H
#include <dos/dosextens.h>
#endif
#ifndef LIBRARIES_LOCALE_H
#include <libraries/locale.h>
#endif

/* aros_print_not_implemented() macro: */
#include <aros/debug.h>

struct IntLocaleBase
{
    struct LocaleBase        lb_LocaleBase;
    struct ExecBase         *lb_SysBase;
    struct DosLibrary       *lb_DosBase;
    struct Library          *lb_IFFParseBase;
    struct Library          *lb_UtilityBase;

    struct IntLocale	    *lb_CurrentLocale;
    struct SignalSemaphore   lb_LocaleLock;
    struct SignalSemaphore   lb_CatalogLock;
    struct MinList           lb_CatalogList;
};

struct IntLocale
{
    struct Locale       il_Locale;

    UWORD                il_Count;
    struct Library      *il_CurrentLanguage;
    APTR                 il_LanguageFunctions[32];

    /* Need to put all sorts of crap here later. */
    UBYTE		PreferredLanguages[10][30];
    UBYTE		DateTimeFormat[80];
    UBYTE		DateFormat[40];
    UBYTE		TimeFormat[40];
    UBYTE		ShortDateTimeFormat[80];
    UBYTE		ShortDateFormat[40];
    UBYTE		ShortTimeFormat[40];
    UBYTE		DecimalPoint[10];
    UBYTE		Grouping[10];
    UBYTE		FracGrouping[10];
    UBYTE		GroupSeparator[10];
    UBYTE		FracGroupSeparator[10];
    UBYTE		MonDecimalPoint[10];
    UBYTE		MonGroupSeparator[10];
    UBYTE		MonGrouping[10];
    UBYTE		MonFracGrouping[10];
    UBYTE		MonFracGroupSeparator[10];
    UBYTE		MonCS[10];
    UBYTE		MonSmallCS[10];
    UBYTE		MonIntCS[10];
    UBYTE		MonPositiveSign[10];
    UBYTE		MonNegativeSign[10];
};

struct CatStr
{
    struct CatStr       *cs_Next;
    ULONG                cs_Id;
    UBYTE                cs_Data[1];
};

struct IntCatalog
{
    struct Catalog      ic_Catalog;
    struct CatStr      *ic_First;
    ULONG		ic_DataSize;
    UWORD		ic_UseCount;
    ULONG		ic_Flags;
};

/* Catalog strings are in order, so we don't have to search them all */
#define ICF_INORDER	(1L<<0)

/* Shortcuts to the internal structures */
#define IntLB(lb)      ((struct IntLocaleBase *)(lb))
#define IntL(locale)   ((struct IntLocale *)(locale))
#define IntCat(cat)    ((struct IntCatalog *)(cat))

/* Global Library bases in our library base */
#define SysBase     (((struct IntLocaleBase *)LocaleBase)->lb_SysBase)
#define DOSBase     (((struct IntLocaleBase *)LocaleBase)->lb_DosBase)
#define IFFParseBase (((struct IntLocaleBase *)LocaleBase)->lb_IFFParseBase)
#define UtilityBase (((struct IntLocaleBase *)LocaleBase)->lb_UtilityBase)


/* Some internal functions */
ULONG calendar_day(struct DateStamp * date);
ULONG calendar_month(struct DateStamp * date);
ULONG calendar_week(struct DateStamp *  date);
ULONG calendar_weekmonday(struct DateStamp * date);
ULONG calendar_weekday(struct DateStamp * date);
ULONG calendar_year(struct DateStamp * date);

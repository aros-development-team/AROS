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

struct IntLocaleBase
{
    struct LocaleBase        lb_LocaleBase;
    struct ExecBase         *lb_SysBase;
    struct DosLibrary       *lb_DosBase;
    struct Library          *lb_IFFParseBase;
    struct Library          *lb_UtilityBase;

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
};

/* Shortcuts to the internal structures */
#define IntLB(lb)      ((struct IntLocaleBase *)(lb))
#define IntL(locale)   ((struct IntLocale *)(locale))
#define IntCat(cat)    ((struct IntCatalog *)(cat))

/* Global Library bases in our library base */
#define SysBase     (((struct IntLocaleBase *)LocaleBase)->lb_SysBase)
#define DOSBase     (((struct IntLocaleBase *)LocaleBase)->lb_DosBase)
#define IFFParseBase (((struct IntLocaleBase *)LocaleBase)->lb_IFFParseBase)
#define UtilityBase (((struct IntLocaleBase *)LocaleBase)->lb_UtilityBase)

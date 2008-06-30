/*
    Copyright � 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#include "locale_intern.h"

#include <exec/execbase.h>
#include <aros/libcall.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/locale.h>

/*********************************************************************************************/

#define LIB_EXEC    1
#define LIB_UTILITY 2
#define LIB_DOS     3
#define LIB_LOCALE  4

/*********************************************************************************************/

#ifdef __MORPHOS__

extern void LIB_LocRawDoFmt(void);
extern void LIB_LocStrnicmp(void);
extern void LIB_LocStricmp(void);
extern void LIB_LocToLower(void);
extern void LIB_LocToUpper(void);
extern void LIB_LocDateToStr(void);
extern void LIB_LocStrToDate(void);
extern void LIB_LocDosGetLocalizedString(void);

#endif

/*********************************************************************************************/

#ifdef __MORPHOS__
static struct patchinfo
{
    WORD    library;
    WORD    whichfunc;
    APTR    whichpatchfunc;
}
pi [] =
{
    {LIB_EXEC   , 87 , LIB_LocRawDoFmt},
    {LIB_UTILITY, 28 , LIB_LocStrnicmp},
    {LIB_UTILITY, 27 , LIB_LocStricmp},
    {LIB_UTILITY, 30 , LIB_LocToLower},
    {LIB_UTILITY, 29 , LIB_LocToUpper},
    {LIB_DOS    , 124, LIB_LocDateToStr},
    {LIB_DOS    , 125, LIB_LocStrToDate},
    {LIB_DOS    , 163, LIB_LocDosGetLocalizedString},
    {NULL}
};

#else
static struct patchinfo
{
    WORD    library;
    WORD    whichfunc;
    WORD    patchlibrary;
    WORD    whichpatchfunc;
}
pi [] =
{
/* This patch was deactivated because LocRawDoFmt() does not accept %V. There is nothing in the
   documentation telling that RawDoFmt() should depend on current locale, so the patch is deactivated.
   The line is left here for reference.
   
   Pavel Fedin <sonic_amiga@rambler.ru>

    {LIB_EXEC   , 87 , LIB_LOCALE, 31},  * RawDoFmt  */
    {LIB_UTILITY, 28 , LIB_LOCALE, 32}, /* Strnicmp  */
    {LIB_UTILITY, 27 , LIB_LOCALE, 33}, /* Stricmp   */
    {LIB_UTILITY, 30 , LIB_LOCALE, 34}, /* ToLower   */
    {LIB_UTILITY, 29 , LIB_LOCALE, 35}, /* ToUpper   */
    {LIB_DOS    , 124, LIB_LOCALE, 36}, /* DateToStr */
    {LIB_DOS    , 125, LIB_LOCALE, 37}, /* StrToDate */
    {LIB_DOS    , 154, LIB_LOCALE, 38}, /* DosGetLocalizedString */
    {0         	    	    	     }
};
#endif

/*********************************************************************************************/

static struct Library *GetLib(WORD which)
{
    struct Library *lib = NULL;

    switch(which)
    {
    	case LIB_EXEC:
	    lib = (struct Library *)SysBase;
	    break;

	case LIB_DOS:
	    lib = (struct Library *)DOSBase;
	    break;

	case LIB_UTILITY:
	    lib = (struct Library *)UtilityBase;
	    break;

	case LIB_LOCALE:
	    lib = (struct Library *)LocaleBase;
	    break;
    }

    return lib;
}

/*********************************************************************************************/

void InstallPatches(void)
{
    WORD i;

#ifdef __MORPHOS__
    static const struct TagItem PatchTags[] =
    {
	{SETFUNCTAG_MACHINE, MACHINE_PPC},
	{SETFUNCTAG_TYPE, SETFUNCTYPE_NORMAL},
	{SETFUNCTAG_IDNAME, (ULONG) "locale.library Language Patch"},
	{SETFUNCTAG_DELETE, TRUE},
	{TAG_DONE}
    };
#endif

    Forbid();
    for(i = 0; pi[i].library; i++)
    {
#ifdef __MORPHOS__
        NewSetFunction(GetLib(pi[i].library), pi[i].whichpatchfunc, -pi[i].whichfunc * LIB_VECTSIZE, PatchTags);
#else
        SetFunction(GetLib(pi[i].library),
	    	    -pi[i].whichfunc * LIB_VECTSIZE,
		    __AROS_GETVECADDR(GetLib(pi[i].patchlibrary), pi[i].whichpatchfunc));
#endif
    }
    Permit();
    LocaleBase->lb_SysPatches = TRUE;

}

/*********************************************************************************************/

/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "locale_intern.h"

#include <exec/execbase.h>
#include <aros/machine.h>
#include <aros/libcall.h>

extern struct LocaleBase *globallocalebase;

#define LocaleBase globallocalebase

/*********************************************************************************************/

#define LIB_EXEC    1
#define LIB_UTILITY 2
#define LIB_DOS     3
#define LIB_LOCALE  4

/*********************************************************************************************/

static struct patchinfo
{
    WORD    library;
    WORD    whichfunc;
    WORD    patchlibrary;
    WORD    whichpatchfunc;
}
pi [] =
{
    {LIB_EXEC   , 87 , LIB_LOCALE, 31}, /* RawDoFmt  */
    {LIB_UTILITY, 28 , LIB_LOCALE, 32}, /* Strnicmp  */
    {LIB_UTILITY, 27 , LIB_LOCALE, 33}, /* Stricmp   */
    {LIB_UTILITY, 30 , LIB_LOCALE, 34}, /* ToLower   */
    {LIB_UTILITY, 29 , LIB_LOCALE, 35}, /* ToUpper   */
    {LIB_DOS    , 124, LIB_LOCALE, 36}, /* DateToStr */
    {LIB_DOS    , 125, LIB_LOCALE, 37}, /* StrToDate */
    {LIB_DOS    , 154, LIB_LOCALE, 38}, /* DosGetLocalizedString */
    {NULL   	    	    	     }
};

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
    static const struct TagItem PPCTag[] =
    {
	{TASKTAG_CODETYPE, CODETYPE_PPC},
	{TAG_DONE}
    };
#endif

    Forbid();
    for(i = 0; pi[i].library; i++)
    {
        SetFunction(GetLib(pi[i].library),
	    	    -pi[i].whichfunc * LIB_VECTSIZE,
		    __AROS_GETVECADDR(GetLib(pi[i].patchlibrary), pi[i].whichpatchfunc));

    }
    Permit();
    LocaleBase->lb_SysPatches = TRUE;

}

/*********************************************************************************************/

/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <aros/macros.h>

#define DEBUG 1
#include <aros/debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*********************************************************************************************/

static APTR mempool;

/*********************************************************************************************/

static void ScanDirectory(STRPTR pattern, struct List *list, LONG entrysize)
{
    struct AnchorPath 	    ap;
    struct ListviewEntry    *entry;
    STRPTR  	    	    sp;
    LONG    	    	    error;
    
    memset(&ap, 0, sizeof(ap));
    
    error = MatchFirst(pattern, &ap);
    while((error == 0))
    {
    	if (ap.ap_Info.fib_DirEntryType < 0)
	{
	    entry = (struct ListviewEntry *)AllocPooled(mempool, entrysize);
	    if (entry)
	    {
	    	entry->node.ln_Name = entry->name;
		strncpy(entry->name, ap.ap_Info.fib_FileName, sizeof(entry->name));
		
		entry->name[0] = ToUpper(entry->name[0]);
		sp = strchr(entry->name, '.');
		if (sp) sp[0] = '\0';
		
		strcpy(entry->realname, entry->name);
		
		sp = strchr(entry->name, '_');
		if (sp)
		{
		    sp[0] = ' ';
		    if (sp[1]) sp[1] = ToUpper(sp[1]);
		}
		SortInNode(list, &entry->node);
	    }
	}
    	error = MatchNext(&ap);
    }
    MatchEnd(&ap);
}

/*********************************************************************************************/

static BOOL GetCountryFromDefLocale(struct CountryPrefs *country)
{
    BOOL retval = FALSE;
    
    if (LocaleBase)
    {
    	struct Locale *loc = OpenLocale(NULL);
	
	CloseLocale(loc);
    }
    
    return retval;
}

/*********************************************************************************************/

void InitPrefs(void)
{
    struct LanguageEntry *entry;
           
    NewList(&country_list);
    NewList(&language_list);
    NewList(&pref_language_list);
    
    mempool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 2048, 2048);
    if (!mempool) Cleanup("Out of memory!");

    ScanDirectory("LOCALE:Countries/~(#?.info)", &country_list, sizeof(struct CountryEntry));    
    ScanDirectory("LOCALE:Languages/#?.language", &language_list, sizeof(struct LanguageEntry));
    
    /* English language is always available */
    
    if ((entry = AllocPooled(mempool, sizeof(struct LanguageEntry))))
    {
    	strcpy(entry->lve.name, "English");
	entry->lve.node.ln_Name = entry->lve.name;
	
	SortInNode(&language_list, &entry->lve.node);
    }
    
    if (!LoadPrefs("ENV:Sys/locale.prefs"))
    {
    	if (!DefaultPrefs())
	{
	    if (!GetCountryFromDefLocale(&localeprefs.lp_CountryData))
	    {
	    	Cleanup("Panic! Cannot setup default prefs!");
    	    }
	}
    }
}

/*********************************************************************************************/

void CleanupPrefs(void)
{
    if (mempool) DeletePool(mempool);
}

/*********************************************************************************************/

static BOOL LoadCountry(STRPTR name, struct CountryPrefs *country)
{
    struct IFFHandle *iff;    
    UBYTE fullname[100];
    BOOL retval = FALSE;
    
    strcpy(fullname, "LOCALE:Countries");
    AddPart(fullname, name, 100);
    strcat(fullname, ".country");
    
    D(bug("LoadCountry: Trying to open \"%s\"\n", fullname));
    
    if ((iff = AllocIFF()))
    {
    	if ((iff->iff_Stream = (IPTR)Open(fullname, MODE_OLDFILE)))
	{
    	    D(bug("LoadCountry: stream opened.\n"));
	    
	    InitIFFasDOS(iff);
	    
	    if (!OpenIFF(iff, IFFF_READ))
	    {
    	    	D(bug("LoadCountry: OpenIFF okay.\n"));
		
	    	if (!StopChunk(iff, ID_PREF, ID_CTRY))
		{
    	    	    D(bug("LoadCountry: StopChunk okay.\n"));
		    
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
			struct ContextNode *cn;
			
    	    	    	D(bug("LoadCountry: ParseIFF okay.\n"));
			
			cn = CurrentChunk(iff);

			if (cn->cn_Size == sizeof(struct CountryPrefs))
			{
   	    	    	    D(bug("LoadCountry: ID_CTRY chunk size okay.\n"));
			    
		    	    if (ReadChunkBytes(iff, country, sizeof(struct CountryPrefs)) == sizeof(struct CountryPrefs))
			    {
   	    	    	    	D(bug("LoadCountry: Reading chunk successful.\n"));

#if !AROS_BIG_ENDIAN
    	    	    	    	/* Fix endianess */
				
				country->cp_Reserved[0]   = AROS_BE2LONG(country->cp_Reserved[0]);
				country->cp_Reserved[1]   = AROS_BE2LONG(country->cp_Reserved[1]);
				country->cp_Reserved[2]   = AROS_BE2LONG(country->cp_Reserved[2]);
				country->cp_Reserved[3]   = AROS_BE2LONG(country->cp_Reserved[3]);
				country->cp_CountryCode   = AROS_BE2LONG(country->cp_CountryCode);
#endif
   	    	    	    	D(bug("LoadCountry: Everything okay :-)\n"));
				
				retval = TRUE;
			    }
			}
			
		    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		    
		} /* if (!StopChunk(iff, ID_PREF, ID_CTRY)) */
		
	    	CloseIFF(iff);
				
	    } /* if (!OpenIFF(iff, IFFF_READ)) */
	    
	    Close((BPTR)iff->iff_Stream);
	    
	} /* if ((iff->iff_Stream = (IPTR)Open(fullname, MODE_OLDFILE))) */
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
    return retval;
}

/*********************************************************************************************/

BOOL LoadPrefs(STRPTR filename)
{
    return FALSE;
}

/*********************************************************************************************/

BOOL DefaultPrefs(void)
{
    BOOL retval = FALSE;
    WORD i;
    
    TellGUI(PAGECMD_PREFS_CHANGING);
    
    localeprefs.lp_Reserved[0] = 0;
    localeprefs.lp_Reserved[1] = 0;
    localeprefs.lp_Reserved[2] = 0;
    localeprefs.lp_Reserved[3] = 0;
    
    strcpy(localeprefs.lp_CountryName, "united_states");
    
    for(i = 0; i < 10; i++)
    {
    	memset(localeprefs.lp_PreferredLanguages[i], 0, sizeof(localeprefs.lp_PreferredLanguages[i]));
    }
    localeprefs.lp_GMTOffset = 0;
    localeprefs.lp_Flags = 0;
    
    if (LoadCountry("united_states", &localeprefs.lp_CountryData))
    {
    	retval = TRUE;
    }

    TellGUI(PAGECMD_PREFS_CHANGED);
    
    return retval;
}

/*********************************************************************************************/

void GetActualPrefs(void)
{

}

/*********************************************************************************************/

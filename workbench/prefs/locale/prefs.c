/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <aros/macros.h>

#define DEBUG 0
#include <aros/debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

static struct LocalePrefs   restore_prefs;
static APTR 	    	    mempool;

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
		
		sp = entry->name;
		while((sp = strchr(sp, '_')))
		{
		    sp[0] = ' ';
		    if (sp[1])
		    {
		    	/* Make char after underscore uppercase only if no
			   more underscores follow */
		    	if (strchr(sp, '_') == 0)
			{
		    	    sp[1] = ToUpper(sp[1]);
			}
		    }
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

void InitPrefs(STRPTR filename, BOOL use, BOOL save)
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
    	strcpy(entry->lve.realname, "English");
	entry->lve.node.ln_Name = entry->lve.name;
	
	SortInNode(&language_list, &entry->lve.node);
    }
    
    if (!LoadPrefs(filename))
    {
    	if (!DefaultPrefs())
	{
	    if (!GetCountryFromDefLocale(&localeprefs.lp_CountryData))
	    {
	    	Cleanup("Panic! Cannot setup default prefs!");
    	    }
	}
    }
    
    restore_prefs = localeprefs;
    
    if (use || save)
    {
    	SavePrefs(CONFIGNAME_ENV);
    }
    
    if (save)
    {
    	SavePrefs(CONFIGNAME_ENVARC);
    }
    
    if (use || save) Cleanup(NULL);
}

/*********************************************************************************************/

void CleanupPrefs(void)
{
    if (mempool) DeletePool(mempool);
}

/*********************************************************************************************/

#if !AROS_BIG_ENDIAN
static void FixCountryEndianess(struct CountryPrefs *country)
{
    country->cp_Reserved[0] = AROS_BE2LONG(country->cp_Reserved[0]);
    country->cp_Reserved[1] = AROS_BE2LONG(country->cp_Reserved[1]);
    country->cp_Reserved[2] = AROS_BE2LONG(country->cp_Reserved[2]);
    country->cp_Reserved[3] = AROS_BE2LONG(country->cp_Reserved[3]);
    country->cp_CountryCode = AROS_BE2LONG(country->cp_CountryCode);
}
#endif

/*********************************************************************************************/

#if !AROS_BIG_ENDIAN
static void FixLocaleEndianess(struct LocalePrefs *localeprefs)
{
    localeprefs->lp_Reserved[0] = AROS_BE2LONG(localeprefs->lp_Reserved[0]);
    localeprefs->lp_Reserved[1] = AROS_BE2LONG(localeprefs->lp_Reserved[1]);
    localeprefs->lp_Reserved[2] = AROS_BE2LONG(localeprefs->lp_Reserved[2]);
    localeprefs->lp_Reserved[3] = AROS_BE2LONG(localeprefs->lp_Reserved[3]);
    localeprefs->lp_GMTOffset   = AROS_BE2LONG(localeprefs->lp_GMTOffset);
    localeprefs->lp_Flags       = AROS_BE2LONG(localeprefs->lp_Flags);
}
#endif

/*********************************************************************************************/

BOOL LoadCountry(STRPTR name, struct CountryPrefs *country)
{
    static struct CountryPrefs  loadcountry;
    struct IFFHandle 	    	*iff;    
    UBYTE   	    	    	fullname[100];
    BOOL    	    	    	retval = FALSE;
    
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
			    
		    	    if (ReadChunkBytes(iff, &loadcountry, sizeof(struct CountryPrefs)) == sizeof(struct CountryPrefs))
			    {
   	    	    	    	D(bug("LoadCountry: Reading chunk successful.\n"));

    	    	    	    	*country = loadcountry;
				
    	    	    	    #if !AROS_BIG_ENDIAN
    	    	    	    	FixCountryEndianess(country);
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
    static struct LocalePrefs loadprefs;
    struct IFFHandle 	    	*iff;    
    BOOL    	    	    	retval = FALSE;
    
    D(bug("LoadPrefs: Trying to open \"%s\"\n", filename));
    
    if ((iff = AllocIFF()))
    {
    	if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE)))
	{
    	    D(bug("LoadPrefs: stream opened.\n"));
	    
	    InitIFFasDOS(iff);
	    
	    if (!OpenIFF(iff, IFFF_READ))
	    {
    	    	D(bug("LoadPrefs: OpenIFF okay.\n"));
		
	    	if (!StopChunk(iff, ID_PREF, ID_LCLE))
		{
    	    	    D(bug("LoadPrefs: StopChunk okay.\n"));
		    
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
			struct ContextNode *cn;
			
    	    	    	D(bug("LoadPrefs: ParseIFF okay.\n"));
			
			cn = CurrentChunk(iff);

			if (cn->cn_Size == sizeof(struct LocalePrefs))
			{
   	    	    	    D(bug("LoadPrefs: ID_LCLE chunk size okay.\n"));
			    
		    	    if (ReadChunkBytes(iff, &loadprefs, sizeof(struct LocalePrefs)) == sizeof(struct LocalePrefs))
			    {
   	    	    	    	D(bug("LoadPrefs: Reading chunk successful.\n"));

    	    	    	    	TellGUI(PAGECMD_PREFS_CHANGING);

    	    	    	    	localeprefs = loadprefs;
				
    	    	    	    #if !AROS_BIG_ENDIAN
 				FixLocaleEndianess(&localeprefs);
    	    	    	    	FixCountryEndianess(&localeprefs.lp_CountryData);
    	    	    	    #endif
    	    	    	    	TellGUI(PAGECMD_PREFS_CHANGED);
			    
   	    	    	    	D(bug("LoadPrefs: Everything okay :-)\n"));
				
				retval = TRUE;
			    }
			}
			
		    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		    
		} /* if (!StopChunk(iff, ID_PREF, ID_CTRY)) */
		
	    	CloseIFF(iff);
				
	    } /* if (!OpenIFF(iff, IFFF_READ)) */
	    
	    Close((BPTR)iff->iff_Stream);
	    
	} /* if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE))) */
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
    return retval;
}

/*********************************************************************************************/

BOOL SavePrefs(STRPTR filename)
{
    static struct LocalePrefs 	saveprefs;
    struct IFFHandle 	     	*iff;
    WORD    	    	    	i;
    BOOL    	    	    	retval = FALSE, delete_if_error = FALSE;
    
    saveprefs = localeprefs;
#if !AROS_BIG_ENDIAN
    FixLocaleEndianess(&saveprefs);
    FixCountryEndianess(&saveprefs.lp_CountryData);
#endif
    
    for(i = 0; i < 10; i++)
    {
    	saveprefs.lp_PreferredLanguages[i][0] = ToLower(saveprefs.lp_PreferredLanguages[i][0]);
    }
    
    D(bug("SavePrefs: Trying to open \"%s\"\n", filename));
    
    if ((iff = AllocIFF()))
    {
    	if ((iff->iff_Stream = (IPTR)Open(filename, MODE_NEWFILE)))
	{
    	    D(bug("SavePrefs: stream opened.\n"));
	    
	    delete_if_error = TRUE;
	    
	    InitIFFasDOS(iff);
	    
	    if (!OpenIFF(iff, IFFF_WRITE))
	    {
    	    	D(bug("SavePrefs: OpenIFF okay.\n"));
		
		if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
		{
    	    	    D(bug("SavePrefs: PushChunk(FORM) okay.\n"));
		    
		    if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
		    {
		    	struct FilePrefHeader head;

    	    	    	D(bug("SavePrefs: PushChunk(PRHD) okay.\n"));
			
			head.ph_Version  = 0; // FIXME: shouold be PHV_CURRENT, but see <prefs/prefhdr.h> 
			head.ph_Type     = 0;
			head.ph_Flags[0] =
			head.ph_Flags[1] =
			head.ph_Flags[2] =
			head.ph_Flags[3] = 0;
			
			if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
			{
    	    	    	    D(bug("SavePrefs: WriteChunkBytes(PRHD) okay.\n"));
			    
			    PopChunk(iff);
			    
			    if (!PushChunk(iff, ID_PREF, ID_LCLE, sizeof(struct LocalePrefs)))
			    {
    	    	    	    	D(bug("SavePrefs: PushChunk(LCLE) okay.\n"));
				
			    	if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
				{
   	    	    	    	    D(bug("SavePrefs: WriteChunkBytes(LCLE) okay.\n"));
  	    	    	    	    D(bug("SavePrefs: Everything okay :-)\n"));
				    
				    retval = TRUE;
				}
				
    			    	PopChunk(iff);

			    } /* if (!PushChunk(iff, ID_PREF, ID_LCLE, sizeof(struct LocalePrefs))) */
			    			    
			} /* if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head)) */
			else
		    	{
			    PopChunk(iff);
			}
			
		    } /* if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct PrefHeader))) */
		    
		    PopChunk(iff);
		    		    
		} /* if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN)) */
		
	    	CloseIFF(iff);
				
	    } /* if (!OpenIFF(iff, IFFFWRITE)) */
	    
	    Close((BPTR)iff->iff_Stream);
	    
	} /* if ((iff->iff_Stream = (IPTR)Open(filename, MODE_NEWFILE))) */
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
    if (!retval && delete_if_error)
    {
    	DeleteFile(filename);
    }
    
    return retval;    
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
    localeprefs.lp_GMTOffset = 5 * 60;
    localeprefs.lp_Flags = 0;
    
    if (LoadCountry("united_states", &localeprefs.lp_CountryData))
    {
    	retval = TRUE;
    }

    TellGUI(PAGECMD_PREFS_CHANGED);
    
    return retval;
}

/*********************************************************************************************/

void RestorePrefs(void)
{
    TellGUI(PAGECMD_PREFS_CHANGING);
    localeprefs = restore_prefs;
    TellGUI(PAGECMD_PREFS_CHANGED);   
}

/*********************************************************************************************/

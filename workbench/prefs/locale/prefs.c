/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "global.h"
#include <aros/macros.h>

//#define DEBUG 1 
#include <aros/debug.h>

#include <prefs/prefhdr.h>

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

static struct LocalePrefs   restore_prefs;
struct LocalePrefs          localeprefs;

/*********************************************************************************************/

void CleanupPrefs(void)
{
    D(bug("[locale prefs] CleanupPrefs\n"));
    if (mempool) DeletePool(mempool);
}

/*********************************************************************************************/

void CopyPrefs(struct LocalePrefs *s, struct LocalePrefs *d)
{
    CopyMem(s, d, sizeof(struct LocalePrefs));
}

void BackupPrefs(void)
{
    CopyPrefs(&localeprefs, &restore_prefs);
}

void RestorePrefs(void)
{
    CopyPrefs(&restore_prefs, &localeprefs);
}

/*********************************************************************************************/

void SortInNode(struct List *list, struct Node *node)
{
    struct Node *sort, *prev = NULL;
    struct Locale *loc;

    loc = OpenLocale(NULL);

    ForeachNode(list, sort)
    {
        if (StrnCmp(loc,
                node->ln_Name, sort->ln_Name,
                strlen(node->ln_Name), SC_COLLATE2)
            < 0)
        {
            break;
        }
	prev = sort;
    }

    Insert(list, node, prev);
    CloseLocale(loc);
}

static void ScanDirectory(char *pattern, struct List *list, LONG entrysize)
{
    struct AnchorPath 	    ap;
    struct ListviewEntry    *entry;
    char   	    	    *sp;
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
		strncpy( entry->name, 
		         (const char *) ap.ap_Info.fib_FileName, 
			 sizeof(entry->name) );
		
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

/* 1 is success */

ULONG InitPrefs(STRPTR filename, BOOL use, BOOL save)
{
    struct LanguageEntry *entry;

    D(bug("[locale prefs] InitPrefs\n"));

    mempool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 2048, 2048);
    if (!mempool) 
    {
	ShowMsg("Out of memory!");
	return 0;
    }

    if (!LoadPrefs(filename))
    {
    	if (!DefaultPrefs())
	{
    	    CleanupPrefs();
	    ShowMsg("Panic! Cannot setup default prefs!");
	    return 0;
	}
    }
    
    NewList(&country_list);
    NewList(&language_list);
    NewList(&pref_language_list);
    
    ScanDirectory("LOCALE:Countries/~(#?.info)", &country_list, sizeof(struct CountryEntry));    
    ScanDirectory("LOCALE:Languages/#?.language", &language_list, sizeof(struct LanguageEntry));
    
    /* English language is always available */
    
    if ((entry = AllocPooled(mempool, sizeof(struct LanguageEntry))))
    {
    	strcpy( entry->lve.name, "English");
    	strcpy( entry->lve.realname, "English");
	entry->lve.node.ln_Name = entry->lve.name;
	
	SortInNode(&language_list, &entry->lve.node);
    }
    
    restore_prefs = localeprefs;
    
    if (use || save)
    {
    	SavePrefs((CONST STRPTR) CONFIGNAME_ENV);
    }
    
    if (save)
    {
    	SavePrefs((CONST STRPTR) CONFIGNAME_ENVARC);
    }
    
    if (use || save) CleanupPrefs();

    return 1;
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

BOOL LoadCountry(STRPTR name, struct CountryPrefs *country)
{
    static struct CountryPrefs  loadcountry;
    struct IFFHandle 	    	*iff;    
    char   	    	    	fullname[100];
    BOOL    	    	    	retval = FALSE;
    
    strcpy(fullname, "LOCALE:Countries");
    AddPart(fullname, name, 100);
    strcat(fullname, ".country");
    
    D(bug("[locale prefs] LoadCountry: Trying to open \"%s\"\n", fullname));
    
    if ((iff = AllocIFF()))
    {
    	if ((iff->iff_Stream = (IPTR)Open(fullname, MODE_OLDFILE)))
	{
    	    D(bug("[locale prefs] LoadCountry: stream opened.\n"));
	    
	    InitIFFasDOS(iff);
	    
	    if (!OpenIFF(iff, IFFF_READ))
	    {
    	    	D(bug("[locale prefs] LoadCountry: OpenIFF okay.\n"));
		
	    	if (!StopChunk(iff, ID_PREF, ID_CTRY))
		{
    	    	    D(bug("[locale prefs] LoadCountry: StopChunk okay.\n"));
		    
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
			struct ContextNode *cn;
			
    	    	    	D(bug("[locale prefs] LoadCountry: ParseIFF okay.\n"));
			
			cn = CurrentChunk(iff);

			if (cn->cn_Size == sizeof(struct CountryPrefs))
			{
   	    	    	    D(bug("[locale prefs] LoadCountry: ID_CTRY chunk size okay.\n"));
			    
		    	    if (ReadChunkBytes(iff, &loadcountry, sizeof(struct CountryPrefs)) == sizeof(struct CountryPrefs))
			    {
   	    	    	    	D(bug("[locale prefs] LoadCountry: Reading chunk successful.\n"));

    	    	    	    	*country = loadcountry;
				
    	    	    	    #if !AROS_BIG_ENDIAN
    	    	    	    	FixCountryEndianess(country);
    	    	    	    #endif
			    
   	    	    	    	D(bug("[locale prefs] LoadCountry: Everything okay :-)\n"));
				
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

BOOL LoadPrefsFH(BPTR fh)
{
    static struct LocalePrefs loadprefs;
    struct IFFHandle 	    	*iff;    
    BOOL    	    	    	retval = FALSE;
    
    D(bug("[locale prefs] LoadPrefsFH\n"));
    
    if ((iff = AllocIFF()))
    {
    	if ((iff->iff_Stream = (IPTR) fh))
	{
    	    D(bug("[locale prefs] LoadPrefs: stream is ok.\n"));
	    
	    InitIFFasDOS(iff);
	    
	    if (!OpenIFF(iff, IFFF_READ))
	    {
    	    	D(bug("[locale prefs] LoadPrefs: OpenIFF okay.\n"));
		
	    	if (!StopChunk(iff, ID_PREF, ID_LCLE))
		{
    	    	    D(bug("[locale prefs] LoadPrefs: StopChunk okay.\n"));
		    
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
			struct ContextNode *cn;
			
    	    	    	D(bug("[locale prefs] LoadPrefs: ParseIFF okay.\n"));
			
			cn = CurrentChunk(iff);

			if (cn->cn_Size == sizeof(struct LocalePrefs))
			{
   	    	    	    D(bug("[locale prefs] LoadPrefs: ID_LCLE chunk size okay.\n"));
			    
		    	    if (ReadChunkBytes(iff, &loadprefs, sizeof(struct LocalePrefs)) == sizeof(struct LocalePrefs))
			    {
   	    	    	    	D(bug("[locale prefs] LoadPrefs: Reading chunk successful.\n"));

    	    	    	    	localeprefs = loadprefs;
				
    	    	    	    #if !AROS_BIG_ENDIAN
 				FixLocaleEndianess(&localeprefs);
    	    	    	    	FixCountryEndianess(&localeprefs.lp_CountryData);
    	    	    	    #endif
			    
   	    	    	    	D(bug("[locale prefs] LoadPrefs: Everything okay :-)\n"));
				
				retval = TRUE;
			    }
			}
			
		    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		    
		} /* if (!StopChunk(iff, ID_PREF, ID_CTRY)) */
		
	    	CloseIFF(iff);
				
	    } /* if (!OpenIFF(iff, IFFF_READ)) */
	    
	} /* if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE))) */
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
    D(bug("[locale prefs] CountryName: %s\n",localeprefs.lp_CountryName));
    int i=0;
    while(i<10 && localeprefs.lp_PreferredLanguages[i])
    {
      D(bug("[locale prefs] preferred %d: %s\n",i,localeprefs.lp_PreferredLanguages[i]));
      i++;
    }
    D(bug("[locale prefs] lp_GMTOffset: %d\n",localeprefs.lp_GMTOffset));
    return retval;
}

BOOL LoadPrefs(STRPTR filename) 
{
    BPTR fh;
    BOOL ret;

    D(bug("[locale prefs] LoadPrefsFH: Trying to open \"%s\"\n", filename));

    fh=Open(filename, MODE_OLDFILE);

    if(!fh) return FALSE;

    ret=LoadPrefsFH(fh);

    Close(fh);
    return ret;
}


/*********************************************************************************************/

BOOL SavePrefsFH(BPTR fh)
{
    static struct LocalePrefs 	saveprefs;
    struct IFFHandle 	     	*iff;    
    BOOL    	    	    	retval = FALSE, delete_if_error = FALSE;
    
    D(bug("[locale prefs] SavePrefsFH: fh: %lx\n", fh));

    saveprefs = localeprefs;

#if !AROS_BIG_ENDIAN
    FixLocaleEndianess(&saveprefs);
    FixCountryEndianess(&saveprefs.lp_CountryData);
#endif
    
    if ((iff = AllocIFF()))
    {
	iff->iff_Stream = (IPTR) fh;
	D(bug("[locale prefs] SavePrefsFH: stream opened.\n"));
	    
	    delete_if_error = TRUE;
	    
	    InitIFFasDOS(iff);
	    
	    if (!OpenIFF(iff, IFFF_WRITE))
	    {
    	    	D(bug("[locale prefs] SavePrefsFH: OpenIFF okay.\n"));
		
		if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
		{
    	    	    D(bug("[locale prefs] SavePrefsFH: PushChunk(FORM) okay.\n"));
		    
		    if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
		    {
		    	struct FilePrefHeader head;

    	    	    	D(bug("[locale prefs] SavePrefsFH: PushChunk(PRHD) okay.\n"));
			
			head.ph_Version  = PHV_CURRENT; 
			head.ph_Type     = 0;
			head.ph_Flags[0] =
			head.ph_Flags[1] =
			head.ph_Flags[2] =
			head.ph_Flags[3] = 0;
			
			if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
			{
    	    	    	    D(bug("[locale prefs] SavePrefsFH: WriteChunkBytes(PRHD) okay.\n"));
			    
			    PopChunk(iff);
			    
			    if (!PushChunk(iff, ID_PREF, ID_LCLE, sizeof(struct LocalePrefs)))
			    {
    	    	    	    	D(bug("[locale prefs] SavePrefsFH: PushChunk(LCLE) okay.\n"));
				
			    	if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
				{
   	    	    	    	    D(bug("[locale prefs] SavePrefsFH: WriteChunkBytes(SERL) okay.\n"));
  	    	    	    	    D(bug("[locale prefs] SavePrefsFH: Everything okay :-)\n"));
				    
				    retval = TRUE;
				}
				
    			    	PopChunk(iff);

			    } /* if (!PushChunk(iff, ID_PREF, ID_SERL, sizeof(struct LocalePrefs))) */
			    			    
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
	    
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
#if 0
    if (!retval && delete_if_error)
    {
    	DeleteFile(filename);
    }
#endif
	    
    
    return retval;    
}

BOOL SavePrefs(CONST STRPTR filename) 
{
    BPTR fh;
    BOOL ret;

    D(bug("[locale prefs] SavePrefs: Trying to open \"%s\"\n", filename));

    fh=Open(filename, MODE_NEWFILE);

    if(fh == NULL) 
    {
      	D(bug("[locale prefs] open \"%s\" failed!\n", filename));
       	return FALSE;
    }

    ret=SavePrefsFH(fh);

    Close(fh);
    return ret;
}

/*********************************************************************************************/

BOOL SaveEnv() {
    BPTR fh;
    BOOL result;

    D(bug("[locale prefs] SaveEnv: Trying to open \"%s\"\n", CONFIGNAME_ENV));

    fh=Open((CONST_STRPTR) CONFIGNAME_ENV, MODE_NEWFILE);

    if(fh == NULL) 
    {
	D(bug("[locale prefs] open \"%s\" failed!\n", CONFIGNAME_ENV));
	return FALSE;
    }

    result=SavePrefsFH(fh);

    Close(fh);

    return result;
}

/*********************************************************************************************/

BOOL DefaultPrefs(void)
{
    BOOL retval = FALSE;
    WORD i;
    
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
    
    if (LoadCountry((STRPTR) "united_states", &localeprefs.lp_CountryData))
    {
    	retval = TRUE;
    }

    return retval;
}


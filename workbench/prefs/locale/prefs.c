/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

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
		if (sp) *sp = '\0';
		
		sp = strchr(entry->name, '_');
		if (sp) if (sp[1]) sp[1] = ToUpper(sp[1]);
		
		SortInNode(list, &entry->node);
	    }
	}
    	error = MatchNext(&ap);
    }
    MatchEnd(&ap);
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
}

/*********************************************************************************************/

void CleanupPrefs(void)
{
    if (mempool) DeletePool(mempool);
}

/*********************************************************************************************/

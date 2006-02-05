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

#define ARRAY_TO_LONG(x) ( ((x)[0] << 24UL) + ((x)[1] << 16UL) + ((x)[2] << 8UL) + ((x)[3]) )
#define ARRAY_TO_WORD(x) ( ((x)[0] << 8UL) + ((x)[1]) )

#define LONG_TO_ARRAY(x,y) (y)[0] = (UBYTE)(ULONG)((x) >> 24UL); \
    	    	    	   (y)[1] = (UBYTE)(ULONG)((x) >> 16UL); \
			   (y)[2] = (UBYTE)(ULONG)((x) >>  8UL); \
			   (y)[3] = (UBYTE)(ULONG)((x));

#define WORD_TO_ARRAY(x,y) (y)[0] = (UBYTE)(ULONG)((x) >>  8UL); \
			   (y)[1] = (UBYTE)(ULONG)((x));
			   
/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

struct FileInputPrefs
{
    char    ip_Keymap[16];
    UBYTE   ip_PointerTicks[2];
    UBYTE   ip_DoubleClick_secs[4];
    UBYTE   ip_DoubleClick_micro[4];
    UBYTE   ip_KeyRptDelay_secs[4];
    UBYTE   ip_KeyRptDelay_micro[4];
    UBYTE   ip_KeyRptSpeed_secs[4];
    UBYTE   ip_KeyRptSpeed_micro[4];
    UBYTE   ip_MouseAccel[2];
};

/*********************************************************************************************/

static APTR 	    	    mempool;

/*********************************************************************************************/

struct nameexp
{
    STRPTR shortname;
    STRPTR longname;
};

/*********************************************************************************************/

struct nameexp model_expansion_table[] =
{
    {"pc105", "PC 105"},
    {"pc104", "PC 104/101"},
    {NULL   ,  NULL   }
};

/*********************************************************************************************/

struct nameexp layout_expansion_table[] =
{
    {"al"   , "Albanian"    	    },
    {"usa1" , "American"    	    },
    {"usa"  , "American"    	    },
    {"us"   , "American (PC)"	    },
    {"usx"  , "American Extended"   },
    {"d"    , "Deutsch"     	    },
    {"be"   , "Belge"     	    },
    {"br"   , "Brasileiro"     	    },
    {"gb"   , "British"     	    },
    {"gbx"  , "British Extended"    },
    {"cdn"  , "Canadien Français"   },
    {"ca"   , "Canadien Français"   },
    {"cz"   , "Czech"	    	    },
    {"dk"   , "Dansk"	    	    },
    {"ne"   , "Dutch"	    	    },
    {"dvx"  , "Dvorak"              },
    {"usa2" , "Dvorak"	    	    },
    {"dvl"  , "Dvorak Left-handed"  },
    {"dvr"  , "Dvorak Right-handed" },
    {"ir"   , "English Ireland"     },
    {"e"    , "Español"     	    },
    {"sp"   , "Español no deadkeys" },
    {"et"   , "Estonian"    	    },
    {"fi"   , "Finnish"    	    },
    {"f"    , "Français"    	    },
    {"ic"   , "Icelandic"    	    },
    {"i"    , "Italiana"    	    },
    {"la"   , "Latin American"      },
    {"lv"   , "Latvian"             },
    {"lt"   , "Lithuanian"          },
    {"hu"   , "Magyar"    	    },
    {"n"    , "Norsk"	    	    },
    {"pl"   , "Polski"		    },
    {"po"   , "Português"   	    },
    {"ru"   , "Russian"     	    },
    {"sg"   , "Schweiz"     	    },
    {"ch2"  , "Schweiz"     	    },
    {"sl"   , "Slovak"     	    },
    {"sf"   , "Suisse"	    	    },
    {"ch1"  , "Suisse"	    	    },
    {"s"    , "Svenskt"     	    },
    {"ur"   , "Ucranian"     	    },
    {NULL   , NULL  	    	    }
};

/*********************************************************************************************/

static void ExpandName(STRPTR name, struct nameexp *exp)
{
    for(; exp->shortname; exp++)
    {
    	if (stricmp(exp->shortname, name) == 0)
	{
	    strcpy(name, exp->longname);
	    break;
	}
    }
}

/*********************************************************************************************/

static void ScanDirectory(STRPTR pattern, struct List *list, LONG entrysize)
{
    struct AnchorPath 	    ap;
    struct ListviewEntry    *entry, *entry2;
    struct List     	    templist;
    STRPTR  	    	    sp;
    LONG    	    	    error;
    
    memset(&ap, 0, sizeof(ap));
    NewList(&templist);

    error = MatchFirst(pattern, &ap);
    while((error == 0))
    {
    	if (ap.ap_Info.fib_DirEntryType < 0)
	{
	    entry = (struct ListviewEntry *)AllocPooled(mempool, entrysize);
	    if (entry)
	    {
	    	entry->node.ln_Name = entry->modelname;
		strncpy(entry->realname, ap.ap_Info.fib_FileName, sizeof(entry->realname));
		
		// entry->name[0] = ToUpper(entry->name[0]);
		
		sp = strchr(entry->realname, '_');
		if (sp)
		{
		    sp[0] = '\0';
		    strcpy(entry->modelname, entry->realname);
		    strcpy(entry->layoutname, sp + 1);
		    sp[0] = '_';
		}
		else
		{
		    strcpy(entry->modelname, "Amiga");
		    strcpy(entry->layoutname, entry->realname);
		}
		ExpandName(entry->modelname, model_expansion_table);
		ExpandName(entry->layoutname, layout_expansion_table);
		AddTail(&templist, &entry->node);
	    }
	}
    	error = MatchNext(&ap);
    }
    MatchEnd(&ap);
    
    /* Sort by Model Name */
    
    ForeachNodeSafe(&templist, entry, entry2)
    {
    	Remove(&entry->node);
    	SortInNode(list, &entry->node);
    }
    
    /* Fix ln_Name to point to Layout Name */
    
    ForeachNode(list, entry)
    {
    	entry->node.ln_Name = entry->layoutname;
    }
    
    /* Move back to temp list */
    
    NewList(&templist);
    
    ForeachNodeSafe(list, entry, entry2)
    {
    	Remove(&entry->node);
	AddTail(&templist, &entry->node);
    }
    
    /* Create Model ~tree nodes + sort by Layout Name in each Model ~tree */
    
    NewList(list);
    
    ForeachNodeSafe(&templist, entry, entry2)
    {
    	struct ListviewEntry    *modelentry, *layoutentry;
	struct List 	    	layoutlist;

	modelentry = (struct ListviewEntry *)AllocPooled(mempool, entrysize);
	if (!modelentry) break;
	
	modelentry->modelnode    = TRUE;
	modelentry->node.ln_Name = entry->modelname;
	
	AddTail(list, &modelentry->node);
	
	/* Create a sorted list of layouts for this model ~tree */
	
	NewList(&layoutlist);
	for(; (entry2 = (struct ListviewEntry *)entry->node.ln_Succ); entry = entry2)
	{
	    if (stricmp(entry->modelname, modelentry->node.ln_Name) != 0)
	    {
	    	entry2 = entry;
	    	break;
	    }
	    Remove(&entry->node);
	    SortInNode(&layoutlist, &entry->node);
	}
	
	/* Add the sorted list of layouts to the final list */
	
	while((layoutentry = (struct ListviewEntry *)RemHead(&layoutlist)))
	{
	    AddTail(list, &layoutentry->node);
	}
 
    	if (!entry2) break; 
	
    } /* ForeachNodeSafe(&templist, entry, entry2) */
    
#if 0
    ForeachNode(list, entry)
    {
    	kprintf("%s%s\n", (entry->modelnode ? "* " : "   "), entry->node.ln_Name);
    }
#endif
}

/*********************************************************************************************/

void InitPrefs(STRPTR filename, BOOL use, BOOL save)
{
           
    NewList(&keymap_list);

    mempool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 2048, 2048);
    if (!mempool) Cleanup("Out of memory!");

    ScanDirectory("DEVS:Keymaps/#?_~(#?.info)", &keymap_list, sizeof(struct KeymapEntry));    
    
    if (!LoadPrefs(filename))
    {
    	if (!DefaultPrefs())
	{
	}
    }
    
    restore_prefs = inputprefs;
    
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

BOOL LoadPrefs(STRPTR filename)
{
    static struct FileInputPrefs    loadprefs;
    struct IFFHandle 	    	    *iff;    
    BOOL    	    	    	    retval = FALSE;
    
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
		
	    	if (!StopChunk(iff, ID_PREF, ID_INPT))
		{
    	    	    D(bug("LoadPrefs: StopChunk okay.\n"));
		    
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
			struct ContextNode *cn;
			
    	    	    	D(bug("LoadPrefs: ParseIFF okay.\n"));
			
			cn = CurrentChunk(iff);

			if (cn->cn_Size == sizeof(struct FileInputPrefs))
			{
   	    	    	    D(bug("LoadPrefs: ID_INPT chunk size okay.\n"));
			    
		    	    if (ReadChunkBytes(iff, &loadprefs, sizeof(struct FileInputPrefs)) == sizeof(struct FileInputPrefs))
			    {
   	    	    	    	D(bug("LoadPrefs: Reading chunk successful.\n"));

    	    	    	    	TellGUI(PAGECMD_PREFS_CHANGING);

    				CopyMem(loadprefs.ip_Keymap, inputprefs.ip_Keymap, sizeof(loadprefs.ip_Keymap));	
				inputprefs.ip_PointerTicks         = ARRAY_TO_WORD(loadprefs.ip_PointerTicks);
				inputprefs.ip_DoubleClick.tv_secs  = ARRAY_TO_LONG(loadprefs.ip_DoubleClick_secs);
				inputprefs.ip_DoubleClick.tv_micro = ARRAY_TO_LONG(loadprefs.ip_DoubleClick_micro);
				inputprefs.ip_KeyRptDelay.tv_secs  = ARRAY_TO_LONG(loadprefs.ip_KeyRptDelay_secs);
				inputprefs.ip_KeyRptDelay.tv_micro = ARRAY_TO_LONG(loadprefs.ip_KeyRptDelay_micro);
				inputprefs.ip_KeyRptSpeed.tv_secs  = ARRAY_TO_LONG(loadprefs.ip_KeyRptSpeed_secs);
				inputprefs.ip_KeyRptSpeed.tv_micro = ARRAY_TO_LONG(loadprefs.ip_KeyRptSpeed_micro);
				inputprefs.ip_MouseAccel    	   = ARRAY_TO_WORD(loadprefs.ip_MouseAccel);

   	    	    	    	TellGUI(PAGECMD_PREFS_CHANGED);
			    
   	    	    	    	D(bug("LoadPrefs: Everything okay :-)\n"));
				
				retval = TRUE;
			    }
			}
			
		    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		    
		} /* if (!StopChunk(iff, ID_PREF, ID_INPT)) */
		
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
    static struct FileInputPrefs    saveprefs;
    struct IFFHandle 	     	    *iff;    
    BOOL    	    	    	    retval = FALSE, delete_if_error = FALSE;
    
    CopyMem(inputprefs.ip_Keymap, saveprefs.ip_Keymap, sizeof(saveprefs.ip_Keymap));	
    WORD_TO_ARRAY(inputprefs.ip_PointerTicks, saveprefs.ip_PointerTicks);
    LONG_TO_ARRAY(inputprefs.ip_DoubleClick.tv_secs , saveprefs.ip_DoubleClick_secs);
    LONG_TO_ARRAY(inputprefs.ip_DoubleClick.tv_micro, saveprefs.ip_DoubleClick_micro);
    LONG_TO_ARRAY(inputprefs.ip_KeyRptDelay.tv_secs , saveprefs.ip_KeyRptDelay_secs);
    LONG_TO_ARRAY(inputprefs.ip_KeyRptDelay.tv_micro, saveprefs.ip_KeyRptDelay_micro);
    LONG_TO_ARRAY(inputprefs.ip_KeyRptSpeed.tv_secs , saveprefs.ip_KeyRptSpeed_secs);
    LONG_TO_ARRAY(inputprefs.ip_KeyRptSpeed.tv_micro, saveprefs.ip_KeyRptSpeed_micro);
    WORD_TO_ARRAY(inputprefs.ip_MouseAccel, saveprefs.ip_MouseAccel);    
	
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
			    
			    if (!PushChunk(iff, ID_PREF, ID_INPT, sizeof(saveprefs)))
			    {
    	    	    	    	D(bug("SavePrefs: PushChunk(INPT) okay.\n"));
				
			    	if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
				{
   	    	    	    	    D(bug("SavePrefs: WriteChunkBytes(INPT) okay.\n"));
  	    	    	    	    D(bug("SavePrefs: Everything okay :-)\n"));
				    
				    retval = TRUE;
				}
				
    			    	PopChunk(iff);

			    } /* if (!PushChunk(iff, ID_PREF, ID_INPT, sizeof(saveprefs))) */
			    			    
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
    TellGUI(PAGECMD_PREFS_CHANGING);
    
    strcpy(inputprefs.ip_Keymap, "amiga_usa0");
    inputprefs.ip_PointerTicks         = 1;
    inputprefs.ip_DoubleClick.tv_secs  = 0;
    inputprefs.ip_DoubleClick.tv_micro = 500000;
    inputprefs.ip_KeyRptDelay.tv_secs  = 0;
    inputprefs.ip_KeyRptDelay.tv_micro = 500000;
    inputprefs.ip_KeyRptSpeed.tv_secs  = 0;
    inputprefs.ip_KeyRptSpeed.tv_micro = 40000;
    inputprefs.ip_MouseAccel           = 1;
    
    TellGUI(PAGECMD_PREFS_CHANGED);
    
    return TRUE;
}

/*********************************************************************************************/

void RestorePrefs(void)
{
    TellGUI(PAGECMD_PREFS_CHANGING);
    inputprefs = restore_prefs;
    TellGUI(PAGECMD_PREFS_CHANGED);   
}

/*********************************************************************************************/

/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#define SHOWFLAGS 1  /* Set to 1 to show Flags in the keylist */

#include <proto/keymap.h>

#define DEBUG 0
#include <aros/debug.h>

#include "prefs.h"

/*********************************************************************************************/

IPTR                mempool;
BOOL                inputdev_changed = FALSE;
struct List         keymap_list;
struct InputPrefs   inputprefs;
struct InputPrefs   restore_prefs;
struct MsgPort     *InputMP;
struct timerequest *InputIO;
BPTR                testkeymap_seg = NULL;

/*********************************************************************************************/

struct nameexp layout_expansion_table[] =
{
    {"al"   , "Albanian"    	    , NULL },
    {"usa"  , "American (Sun 5c)"   , "United_States"   },
    {"us"   , "American (PC 104)"   , "United_States"   },
    {"usx"  , "American (PC 105)"   , "United_States"   },
    {"d"    , "Deutsch"     	    , "Deutschland"     },
    {"be"   , "Belge"     	        , "Belgique"        },
    {"br"   , "Brasileiro"     	    , "Brasil"          },
    {"gb"   , "British"     	    , "United_Kingdom"  },
    {"gbx"  , "British Extended"    , "United_Kingdom"  },
    {"cdn"  , "Canadien Français"   , "Canada"          },
    {"ca"   , "Canadien Français"   , "Canada_Français" },
    {"cz"   , "Czech"	    	    , "Cesko"           },
    {"dk"   , "Dansk"	    	    , "Danmark"         },
    {"ne"   , "Dutch"	    	    , "Nederland"       },
    {"dvx"  , "Dvorak"              , "Espana"          },
    {"usa2" , "Dvorak"	    	    , NULL },
    {"dvl"  , "Dvorak Left-handed"  , NULL },
    {"dvr"  , "Dvorak Right-handed" , NULL },
    {"el"   , "Ellinikí"    	    , "Hellas"           },
    {"ir"   , "English Ireland"     , "Ireland"        },
    {"e"    , "Español"     	    , "España" },
    {"sp"   , "Español no deadkeys" , "España" },
    {"et"   , "Estonian"    	    , NULL },
    {"fi"   , "Finnish"    	        , "Suomi"            },
    {"f"    , "Français"    	    , "France"           },
    {"hr"   , "Hrvatski"    	    , "Hrvatska"         },
    {"ic"   , "Icelandic"    	    , "Island"           },
    {"i"    , "Italiana"    	    , "Italia"           },
    {"la"   , "Latin American"      , NULL },
    {"lv"   , "Latvian"             , NULL },
    {"lt"   , "Lithuanian"          , NULL },
    {"hu"   , "Magyar"    	        , NULL },
    {"n"    , "Norsk"	    	    , "Norge"            },
    {"pl"   , "Polski"		        , "Polska"           },
    {"po"   , "Português"   	    , "Portugal"         },
    {"ru"   , "Russian"     	    , "Rossija"          },
    {"sg"   , "Schweiz"     	    , "Suisse"           },
    {"ch2"  , "Schweiz"     	    , "Suisse"           },
    {"sl"   , "Slovak"     	        , "Slovakia"         },
    {"sf"   , "Suisse"	    	    , "Suisse"           },
    {"ch1"  , "Suisse"	    	    , "Suisse"           },
    {"s"    , "Svenskt"     	    , "Sverige"          },
    {"ur"   , "Ucranian"     	    , "Ukrajina"         },
    {NULL   , NULL  	    	    , NULL }
};

/*********************************************************************************************/

static void ExpandName(STRPTR name, STRPTR flag, struct nameexp *exp)
{
    for(; exp->shortname; exp++)
    {
    	if (stricmp(exp->shortname, name) == 0)
	{
        strcpy(name, exp->longname);
        if (exp->flag != NULL) strcpy(flag, exp->flag);
        break;
	   }
    }
}

/*********************************************************************************************/


void SortInNode(struct List *list, struct Node *node)
{
    struct Node *sort, *prev = NULL;

    ForeachNode(list, sort)
    {
        if (Stricmp(node->ln_Name, sort->ln_Name) < 0) break;
        prev = sort;
    }

    Insert(list, node, prev);
}

void ScanDirectory(STRPTR pattern, struct List *list, LONG entrysize)
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
	    entry = (struct ListviewEntry *)AllocPooled((APTR)mempool, entrysize);
	    if (entry)
	    {
	    	entry->node.ln_Name = entry->layoutname;
		strncpy(entry->realname, ap.ap_Info.fib_FileName, sizeof(entry->realname));
		
		sp = strchr(entry->realname, '_');
		if (sp)
		{
		    sp[0] = '\0';
		    strcpy(entry->layoutname, sp + 1);
		    sp[0] = '_';
		}
		else
		    strcpy(entry->layoutname, entry->realname);

		ExpandName(entry->layoutname, entry->flagname, layout_expansion_table);
		AddTail(&templist, &entry->node);
	    }
	}
    	error = MatchNext(&ap);
    }
    MatchEnd(&ap);
    
    /* Sort by Layout Name */
    
    ForeachNodeSafe(&templist, entry, entry2)
    {
    	Remove(&entry->node);
    	SortInNode(list, &entry->node);
    }

    ForeachNode(list, entry)
    {
#if SHOWFLAGS == 1
	sprintf(entry->displayname, "\033I[5:Locale:Flags/Countries/%s]%s", entry->flagname, entry->node.ln_Name);
#else
	sprintf(entry->displayname, "%s", entry->node.ln_Name);
#endif
	D(bug("IPrefs: kbd entry flag: %s\n", entry->flagname));
    }
}

/*********************************************************************************************/

BOOL LoadPrefs(BPTR fh)
{
    static struct FileInputPrefs    loadprefs;
    struct IFFHandle 	    	    *iff;
    BOOL    	    	    	    retval = FALSE;
    
    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR)fh;
        if (fh != NULL)
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

                                CopyMem(loadprefs.ip_Keymap, inputprefs.ip_Keymap, sizeof(loadprefs.ip_Keymap));
                                inputprefs.ip_PointerTicks         = ARRAY_TO_WORD(loadprefs.ip_PointerTicks);
                                inputprefs.ip_DoubleClick.tv_secs  = ARRAY_TO_LONG(loadprefs.ip_DoubleClick_secs);
                                inputprefs.ip_DoubleClick.tv_micro = ARRAY_TO_LONG(loadprefs.ip_DoubleClick_micro);
                                inputprefs.ip_KeyRptDelay.tv_secs  = ARRAY_TO_LONG(loadprefs.ip_KeyRptDelay_secs);
                                inputprefs.ip_KeyRptDelay.tv_micro = ARRAY_TO_LONG(loadprefs.ip_KeyRptDelay_micro);
                                inputprefs.ip_KeyRptSpeed.tv_secs  = ARRAY_TO_LONG(loadprefs.ip_KeyRptSpeed_secs);
                                inputprefs.ip_KeyRptSpeed.tv_micro = ARRAY_TO_LONG(loadprefs.ip_KeyRptSpeed_micro);
                                inputprefs.ip_MouseAccel    	   = ARRAY_TO_WORD(loadprefs.ip_MouseAccel);

                                D(bug("LoadPrefs: Everything okay :-)\n"));
				
                                retval = TRUE;
                            }
                        }
			
                    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		    
                } /* if (!StopChunk(iff, ID_PREF, ID_INPT)) */
		
                CloseIFF(iff);
				
            } /* if (!OpenIFF(iff, IFFF_READ)) */
	    
        } /* if (fh != NULL) */
	
        FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
        return retval;
    }

/*********************************************************************************************/

BOOL SavePrefs(BPTR fh)
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
	
    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR)fh;
    	if (iff->iff_Stream)
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
	    
	    
	} /* if (iff->iff_Stream)) */
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
//     if (!retval && delete_if_error)
//     {
//     	DeleteFile(filename);
//     }
    
    return retval;    
}

/*********************************************************************************************/

BOOL DefaultPrefs(void)
{
    strcpy(inputprefs.ip_Keymap, "amiga_usa0");
    inputprefs.ip_PointerTicks         = 1;
    inputprefs.ip_DoubleClick.tv_secs  = 0;
    inputprefs.ip_DoubleClick.tv_micro = 500000;
    inputprefs.ip_KeyRptDelay.tv_secs  = 0;
    inputprefs.ip_KeyRptDelay.tv_micro = 500000;
    inputprefs.ip_KeyRptSpeed.tv_secs  = 0;
    inputprefs.ip_KeyRptSpeed.tv_micro = 40000;
    inputprefs.ip_MouseAccel           = 1;
    
    return TRUE;
}

/*********************************************************************************************/

void CopyPrefs(struct InputPrefs *s, struct InputPrefs *d)
{
    CopyMem(s, d, sizeof(struct InputPrefs));
}

void RestorePrefs(void)
{
    CopyPrefs(&restore_prefs, &inputprefs);
}

/*********************************************************************************************/

void update_inputdev(void)
{
    if (InputIO)
    {
        if (InputIO->tr_node.io_Device)
        {
            InputIO->tr_node.io_Command = IND_SETPERIOD;
            InputIO->tr_time = inputprefs.ip_KeyRptSpeed;
            DoIO(&InputIO->tr_node);

            InputIO->tr_node.io_Command = IND_SETTHRESH;
            InputIO->tr_time = inputprefs.ip_KeyRptDelay;
            DoIO(&InputIO->tr_node);

            inputdev_changed = TRUE;
            
        }
    }
}

void try_setting_mousespeed(void)
{
    struct Preferences p;
    
    GetPrefs(&p, sizeof(p));
    p.PointerTicks = inputprefs.ip_PointerTicks;
    p.DoubleClick  = inputprefs.ip_DoubleClick;
    p.KeyRptDelay  = inputprefs.ip_KeyRptDelay;
    p.KeyRptSpeed  = inputprefs.ip_KeyRptSpeed;
    if (inputprefs.ip_MouseAccel)
    {
        p.EnableCLI |= MOUSE_ACCEL;
    }
    else
    {
        p.EnableCLI &= ~MOUSE_ACCEL;
    }
    
    SetPrefs(&p, sizeof(p), FALSE);
}

void try_setting_test_keymap(void)
{
    struct KeyMapResource *KeyMapResource;
    struct Library        *KeymapBase;
    struct KeyMapNode	  *kmn = NULL;
    struct Node     	  *node;
    BPTR    	    	   lock, seg, olddir, oldseg = 0;

    if ((KeyMapResource = OpenResource("keymap.resource")))
    {
        Forbid();
	
        ForeachNode(&KeyMapResource->kr_List, node)
        {
            if (!stricmp(inputprefs.ip_Keymap, node->ln_Name))
            {
                kmn = (struct KeyMapNode *)node;
                break;
            }
        }
	
        Permit();

    }

    if (!kmn)
    {
        lock = Lock("DEVS:Keymaps", SHARED_LOCK);

        if (lock)
        {
            olddir = CurrentDir(lock);

            if ((seg = LoadSeg(inputprefs.ip_Keymap)))
            {
                kmn = (struct KeyMapNode *) (((UBYTE *)BADDR(seg)) + sizeof(APTR));
                oldseg = testkeymap_seg;
                testkeymap_seg = seg;

            }
	    
            CurrentDir(olddir);
            UnLock(lock);
        }
    }

    if (kmn)
    {
        KeymapBase = OpenLibrary("keymap.library", 0);
        if (KeymapBase)
        {
            SetKeyMapDefault((struct KeyMap *)&kmn->kn_KeyMap);
            CloseLibrary(KeymapBase);
        }
    }
    if (oldseg) UnLoadSeg(oldseg);
    
}

void kbd_cleanup(void)
{
    if (inputdev_changed)
    {
        InputIO->tr_node.io_Command = IND_SETPERIOD;
        InputIO->tr_time = restore_prefs.ip_KeyRptSpeed;
        DoIO(&InputIO->tr_node);
	
        InputIO->tr_node.io_Command = IND_SETTHRESH;
        InputIO->tr_time = restore_prefs.ip_KeyRptDelay;
        DoIO(&InputIO->tr_node);
        inputdev_changed = FALSE;
    }
    
    if (testkeymap_seg)
    {
        UnLoadSeg(testkeymap_seg);
    }
}

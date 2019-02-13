/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change your input preferences
    Lang: English
*/

/*********************************************************************************************/

#define SHOWFLAGS 1  /* Set to 1 to show Flags in the keylist */

#include <libraries/kms.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <proto/utility.h>

#define DEBUG 0
#include <aros/debug.h>

#include "prefs.h"
#include "misc.h"

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:SYS/input.prefs"
#define PREFS_PATH_ENV    "ENV:SYS/input.prefs"

/*********************************************************************************************/

APTR                mempool;
struct InputPrefs   inputprefs;
struct KMSPrefs     kmsprefs;
struct List         keymap_list;

/*********************************************************************************************/

static const struct nameexp layout_expansion_table[] =
{
    {"usa"  , "American"	    , "Countries/United_States" },
    {"usa0" , "American"	    , "Countries/United_States" },
    {"us"   , "American"	    , "Countries/United_States" },
    {"usx"  , "American"	    , "Countries/United_States" },
    {"col"  , "Colemak (US)"	    , "Countries/United_States" },
    {"d"    , "Deutsch"             , "Countries/Germany"       },
    {"b"    , "Belge"               , "Countries/Belgium"       },
    {"by"   , "Belarussian"         , "Countries/Belarus"       },
    {"br"   , "Brasileiro"          , "Countries/Brazil"        },
    {"gb"   , "British"             , "Countries/United_Kingdom"},
    {"gbx"  , "British Extended"    , "Countries/United_Kingdom"},
    {"bg"   , "Bulgarian"           , "Countries/Bulgaria"      },
    {"cdn"  , "Canadien Français"   , "Countries/Quebec"        },
    {"cz"   , "Czech"               , "Countries/Czech_Republic"},
    {"dk"   , "Dansk"               , "Countries/Denmark"       },
    {"nl"   , "Dutch"               , "Countries/Netherlands"   },
    {"dvx"  , "Dvorak"              , "Misc/Unknown"            },
    {"usa2" , "Dvorak"              , "Misc/Unknown"            },
    {"dvl"  , "Dvorak Left-handed"  , "Misc/Unknown"            },
    {"dvr"  , "Dvorak Right-handed" , "Misc/Unknown"            },
    {"irl"  , "English Ireland"     , "Countries/Ireland"       },
    {"e"    , "Español"             , "Countries/Spain"         },
    {"sp"   , "Español no deadkeys" , "Countries/Spain"         },
    {"est"  , "Eesti"               , "Countries/Estonia"       },
    {"fin"  , "Finnish"             , "Countries/Finland"       },
    {"f"    , "Français"            , "Countries/France"        },
    {"il"   , "Hebrew"              , "Countries/Israel"        },
    {"gr"   , "Hellenic"            , "Countries/Greece"        },
    {"hr"   , "Hrvatski"            , "Countries/Croatia"       },
    {"is"   , "Íslenska"            , "Countries/Iceland"       },
    {"i"    , "Italiana"            , "Countries/Italy"         },
    {"la"   , "Latin American"      , "Misc/Unknown"            },
    {"lv"   , "Latvijas"            , "Countries/Latvia"        },
    {"lt"   , "Lietuvos"            , "Countries/Lithuania"     },
    {"h"    , "Magyar"              , "Countries/Hungary"       },
    {"n"    , "Norsk"               , "Countries/Norway"        },
    {"pl"   , "Polski"              , "Countries/Poland"        },
    {"p"    , "Português"           , "Countries/Portugal"      },
    {"ro"   , "Românesc"            , "Countries/Romania"       },
    {"rus"  , "Russian"             , "Countries/Russia"        },
    {"ch1"  , "Schweizer"           , "Countries/Switzerland"   },
    {"al"   , "Shqip"               , "Countries/Albania"       },
    {"sk"   , "Slovak"              , "Countries/Slovakia"      },
    {"ch2"  , "Suisse"              , "Countries/Switzerland"   },
    {"s"    , "Svenskt"             , "Countries/Sweden"        },
    {"tr"   , "Türkçe"              , "Countries/Turkey"        },
    {"ua"   , "Ukranian"            , "Countries/Ukraine"       },
    {NULL   , NULL                  , NULL                      }
};

static const struct typeexp type_expansion_table[] = {
    {"amiga", "Amiga" },
    {"pc104", "PC 104"},
    {"pc105", "PC 105"},
    {"sun"  , "Sun"   },
    {NULL   , NULL    }
};

/*********************************************************************************************/

static BOOL Prefs_Load(STRPTR from)
{
    BOOL retval = FALSE;

    BPTR fh = Open(from, MODE_OLDFILE);
    if (fh)
    {
        retval = Prefs_ImportFH(fh);
        Close(fh);
    }

    return retval;
}

/*********************************************************************************************/

static void ExpandName(struct ListviewEntry *entry)
{
    char *sp;
    char *type = NULL;
    const struct nameexp *exp;

    strcpy(entry->layoutname, entry->realname);

    sp = strchr(entry->realname, '_');
    if (sp)
    {
    	/*
    	 * We have several variants of US keyboard, so
    	 * we append type name.
    	 */
        if ((sp[1] == 'u' && sp[2] == 's'))
        {
             const struct typeexp *te;

             *sp = 0;   
	     for (te = type_expansion_table; te->shortname; te++)
	     {
	    	if (!strcmp(entry->realname, te->shortname))
	    	{
	    	    type = te->longname;
	    	    break;
	    	}
	    }
            *sp = '_';
        }
        sp++;
    }
    else
	sp = entry->realname;

    for (exp = layout_expansion_table; exp->shortname; exp++)
    {
        if (stricmp(exp->shortname, sp) == 0)
        {
            if (type)
            	snprintf(entry->layoutname, sizeof(entry->layoutname), "%s (%s)", exp->longname, type);
            else
	        strcpy(entry->layoutname, exp->longname);

            if (exp->flag != NULL)
            {
#if SHOWFLAGS == 1
        	sprintf(entry->displayflag, "\033I[5:Locale:Flags/%s]", exp->flag);
#else
        	entry->displayflag[0] = '\0';
#endif
	    }
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

void Prefs_ScanDirectory(STRPTR pattern, struct List *list, LONG entrysize)
{
    struct AnchorPath       ap;
    struct ListviewEntry    *entry, *entry2;
    struct List             templist;
    STRPTR                  sp;
    LONG                    error;

    memset(&ap, 0, sizeof(ap));
    NewList(&templist);

    /*
     * Add default keymap.
     * CHECKME: possibly too hacky. May be we should verify its presence in keymap.resource?
     * How is it done in classic AmigaOS(tm) ?
     */
    entry = AllocPooled(mempool, entrysize);
    if (entry)
    {
        strcpy(entry->layoutname, "American (Default)");
    	strcpy(entry->realname  , DEFAULT_KEYMAP);
#if SHOWFLAGS == 1
    	strcpy(entry->displayflag, "\033I[5:Locale:Flags/Countries/United_States]");
#else
        entry->displayflag[0] = '\0';
#endif

    	entry->node.ln_Name = entry->layoutname;
    	AddTail(&templist, &entry->node);    	
    }

    error = MatchFirst(pattern, &ap);
    while((error == 0))
    {
        if (ap.ap_Info.fib_DirEntryType < 0)
        {
            entry = (struct ListviewEntry *)AllocPooled((APTR)mempool, entrysize);
            if (entry)
            {
                entry->node.ln_Name = entry->layoutname;
                CopyMem(ap.ap_Info.fib_FileName, entry->realname, sizeof(entry->realname));
                entry->realname[sizeof(entry->realname) - 1] = '\0';

                sp = strchr(entry->realname, '_');
                if (sp)
                   sp++;
                else
                   sp = entry->realname;
                strcpy(entry->layoutname, sp);

                ExpandName(entry);
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
}

/*********************************************************************************************/

static LONG stopchunks[] =
{
    ID_PREF, ID_INPT,
    ID_PREF, ID_KMSW
};

BOOL Prefs_ImportFH(BPTR fh)
{
    struct FileInputPrefs   loadprefs;
    struct FileKMSPrefs	    loadkmsprefs;
    struct IFFHandle       *iff;
    ULONG		    size;
    BOOL                    retval = FALSE;

    memset(&loadprefs, 0, sizeof(loadprefs));

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR)fh;
        if (fh != BNULL)
        {
            D(Printf("LoadPrefs: stream opened.\n"));

            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_READ))
            {
                D(Printf("LoadPrefs: OpenIFF okay.\n"));

                if (!StopChunks(iff, stopchunks, 2))
                {
                    D(Printf("LoadPrefs: StopChunk okay.\n"));
		    retval = TRUE;

                    while (!ParseIFF(iff, IFFPARSE_SCAN))
                    {
                        struct ContextNode *cn;

                        D(Printf("LoadPrefs: ParseIFF okay.\n"));

                        cn = CurrentChunk(iff);
			size = cn->cn_Size;

			switch (cn->cn_ID)
			{
			case ID_INPT:
			    D(Printf("LoadPrefs: INPT chunk\n"));

			    if (size > sizeof(struct FileInputPrefs))
				size = sizeof(struct FileInputPrefs);

                            if (ReadChunkBytes(iff, &loadprefs, size) == size)
			    {
				D(Printf("LoadPrefs: Reading chunk successful.\n"));

				CopyMem(loadprefs.ip_Keymap, inputprefs.ip_Keymap, sizeof(loadprefs.ip_Keymap));
				inputprefs.ip_PointerTicks         = ARRAY_TO_WORD(loadprefs.ip_PointerTicks);
				inputprefs.ip_DoubleClick.tv_secs  = ARRAY_TO_LONG(loadprefs.ip_DoubleClick_secs);
				inputprefs.ip_DoubleClick.tv_micro = ARRAY_TO_LONG(loadprefs.ip_DoubleClick_micro);
				inputprefs.ip_KeyRptDelay.tv_secs  = ARRAY_TO_LONG(loadprefs.ip_KeyRptDelay_secs);
				inputprefs.ip_KeyRptDelay.tv_micro = ARRAY_TO_LONG(loadprefs.ip_KeyRptDelay_micro);
				inputprefs.ip_KeyRptSpeed.tv_secs  = ARRAY_TO_LONG(loadprefs.ip_KeyRptSpeed_secs);
				inputprefs.ip_KeyRptSpeed.tv_micro = ARRAY_TO_LONG(loadprefs.ip_KeyRptSpeed_micro);
				inputprefs.ip_MouseAccel           = ARRAY_TO_WORD(loadprefs.ip_MouseAccel);
				inputprefs.ip_ClassicKeyboard      = ARRAY_TO_LONG(loadprefs.ip_ClassicKeyboard);
				CopyMem(loadprefs.ip_KeymapName, inputprefs.ip_KeymapName, sizeof(loadprefs.ip_KeymapName));
				inputprefs.ip_SwitchMouseButtons   = loadprefs.ip_SwitchMouseButtons[3];
			    
				D(Printf("LoadPrefs: SwitchMouseButtons: %ld\n", inputprefs.ip_SwitchMouseButtons));
			    }
			    else
				retval = FALSE;
			    break;

			case ID_KMSW:
			    D(Printf("LoadPrefs: KMSW chunk\n"));

			    if (size > sizeof(struct FileKMSPrefs))
				size = sizeof(struct FileKMSPrefs);

                            if (ReadChunkBytes(iff, &loadkmsprefs, size) == size)
			    {
				D(Printf("LoadPrefs: Reading chunk successful.\n"));

				kmsprefs.kms_Enabled    = loadkmsprefs.kms_Enabled;
				kmsprefs.kms_Reserved   = loadkmsprefs.kms_Reserved;
				kmsprefs.kms_SwitchQual = ARRAY_TO_WORD(loadkmsprefs.kms_SwitchQual);
				kmsprefs.kms_SwitchCode = ARRAY_TO_WORD(loadkmsprefs.kms_SwitchCode);
				CopyMem(loadkmsprefs.kms_AltKeymap, kmsprefs.kms_AltKeymap, sizeof(kmsprefs.kms_AltKeymap));
			    }
			    else
				retval = FALSE;
			    break;
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

BOOL Prefs_ExportFH(BPTR fh)
{
    struct FileInputPrefs   saveprefs;
    struct FileKMSPrefs	    savekmsprefs;
    struct IFFHandle       *iff;
    BOOL                    retval = FALSE;
#if 0 /* unused */
    BOOL                    delete_if_error = FALSE;
#endif

    D(Printf("SavePrefs: SwitchMouseButtons: %ld\n", inputprefs.ip_SwitchMouseButtons));

    CopyMem(inputprefs.ip_Keymap, saveprefs.ip_Keymap, sizeof(saveprefs.ip_Keymap));
    WORD_TO_ARRAY(inputprefs.ip_PointerTicks, saveprefs.ip_PointerTicks);
    LONG_TO_ARRAY(inputprefs.ip_DoubleClick.tv_secs , saveprefs.ip_DoubleClick_secs);
    LONG_TO_ARRAY(inputprefs.ip_DoubleClick.tv_micro, saveprefs.ip_DoubleClick_micro);
    LONG_TO_ARRAY(inputprefs.ip_KeyRptDelay.tv_secs , saveprefs.ip_KeyRptDelay_secs);
    LONG_TO_ARRAY(inputprefs.ip_KeyRptDelay.tv_micro, saveprefs.ip_KeyRptDelay_micro);
    LONG_TO_ARRAY(inputprefs.ip_KeyRptSpeed.tv_secs , saveprefs.ip_KeyRptSpeed_secs);
    LONG_TO_ARRAY(inputprefs.ip_KeyRptSpeed.tv_micro, saveprefs.ip_KeyRptSpeed_micro);
    WORD_TO_ARRAY(inputprefs.ip_MouseAccel, saveprefs.ip_MouseAccel);
    LONG_TO_ARRAY(inputprefs.ip_ClassicKeyboard, saveprefs.ip_ClassicKeyboard);
    CopyMem(inputprefs.ip_KeymapName, saveprefs.ip_KeymapName, sizeof(saveprefs.ip_KeymapName));
    saveprefs.ip_SwitchMouseButtons[0] = 0;
    saveprefs.ip_SwitchMouseButtons[1] = 0;
    saveprefs.ip_SwitchMouseButtons[2] = 0;
    saveprefs.ip_SwitchMouseButtons[3] = inputprefs.ip_SwitchMouseButtons;

    savekmsprefs.kms_Enabled  = kmsprefs.kms_Enabled;
    savekmsprefs.kms_Reserved = kmsprefs.kms_Reserved;
    WORD_TO_ARRAY(kmsprefs.kms_SwitchQual, savekmsprefs.kms_SwitchQual);
    WORD_TO_ARRAY(kmsprefs.kms_SwitchCode, savekmsprefs.kms_SwitchCode);
    CopyMem(kmsprefs.kms_AltKeymap, savekmsprefs.kms_AltKeymap, sizeof(savekmsprefs.kms_AltKeymap));

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR)fh;
        if (iff->iff_Stream)
        {
            D(Printf("SavePrefs: stream opened.\n"));

#if 0 /* unused */
            delete_if_error = TRUE;
#endif

            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_WRITE))
            {
                D(Printf("SavePrefs: OpenIFF okay.\n"));

                if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
                {
                    D(Printf("SavePrefs: PushChunk(FORM) okay.\n"));

                    if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
                    {
                        struct FilePrefHeader head;

                        D(Printf("SavePrefs: PushChunk(PRHD) okay.\n"));

                        head.ph_Version  = 0; // FIXME: should be PHV_CURRENT, but see <prefs/prefhdr.h>
                        head.ph_Type     = 0;
                        head.ph_Flags[0] =
                        head.ph_Flags[1] =
                        head.ph_Flags[2] =
                        head.ph_Flags[3] = 0;

                        if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
                        {
                            D(Printf("SavePrefs: WriteChunkBytes(PRHD) okay.\n"));

                            PopChunk(iff);

                            if (!PushChunk(iff, ID_PREF, ID_INPT, sizeof(saveprefs)))
                            {
                                D(Printf("SavePrefs: PushChunk(INPT) okay.\n"));

                                if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
                                {
                                    D(Printf("SavePrefs: WriteChunkBytes(INPT) okay.\n"));

                                    retval = TRUE;
                                }

                                PopChunk(iff);

                            } /* if (!PushChunk(iff, ID_PREF, ID_INPT, sizeof(saveprefs))) */

                            if (!PushChunk(iff, ID_PREF, ID_KMSW, sizeof(savekmsprefs)))
                            {
                                if (WriteChunkBytes(iff, &savekmsprefs, sizeof(savekmsprefs)) == sizeof(savekmsprefs))
                                    retval = retval && TRUE;

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

#if 0 /* unused */
     if (!retval && delete_if_error)
     {
          DeleteFile(filename);
     }
#endif

    return retval;
}

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save)
{
    BPTR fh;

    if (from)
    {
        if (!Prefs_Load(from))
        {
            ShowMessage("Can't read from input file");
            return FALSE;
        }
    }
    else
    {
        if (!Prefs_Load(PREFS_PATH_ENV))
        {
            if (!Prefs_Load(PREFS_PATH_ENVARC))
            {
                ShowMessage
                (
                    "Can't read from file " PREFS_PATH_ENVARC
                    ".\nUsing default values."
                );
                Prefs_Default();
            }
        }
    }

    if (use || save)
    {
        fh = Open(PREFS_PATH_ENV, MODE_NEWFILE);
        if (fh)
        {
            Prefs_ExportFH(fh);
            Close(fh);
        }
        else
        {
            ShowMessage("Can't open " PREFS_PATH_ENV " for writing.");
        }
    }
    if (save)
    {
        fh = Open(PREFS_PATH_ENVARC, MODE_NEWFILE);
        if (fh)
        {
            Prefs_ExportFH(fh);
            Close(fh);
        }
        else
        {
            ShowMessage("Can't open " PREFS_PATH_ENVARC " for writing.");
        }
    }
    return TRUE;
}

/*********************************************************************************************/

BOOL Prefs_Default(void)
{
    strcpy(inputprefs.ip_Keymap, DEFAULT_KEYMAP);
    inputprefs.ip_PointerTicks         = 1;
    inputprefs.ip_DoubleClick.tv_secs  = 0;
    inputprefs.ip_DoubleClick.tv_micro = 500000;
    inputprefs.ip_KeyRptDelay.tv_secs  = 0;
    inputprefs.ip_KeyRptDelay.tv_micro = 500000;
    inputprefs.ip_KeyRptSpeed.tv_secs  = 0;
    inputprefs.ip_KeyRptSpeed.tv_micro = 40000;
    inputprefs.ip_MouseAccel           = 1;
    inputprefs.ip_ClassicKeyboard      = 0;
    strcpy(inputprefs.ip_KeymapName, DEFAULT_KEYMAP);
    inputprefs.ip_SwitchMouseButtons   = FALSE;

    kmsprefs.kms_Enabled      = FALSE;
    kmsprefs.kms_SwitchQual   = KMS_QUAL_DISABLE;
    kmsprefs.kms_SwitchCode   = KMS_CODE_NOKEY;
    kmsprefs.kms_AltKeymap[0] = 0;

    return TRUE;
}

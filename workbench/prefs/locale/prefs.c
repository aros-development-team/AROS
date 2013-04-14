/*
   Copyright © 1995-2013, The AROS Development Team. All rights reserved.
   $Id$
*/

/*********************************************************************************************/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <proto/alib.h>
#include <proto/iffparse.h>

#include <aros/macros.h>

#include <prefs/prefhdr.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "prefs.h"
#include "misc.h"

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:SYS/locale.prefs"
#define PREFS_PATH_ENV    "ENV:SYS/locale.prefs"

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

struct LocalePrefs  localeprefs;
char                character_set[CHARACTER_SET_LEN];
char                restore_charset[CHARACTER_SET_LEN];
struct List         region_list;
struct List         language_list;
struct List         pref_language_list;

/*********************************************************************************************/

static APTR         mempool;

const char          *flagpathstr = "\033I[5:Locale:Flags/";

/*********************************************************************************************/

STATIC VOID SortInNode(struct List *list, struct Node *node)
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

/*********************************************************************************************/

char *GetAROSRegionAttribs(struct AnchorPath *ap, char **regionNamePtr)
{
    char                *lockFlag = NULL;
    struct IFFHandle    *iff;
    struct ContextNode  *cn;
    LONG                parse_mode = IFFPARSE_SCAN, error;

    if ((iff = AllocIFF()))
    {
        if ((iff->iff_Stream = (IPTR)Open(ap->ap_Info.fib_FileName, MODE_OLDFILE)))
        {
            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_READ))
            {
                if (!StopChunk(iff, ID_PREF, ID_PRHD))
                {
                    do
                    {
                        if ((error = ParseIFF(iff, parse_mode)) == 0)
                        {
                            parse_mode = IFFPARSE_STEP; 

                            cn = CurrentChunk(iff);
                            D(bug("[LocalePrefs] GetAROSRegionAttribs: Chunk ID %08x. %d bytes\n", cn->cn_ID, cn->cn_Size));

                            if (cn->cn_ID == MAKE_ID('N','N','A','M'))
                            {
                                FreeVecPooled(mempool, *regionNamePtr);
                                *regionNamePtr = AllocVecPooled(mempool, cn->cn_Size);
                                ReadChunkBytes(iff, *regionNamePtr, cn->cn_Size);
                                D(bug("[LocalePrefs] GetAROSRegionAttribs: NativeNames '%s'\n", *regionNamePtr));
                            }

                            if (cn->cn_ID == MAKE_ID('F','L','A','G'))
                            {
                                lockFlag = AllocVecPooled(mempool, cn->cn_Size + 18 + 1);
                                sprintf(lockFlag, flagpathstr);
                                ReadChunkBytes(iff, lockFlag + 18, cn->cn_Size);
                                lockFlag[cn->cn_Size + 17] = ']';
                                D(bug("[LocalePrefs] GetAROSRegionAttribs: Flag '%s'\n", lockFlag));
                            }
                        }
                    } while ((error != IFFERR_EOF) && (error != IFFERR_NOTIFF));
                }
                CloseIFF(iff);
            }
            Close(iff->iff_Stream);
        }
        FreeIFF(iff);
    }

    return lockFlag;
}

/*********************************************************************************************/

STATIC VOID ScanDirectory(char *pattern, struct List *list, LONG entrysize)
{
    struct AnchorPath           ap;
    struct ListviewEntry        *entry;
    BPTR                        curdir = BNULL;
    char                        *sp;
    LONG                        error;

    memset(&ap, 0, sizeof(ap));

    if ((error = MatchFirst(pattern, &ap)) == 0)
        curdir = CurrentDir(ap.ap_Current->an_Lock);

    while((error == 0))
    {
        if (ap.ap_Info.fib_DirEntryType < 0)
        {
            entry = (struct ListviewEntry *)AllocPooled(mempool, entrysize);
            if (entry)
            {
                entry->node.ln_Name = AllocVecPooled(mempool, strlen(ap.ap_Info.fib_FileName));
                strcpy(entry->node.ln_Name, ap.ap_Info.fib_FileName);

                entry->node.ln_Name[0] = ToUpper(entry->node.ln_Name[0]);
                if ((sp = strchr(entry->node.ln_Name, '.')) != NULL)
                    sp[0] = '\0';

                strcpy(entry->realname, entry->node.ln_Name);

                if (entrysize == sizeof(struct RegionEntry))
                {
                    D(bug("[LocalePrefs] ScanDir: Checking for FLAG chunk\n"));
                    if (!(entry->displayflag = GetAROSRegionAttribs(&ap, &entry->node.ln_Name)))
                    {
                        entry->displayflag = AllocVec(strlen(entry->realname) + strlen(flagpathstr) + 12, MEMF_CLEAR);
                        sprintf(entry->displayflag, "%sCountries/%s]", flagpathstr, entry->realname);
                    }
                }
                else if (entrysize == sizeof(struct LanguageEntry))
                {
                    // TODO: handle translating english language name -> native name.
                }

                sp = entry->node.ln_Name;
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
    if (curdir != BNULL)
        CurrentDir(curdir);
    MatchEnd(&ap);
}

/*********************************************************************************************/

#if !AROS_BIG_ENDIAN
STATIC VOID FixCountryEndianess(struct CountryPrefs *region)
{
    region->cp_Reserved[0] = AROS_BE2LONG(region->cp_Reserved[0]);
    region->cp_Reserved[1] = AROS_BE2LONG(region->cp_Reserved[1]);
    region->cp_Reserved[2] = AROS_BE2LONG(region->cp_Reserved[2]);
    region->cp_Reserved[3] = AROS_BE2LONG(region->cp_Reserved[3]);
    region->cp_CountryCode = AROS_BE2LONG(region->cp_CountryCode);
}
#endif

/*********************************************************************************************/

#if !AROS_BIG_ENDIAN
STATIC VOID FixLocaleEndianess(struct LocalePrefs *localeprefs)
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

BOOL Prefs_LoadRegion(STRPTR name, struct CountryPrefs *region)
{
    static struct CountryPrefs  loadregion;
    struct IFFHandle            *iff;
    struct ContextNode          *cn;
    LONG                        parse_mode = IFFPARSE_SCAN, error;
    char                        fullname[100];
    BOOL                        retval = FALSE;

    strcpy(fullname, "LOCALE:Countries");
    AddPart(fullname, name, 100);
    strcat(fullname, ".country");

    D(bug("[LocalePrefs] LoadRegion: Trying to open \"%s\"\n", fullname));

    if ((iff = AllocIFF()))
    {
        if ((iff->iff_Stream = (IPTR)Open(fullname, MODE_OLDFILE)))
        {
            D(bug("[LocalePrefs] LoadRegion: stream opened.\n"));

            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_READ))
            {
                D(bug("[LocalePrefs] LoadRegion: OpenIFF okay.\n"));

                if (!StopChunk(iff, ID_PREF, ID_PRHD))
                {
                    D(bug("[LocalePrefs] LoadRegion: StopChunk okay.\n"));

                    do
                    {
                        if ((error = ParseIFF(iff, parse_mode)) == 0)
                        {
                            parse_mode = IFFPARSE_STEP; 
                            
                            D(bug("[LocalePrefs] LoadRegion: ParseIFF okay.\n"));

                            cn = CurrentChunk(iff);

                            D(bug("[LocalePrefs] LoadRegion: Chunk ID %08x.\n", cn->cn_ID));

                            if ((cn->cn_ID == ID_CTRY) && (cn->cn_Size == sizeof(struct CountryPrefs)))
                            {
                                D(bug("[LocalePrefs] LoadRegion: Chunk ID_CTRY (size okay).\n"));

                                if (ReadChunkBytes(iff, &loadregion, sizeof(struct CountryPrefs)) == sizeof(struct CountryPrefs))
                                {
                                    D(bug("[LocalePrefs] LoadRegion: Reading chunk successful.\n"));

                                    *region = loadregion;

#if !AROS_BIG_ENDIAN
                                    FixCountryEndianess(region);
#endif

                                    D(bug("[LocalePrefs] LoadRegion: Everything okay :-)\n"));

                                    retval = TRUE;
                                    error = IFFERR_EOF;
                                }
                            }
                        } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
                    } while ((error != IFFERR_EOF) && (error != IFFERR_NOTIFF));
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

BOOL Prefs_ImportFH(BPTR fh)
{
    static struct LocalePrefs   loadprefs;
    struct IFFHandle           *iff;
    BOOL                        retval = FALSE;

    D(bug("[LocalePrefs] LoadPrefsFH\n"));

    if ((iff = AllocIFF()))
    {
        if ((iff->iff_Stream = (IPTR) fh))
        {
            D(bug("[LocalePrefs] LoadPrefsFH: stream is ok.\n"));

            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_READ))
            {
                D(bug("[LocalePrefs] LoadPrefsFH: OpenIFF okay.\n"));

                if (!StopChunk(iff, ID_PREF, ID_LCLE))
                {
                    D(bug("[LocalePrefs] LoadPrefsFH: StopChunk okay.\n"));

                    if (!ParseIFF(iff, IFFPARSE_SCAN))
                    {
                        struct ContextNode *cn;

                        D(bug("[LocalePrefs] LoadPrefsFH: ParseIFF okay.\n"));

                        cn = CurrentChunk(iff);

                        if (cn->cn_Size == sizeof(struct LocalePrefs))
                        {
                            D(bug("[LocalePrefs] LoadPrefsFH: ID_LCLE chunk size okay.\n"));

                            if (ReadChunkBytes(iff, &loadprefs, sizeof(struct LocalePrefs)) == sizeof(struct LocalePrefs))
                            {
                                D(bug("[LocalePrefs] LoadPrefsFH: Reading chunk successful.\n"));

                                localeprefs = loadprefs;

#if !AROS_BIG_ENDIAN
                                FixLocaleEndianess(&localeprefs);
                                FixCountryEndianess(&localeprefs.lp_CountryData);
#endif

                                D(bug("[LocalePrefs] LoadPrefsFH: Everything okay :-)\n"));

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

    D(bug("[LocalePrefs] CountryName: %s\n",localeprefs.lp_CountryName));
    int i=0;
    while(i<10 && localeprefs.lp_PreferredLanguages[i])
    {
        D(bug("[LocalePrefs] preferred %ld: %s\n",i,localeprefs.lp_PreferredLanguages[i]));
        i++;
    }
    D(bug("[LocalePrefs] lp_GMTOffset: %ld\n",localeprefs.lp_GMTOffset));
    return retval;
}


/*********************************************************************************************/

BOOL Prefs_ExportFH(BPTR fh)
{
    struct LocalePrefs  saveprefs;
    struct IFFHandle   *iff;
    BOOL                retval = FALSE;
#if 0 /* unused */
    BOOL                delete_if_error = FALSE;
#endif

    D(bug("[LocalePrefs] SavePrefsFH: fh: %lx\n", fh));

    saveprefs = localeprefs;

#if !AROS_BIG_ENDIAN
    FixLocaleEndianess(&saveprefs);
    FixCountryEndianess(&saveprefs.lp_CountryData);
#endif

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR) fh;
        D(bug("[LocalePrefs] SavePrefsFH: stream opened.\n"));

#if 0 /* unused */
        delete_if_error = TRUE;
#endif

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_WRITE))
        {
            D(bug("[LocalePrefs] SavePrefsFH: OpenIFF okay.\n"));

            if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
            {
                D(bug("[LocalePrefs] SavePrefsFH: PushChunk(FORM) okay.\n"));

                if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
                {
                    struct FilePrefHeader head;

                    D(bug("[LocalePrefs] SavePrefsFH: PushChunk(PRHD) okay.\n"));

                    head.ph_Version  = PHV_CURRENT;
                    head.ph_Type     = 0;
                    head.ph_Flags[0] =
                        head.ph_Flags[1] =
                        head.ph_Flags[2] =
                        head.ph_Flags[3] = 0;

                    if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
                    {
                        D(bug("[LocalePrefs] SavePrefsFH: WriteChunkBytes(PRHD) okay.\n"));

                        PopChunk(iff);

                        if (!PushChunk(iff, ID_PREF, ID_LCLE, sizeof(struct LocalePrefs)))
                        {
                            D(bug("[LocalePrefs] SavePrefsFH: PushChunk(LCLE) okay.\n"));

                            if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
                            {
                                D(bug("[LocalePrefs] SavePrefsFH: WriteChunkBytes(SERL) okay.\n"));
                                D(bug("[LocalePrefs] SavePrefsFH: Everything okay :-)\n"));

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

#if 0 /* unused */
    if (!retval && delete_if_error)
    {
        DeleteFile(filename);
    }
#endif

    return retval;
}

/*********************************************************************************************/

BOOL Prefs_SaveCharset(BOOL envarc)
{
    LONG flags = GVF_GLOBAL_ONLY;

    D(bug("[LocalePrefs] SaveCharset(%ld)\n", envarc));

    if (envarc)
        flags |= GVF_SAVE_VAR;
    /* We don't check results of the following actions because they may fail if ENVARC: is read-only (CD-ROM) */
    if (character_set[0])
        SetVar("CHARSET", character_set, -1, flags);
    else
        DeleteVar("CHARSET", flags);

    return TRUE;
}

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
        Prefs_LoadRegion(localeprefs.lp_CountryName, &localeprefs.lp_CountryData);
        fh = Open(PREFS_PATH_ENV, MODE_NEWFILE);
        if (fh)
        {
            Prefs_ExportFH(fh);
            Close(fh);
        }
        else
        {
            ShowMessage("Cant' open " PREFS_PATH_ENV " for writing.");
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
            ShowMessage("Cant' open " PREFS_PATH_ENVARC " for writing.");
        }
    }

    return TRUE;
}

/*********************************************************************************************/

BOOL Prefs_Initialize(VOID)
{
    D(bug("[LocalePrefs] InitPrefs\n"));

    struct LanguageEntry *entry;

    mempool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 2048, 2048);
    if (!mempool)
    {
        ShowMessage("Out of memory!");
        return FALSE;
    }

    NewList(&region_list);
    NewList(&language_list);
    NewList(&pref_language_list);

    ScanDirectory("LOCALE:Countries/~(#?.info)", &region_list, sizeof(struct RegionEntry));
    ScanDirectory("LOCALE:Languages/#?.language", &language_list, sizeof(struct LanguageEntry));

    /* English language is always available */

    if ((entry = AllocPooled(mempool, sizeof(struct LanguageEntry))))
    {
        entry->lve.node.ln_Name = AllocVecPooled(mempool, 8);
        strcpy( entry->lve.node.ln_Name, "English");
        strcpy( entry->lve.realname, "English");

        SortInNode(&language_list, &entry->lve.node);
    }

    character_set[0] = 0;
    GetVar("CHARSET", character_set, sizeof(character_set), 0);
    D(bug("[LocalePrefs] System character set: %s\n", character_set));

    return TRUE;
}

/*********************************************************************************************/

VOID Prefs_Deinitialize(VOID)
{
    D(bug("[LocalePrefs] CleanupPrefs\n"));
    if (mempool)
    {
        DeletePool(mempool);
        mempool = NULL;
    }
}

/*********************************************************************************************/

BOOL Prefs_Default(VOID)
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

    if (Prefs_LoadRegion((STRPTR) "united_states", &localeprefs.lp_CountryData))
    {
        retval = TRUE;
    }

    character_set[0] = 0;

    return retval;
}

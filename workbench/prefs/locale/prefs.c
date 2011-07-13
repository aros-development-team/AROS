/*
   Copyright © 1995-2011, The AROS Development Team. All rights reserved.
   $Id$
*/

/*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <aros/macros.h>

//#define DEBUG 1
#include <aros/debug.h>

#include <prefs/prefhdr.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <proto/alib.h>
#include <proto/iffparse.h>

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
struct List         country_list;
struct List         language_list;
struct List         pref_language_list;

/*********************************************************************************************/

static APTR         mempool;

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

STATIC VOID ScanDirectory(char *pattern, struct List *list, LONG entrysize)
{
    struct AnchorPath       ap;
    struct ListviewEntry   *entry;
    char                   *sp;
    LONG                    error;

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

    ForeachNode(list, entry)
    {
        sprintf(entry->displayflag, "\033I[5:Locale:Flags/Countries/%s]", entry->realname);
        D(Printf("Locale: country entry flag: %s\n", entry->realname));
    }

}

/*********************************************************************************************/

#if !AROS_BIG_ENDIAN
STATIC VOID FixCountryEndianess(struct CountryPrefs *country)
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

BOOL Prefs_LoadCountry(STRPTR name, struct CountryPrefs *country)
{
    static struct CountryPrefs  loadcountry;
    struct IFFHandle           *iff;
    char                        fullname[100];
    BOOL                        retval = FALSE;

    strcpy(fullname, "LOCALE:Countries");
    AddPart(fullname, name, 100);
    strcat(fullname, ".country");

    D(Printf("[locale prefs] LoadCountry: Trying to open \"%s\"\n", fullname));

    if ((iff = AllocIFF()))
    {
        if ((iff->iff_Stream = (IPTR)Open(fullname, MODE_OLDFILE)))
        {
            D(Printf("[locale prefs] LoadCountry: stream opened.\n"));

            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_READ))
            {
                D(Printf("[locale prefs] LoadCountry: OpenIFF okay.\n"));

                if (!StopChunk(iff, ID_PREF, ID_CTRY))
                {
                    D(Printf("[locale prefs] LoadCountry: StopChunk okay.\n"));

                    if (!ParseIFF(iff, IFFPARSE_SCAN))
                    {
                        struct ContextNode *cn;

                        D(Printf("[locale prefs] LoadCountry: ParseIFF okay.\n"));

                        cn = CurrentChunk(iff);

                        if (cn->cn_Size == sizeof(struct CountryPrefs))
                        {
                            D(Printf("[locale prefs] LoadCountry: ID_CTRY chunk size okay.\n"));

                            if (ReadChunkBytes(iff, &loadcountry, sizeof(struct CountryPrefs)) == sizeof(struct CountryPrefs))
                            {
                                D(Printf("[locale prefs] LoadCountry: Reading chunk successful.\n"));

                                *country = loadcountry;

#if !AROS_BIG_ENDIAN
                                FixCountryEndianess(country);
#endif

                                D(Printf("[locale prefs] LoadCountry: Everything okay :-)\n"));

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

BOOL Prefs_ImportFH(BPTR fh)
{
    static struct LocalePrefs   loadprefs;
    struct IFFHandle           *iff;
    BOOL                        retval = FALSE;

    D(Printf("[locale prefs] LoadPrefsFH\n"));

    if ((iff = AllocIFF()))
    {
        if ((iff->iff_Stream = (IPTR) fh))
        {
            D(Printf("[locale prefs] LoadPrefsFH: stream is ok.\n"));

            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_READ))
            {
                D(Printf("[locale prefs] LoadPrefsFH: OpenIFF okay.\n"));

                if (!StopChunk(iff, ID_PREF, ID_LCLE))
                {
                    D(Printf("[locale prefs] LoadPrefsFH: StopChunk okay.\n"));

                    if (!ParseIFF(iff, IFFPARSE_SCAN))
                    {
                        struct ContextNode *cn;

                        D(Printf("[locale prefs] LoadPrefsFH: ParseIFF okay.\n"));

                        cn = CurrentChunk(iff);

                        if (cn->cn_Size == sizeof(struct LocalePrefs))
                        {
                            D(Printf("[locale prefs] LoadPrefsFH: ID_LCLE chunk size okay.\n"));

                            if (ReadChunkBytes(iff, &loadprefs, sizeof(struct LocalePrefs)) == sizeof(struct LocalePrefs))
                            {
                                D(Printf("[locale prefs] LoadPrefsFH: Reading chunk successful.\n"));

                                localeprefs = loadprefs;

#if !AROS_BIG_ENDIAN
                                FixLocaleEndianess(&localeprefs);
                                FixCountryEndianess(&localeprefs.lp_CountryData);
#endif

                                D(Printf("[locale prefs] LoadPrefsFH: Everything okay :-)\n"));

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

    D(Printf("[locale prefs] CountryName: %s\n",localeprefs.lp_CountryName));
    int i=0;
    while(i<10 && localeprefs.lp_PreferredLanguages[i])
    {
        D(Printf("[locale prefs] preferred %ld: %s\n",i,localeprefs.lp_PreferredLanguages[i]));
        i++;
    }
    D(Printf("[locale prefs] lp_GMTOffset: %ld\n",localeprefs.lp_GMTOffset));
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

    D(Printf("[locale prefs] SavePrefsFH: fh: %lx\n", fh));

    saveprefs = localeprefs;

#if !AROS_BIG_ENDIAN
    FixLocaleEndianess(&saveprefs);
    FixCountryEndianess(&saveprefs.lp_CountryData);
#endif

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR) fh;
        D(Printf("[locale prefs] SavePrefsFH: stream opened.\n"));

#if 0 /* unused */
        delete_if_error = TRUE;
#endif

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_WRITE))
        {
            D(Printf("[locale prefs] SavePrefsFH: OpenIFF okay.\n"));

            if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
            {
                D(Printf("[locale prefs] SavePrefsFH: PushChunk(FORM) okay.\n"));

                if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
                {
                    struct FilePrefHeader head;

                    D(Printf("[locale prefs] SavePrefsFH: PushChunk(PRHD) okay.\n"));

                    head.ph_Version  = PHV_CURRENT;
                    head.ph_Type     = 0;
                    head.ph_Flags[0] =
                        head.ph_Flags[1] =
                        head.ph_Flags[2] =
                        head.ph_Flags[3] = 0;

                    if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
                    {
                        D(Printf("[locale prefs] SavePrefsFH: WriteChunkBytes(PRHD) okay.\n"));

                        PopChunk(iff);

                        if (!PushChunk(iff, ID_PREF, ID_LCLE, sizeof(struct LocalePrefs)))
                        {
                            D(Printf("[locale prefs] SavePrefsFH: PushChunk(LCLE) okay.\n"));

                            if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
                            {
                                D(Printf("[locale prefs] SavePrefsFH: WriteChunkBytes(SERL) okay.\n"));
                                D(Printf("[locale prefs] SavePrefsFH: Everything okay :-)\n"));

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

    D(Printf("[locale prefs] SaveCharset(%ld)\n", envarc));

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
        Prefs_LoadCountry(localeprefs.lp_CountryName, &localeprefs.lp_CountryData);
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
    D(Printf("[locale prefs] InitPrefs\n"));

    struct LanguageEntry *entry;

    mempool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 2048, 2048);
    if (!mempool)
    {
        ShowMessage("Out of memory!");
        return FALSE;
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

    character_set[0] = 0;
    GetVar("CHARSET", character_set, sizeof(character_set), 0);
    D(Printf("[locale prefs] System character set: %s\n", character_set));

    return TRUE;
}

/*********************************************************************************************/

VOID Prefs_Deinitialize(VOID)
{
    D(Printf("[locale prefs] CleanupPrefs\n"));
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

    if (Prefs_LoadCountry((STRPTR) "united_states", &localeprefs.lp_CountryData))
    {
        retval = TRUE;
    }

    character_set[0] = 0;

    return retval;
}

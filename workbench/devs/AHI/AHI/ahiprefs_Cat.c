/****************************************************************
   This file was created automatically by `FlexCat 2.4'
   from "/cygdrive/e/Private/Projects/AROS-Win32/contrib/necessary/AHI/AHI/ahiprefs.cd".

   Do NOT edit by hand!
****************************************************************/

/*
    Include files and compiler specific stuff
*/
#include <config.h>

#include <exec/memory.h>
#include <libraries/locale.h>
#include <libraries/iffparse.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <proto/iffparse.h>

#include <stdlib.h>
#include <string.h>



#include "ahiprefs_Cat.h"


/*
    Variables
*/
struct FC_String ahiprefs_Strings[75] = {
    { (STRPTR) "\000Project", 0 },
    { (STRPTR) "O\000Open...", 1 },
    { (STRPTR) "A\000Save As...", 2 },
    { (STRPTR) "?\000About...", 3 },
    { (STRPTR) "Q\000Quit", 4 },
    { (STRPTR) "\000Edit", 5 },
    { (STRPTR) "D\000Reset To Defaults", 6 },
    { (STRPTR) "L\000Last Saved", 7 },
    { (STRPTR) "R\000Restore", 8 },
    { (STRPTR) "\000Settings", 9 },
    { (STRPTR) "I\000Create Icons?", 10 },
    { (STRPTR) "\000Help", 11 },
    { (STRPTR) "Help\000Help...", 12 },
    { (STRPTR) "H\000AHI User's guide...", 13 },
    { (STRPTR) "\000Concept Index...", 14 },
    { (STRPTR) "Mode settings", 15 },
    { (STRPTR) "Advanced settings", 16 },
    { (STRPTR) "Unit %ld", 17 },
    { (STRPTR) "Music unit", 18 },
    { (STRPTR) "Mode ID\nRecording\nAuthor\nCopyright\nDriver\nVersion\nAnnotation", 19 },
    { (STRPTR) "Full duplex", 20 },
    { (STRPTR) "Half duplex", 21 },
    { (STRPTR) "Nope", 22 },
    { (STRPTR) "Options", 23 },
    { (STRPTR) "_Frequency", 24 },
    { (STRPTR) "C_hannels", 25 },
    { (STRPTR) "", 26 },
    { (STRPTR) "_Volume", 27 },
    { (STRPTR) "_Monitor", 28 },
    { (STRPTR) "_Gain", 29 },
    { (STRPTR) "Muted", 30 },
    { (STRPTR) "_Input", 31 },
    { (STRPTR) "No inputs", 32 },
    { (STRPTR) "_Output", 33 },
    { (STRPTR) "No outputs", 34 },
    { (STRPTR) "Global options", 35 },
    { (STRPTR) "_Debug level", 36 },
    { (STRPTR) "_Echo", 37 },
    { (STRPTR) "Surround in \"_Fast\" modes", 38 },
    { (STRPTR) "CPU usage _limit", 39 },
    { (STRPTR) "None", 40 },
    { (STRPTR) "Low", 41 },
    { (STRPTR) "High", 42 },
    { (STRPTR) "Full", 43 },
    { (STRPTR) "Enabled", 44 },
    { (STRPTR) "Fast", 45 },
    { (STRPTR) "Disabled", 46 },
    { (STRPTR) "Enabled", 47 },
    { (STRPTR) "Disabled", 48 },
    { (STRPTR) "_Save", 49 },
    { (STRPTR) "_Use", 50 },
    { (STRPTR) "_Cancel", 51 },
    { (STRPTR) "OK", 52 },
    { (STRPTR) "AHI preferences", 53 },
    { (STRPTR) "%s%s\nCopyright ©%s", 54 },
    { (STRPTR) "Unable to open window.", 55 },
    { (STRPTR) "Unable to allocate file requester.", 56 },
    { (STRPTR) "Unable to find `%s'!", 57 },
    { (STRPTR) "Unable to open `%s´ version %ld.", 58 },
    { (STRPTR) "_Master volume", 59 },
    { (STRPTR) "Without clipping", 60 },
    { (STRPTR) "With clipping", 61 },
    { (STRPTR) "Default anti-click time", 62 },
    { (STRPTR) "_Play a test sound", 63 },
    { (STRPTR) "Volume sc_aling", 64 },
    { (STRPTR) "Safe", 65 },
    { (STRPTR) "Safe, dynamic", 66 },
    { (STRPTR) "Full volume", 67 },
    { (STRPTR) "-3 dB", 68 },
    { (STRPTR) "-6 dB", 69 },
    { (STRPTR) "%ld Hz", 70 },
    { (STRPTR) "%ld", 71 },
    { (STRPTR) "%+4.1f dB", 72 },
    { (STRPTR) "%ld ms", 73 },
    { (STRPTR) "%ld%%", 74 }
};

STATIC struct Catalog *ahiprefsCatalog = NULL;
#ifdef LOCALIZE_V20
STATIC STRPTR ahiprefsStrings = NULL;
STATIC ULONG ahiprefsStringsSize;
#endif


VOID CloseahiprefsCatalog(VOID)

{
    if (ahiprefsCatalog) {
	CloseCatalog(ahiprefsCatalog);
    }
#ifdef LOCALIZE_V20
    if (ahiprefsStrings) {
	FreeMem(ahiprefsStrings, ahiprefsStringsSize);
    }
#endif
}


VOID OpenahiprefsCatalog(VOID)

{
    if (LocaleBase) {
	static const struct TagItem ahiprefs_tags[] = {
	  { OC_BuiltInLanguage, (IPTR)"english" },
	  { OC_Version,         4 },
	  { TAG_DONE,           0  }
	};

	ahiprefsCatalog = OpenCatalogA(NULL, (STRPTR)"ahiprefs.catalog", (struct TagItem *)&ahiprefs_tags[0]);
	if (ahiprefsCatalog) {
	    struct FC_String *fc;
	    int i;

	    for (i = 0, fc = ahiprefs_Strings;  i < 75;  i++, fc++) {
		 fc->msg = GetCatalogStr(ahiprefsCatalog, fc->id, (STRPTR) fc->msg);
	    }
	}
    }
}


#ifdef LOCALIZE_V20
VOID InitahiprefsCatalog(STRPTR language)

{
    struct IFFHandle *iffHandle;

    /*
    **  Use iffparse.library only, if we need to.
    */
    if (LocaleBase  ||  !IFFParseBase  ||  !language  ||
	Stricmp(language, "english") == 0) {
	return;
    }

    if ((iffHandle = AllocIFF())) {
	char path[128]; /* Enough to hold 4 path items (dos.library 3.1)    */
	strcpy(path, "PROGDIR:Catalogs");
	AddPart((STRPTR) path, language, sizeof(path));
	AddPart((STRPTR) path, "ahiprefs.catalog", sizeof(path));
	if (!(iffHandle->iff_Stream = Open((STRPTR) path, MODE_OLDFILE))) {
	    strcpy(path, "LOCALE:Catalogs");
	    AddPart((STRPTR) path, language, sizeof(path));
	    AddPart((STRPTR) path, language, sizeof(path));
	    iffHandle->iff_Stream = Open((STRPTR) path, MODE_OLDFILE);
	}

	if (iffHandle->iff_Stream) {
	    InitIFFasDOS(iffHandle);
	    if (!OpenIFF(iffHandle, IFFF_READ)) {
		if (!PropChunk(iffHandle, MAKE_ID('C','T','L','G'),
			       MAKE_ID('S','T','R','S'))) {
		    struct StoredProperty *sp;
		    int error;

		    for (;;) {
			if ((error = ParseIFF(iffHandle, IFFPARSE_STEP))
				   ==  IFFERR_EOC) {
			    continue;
			}
			if (error) {
			    break;
			}

			if ((sp = FindProp(iffHandle, MAKE_ID('C','T','L','G'),
					   MAKE_ID('S','T','R','S')))) {
			    /*
			    **  Check catalog and calculate the needed
			    **  number of bytes.
			    **  A catalog string consists of
			    **      ID (LONG)
			    **      Size (LONG)
			    **      Bytes (long word padded)
			    */
			    LONG bytesRemaining;
			    LONG *ptr;

			    ahiprefsStringsSize = 0;
			    bytesRemaining = sp->sp_Size;
			    ptr = (LONG *) sp->sp_Data;

			    while (bytesRemaining > 0) {
				LONG skipSize, stringSize;

				ptr++;                  /*  Skip ID     */
				stringSize = *ptr++;
				skipSize = ((stringSize+3) >> 2);

				ahiprefsStringsSize += stringSize+1;  /*  NUL */
				bytesRemaining -= 8 + (skipSize << 2);
				ptr += skipSize;
			    }

			    if (!bytesRemaining  &&
				(ahiprefsStrings = AllocMem(ahiprefsStringsSize, MEMF_ANY))) {
				STRPTR sptr;

				bytesRemaining = sp->sp_Size;
				ptr = (LONG *) sp->sp_Data;
				sptr = ahiprefsStrings;

				while (bytesRemaining) {
				    LONG skipSize, stringSize, id;
				    struct FC_String *fc;
				    int i;

				    id = *ptr++;
				    stringSize = *ptr++;
				    skipSize = ((stringSize+3) >> 2);

				    CopyMem(ptr, sptr, stringSize);
				    bytesRemaining -= 8 + (skipSize << 2);
				    ptr += skipSize;

				    for (i = 0, fc = ahiprefs_Strings;  i < 75;  i++, fc++) {
					if (fc->id == id) {
					    fc->msg = sptr;
					}
				    }

				    sptr += stringSize;
				    *sptr++ = '\0';
				}
			    }
			    break;
			}
		    }
		}
		CloseIFF(iffHandle);
	    }
	    Close(iffHandle->iff_Stream);
	}
	FreeIFF(iffHandle);
    }
}
#endif

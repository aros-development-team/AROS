
#define DEBUG 1

#include <exec/types.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <dos/notify.h>
#include <workbench/handler.h>

#ifndef PROTO_EXEC_H
#include <proto/exec.h>
#endif

#ifndef PROTO_DOS_H
#include <proto/dos.h>
#endif

#ifndef PROTO_INTUITION_H
#include <proto/intuition.h>
#endif

#ifndef PROTO_GRAPHICS_H
#include <proto/graphics.h>
#endif

#ifndef PROTO_UTILITY_H
#include <proto/utility.h>
#endif

#ifndef PROTO_KEYMAP_H
#include <proto/keymap.h>
#endif

#ifndef PROTO_LOCALE_H
#include <proto/locale.h>
#endif

#ifndef PROTO_LAYERS_H
#include <proto/layers.h>
#endif

#ifndef PROTO_DATATYPES_H
#include <proto/datatypes.h>
#endif

#ifndef PROTO_ALIB_H
#include <proto/alib.h>
#endif

#ifndef PROTO_ASL_H
#include <proto/asl.h>
#endif

#ifndef PROTO_DISKFONT_H
#include <proto/diskfont.h>
#endif

#ifndef PROTO_IFFPARSE_H
#include <proto/iffparse.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <prefs/prefhdr.h>
#include <prefs/font.h>

#include <aros/debug.h>

#include "wandererprefsintern.h"

#define CHECK_PRHD_VERSION 1
#define CHECK_PRHD_SIZE    1

/** Data/Structs for font.prefs settings */

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

struct FileFontPrefs
{
    UBYTE   fp_Reserved[4 * 3];
    UBYTE   fp_Reserved2[2];
    UBYTE   fp_Type[2];
    UBYTE   fp_FrontPen;
    UBYTE   fp_BackPen;
    UBYTE   fp_Drawmode;
    UBYTE   fp_pad;
    UBYTE   fp_TextAttr_ta_Name[4];
    UBYTE   fp_TextAttr_ta_YSize[2];
    UBYTE   fp_TextAttr_ta_Style;
    UBYTE   fp_TextAttr_ta_Flags;
    BYTE    fp_Name[FONTNAMESIZE];
};

LONG stopchunks[] =
{
    ID_PREF, ID_FONT
};

/** Data pertaining to wanderers internal prefs configured state */

struct IFFHandle *CreateIFF(STRPTR filename, LONG *stopchunks, LONG numstopchunks)
{
    struct IFFHandle *iff;
    
    D(bug("CreateIFF: filename = \"%s\"\n", filename));
    
    if ((iff = AllocIFF()))
    {
    	D(bug("CreateIFF: AllocIFF okay.\n"));
    	if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE)))
	{
    	    D(bug("CreateIFF: Open() okay.\n"));
	    InitIFFasDOS(iff);
	    
	    if (OpenIFF(iff, IFFF_READ) == 0)
	    {
	    	BOOL ok = FALSE;

    	    	D(bug("CreateIFF: OpenIFF okay.\n"));
		
	    	if ((StopChunk(iff, ID_PREF, ID_PRHD) == 0) &&
		    (StopChunks(iff, stopchunks, numstopchunks) == 0))
		{
    	    	    D(bug("CreateIFF: StopChunk(PRHD) okay.\n"));
		    
		    if (ParseIFF(iff, IFFPARSE_SCAN) == 0)
		    {
		    	struct ContextNode *cn;

			cn = CurrentChunk(iff);
			
    	    	    	D(bug("CreateIFF: ParseIFF okay. Chunk Type = %c%c%c%c  ChunkID = %c%c%c%c\n",
			      cn->cn_Type >> 24,
			      cn->cn_Type >> 16,
			      cn->cn_Type >> 8,
			      cn->cn_Type,
			      cn->cn_ID >> 24,
			      cn->cn_ID >> 16,
			      cn->cn_ID >> 8,
			      cn->cn_ID));
			
			if ((cn->cn_ID == ID_PRHD)
    	    	    #if CHECK_PRHD_SIZE
			    && (cn->cn_Size == sizeof(struct FilePrefHeader))
    	    	    #endif
    	    	    	   )
			{
			    struct FilePrefHeader h;
			    
    	    	    	    D(bug("CreateIFF: PRHD chunk okay.\n"));

		    	    if (ReadChunkBytes(iff, &h, sizeof(h)) == sizeof(h))
			    {
    	    	    	    	D(bug("CreateIFF: Reading PRHD chunk okay.\n"));

    	    	    	    #if CHECK_PRHD_VERSION
			    	if (h.ph_Version == PHV_CURRENT)
				{
    	    	    	    	    D(bug("CreateIFF: PrefHeader version is correct.\n"));
				    ok = TRUE;
				}
    	    	    	    #else
    	    	    	    	ok = TRUE;
    	    	    	    #endif	
			    			
			    }
			    
			}
			
		    } /* if (ParseIFF(iff, IFFPARSE_SCAN) == 0) */
		    
		} /* if ((StopChunk(iff, ID_PREF, ID_PRHD) == 0) && (StopChunks(... */
		
		if (!ok)
		{
		    CloseIFF(iff);
	    	    Close((BPTR)iff->iff_Stream);
		    FreeIFF(iff);
		    iff = NULL;
		}
		
	    } /* if (OpenIFF(iff, IFFF_READ) == 0) */
	    else
	    {
	    	Close((BPTR)iff->iff_Stream);
		FreeIFF(iff);
		iff = NULL;
	    }
	    
	} /* if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE))) */
	else
	{
	   FreeIFF(iff); 
	   iff = NULL;
	}
	
    } /* if ((iff = AllocIFF())) */
    
    return iff;
}


/*********************************************************************************************/

void KillIFF(struct IFFHandle *iff)
{
  if (iff)
  {
    CloseIFF(iff);
    Close((BPTR)iff->iff_Stream);
    FreeIFF(iff);
  }
}

/*********************************************************************************************/

static void WandererPrefs_CheckFont(struct WandererInternalPrefsData *WIPD)
{
  struct IFFHandle *iff = NULL;

D(bug("[wanderer] In WandererPrefs_CheckFont()\n"));
    
  if ((iff = CreateIFF("ENV:SYS/Font.prefs", stopchunks, 1)))
  {
    while(ParseIFF(iff, IFFPARSE_SCAN) == 0)
    {
      struct ContextNode   *cn;
      struct FileFontPrefs fontprefs;

      cn = CurrentChunk(iff);

D(bug("[wanderer] WandererPrefs_CheckFont: ParseIFF okay. Chunk Type = %c%c%c%c  ChunkID = %c%c%c%c\n",
      cn->cn_Type >> 24,
      cn->cn_Type >> 16,
      cn->cn_Type >> 8,
      cn->cn_Type,
      cn->cn_ID >> 24,
      cn->cn_ID >> 16,
      cn->cn_ID >> 8,
      cn->cn_ID));

      if ((cn->cn_ID == ID_FONT) && (cn->cn_Size == sizeof(fontprefs)))
      {
D(bug("[wanderer] WandererPrefs_CheckFont: ID_FONT chunk with correct size found.\n"));

        if (ReadChunkBytes(iff, &fontprefs, sizeof(fontprefs)) == sizeof(fontprefs))
        {
		    UWORD   	    type;
		    
D(bug("[wanderer] WandererPrefs_CheckFont: Reading of ID_FONT chunk okay.\n"));

		    type = (fontprefs.fp_Type[0] << 8) + fontprefs.fp_Type[1];

D(bug("[wanderer] WandererPrefs_CheckFont: Type = %d  Name = %s\n", type, fontprefs.fp_Name));

		    if (type == FP_WBFONT)
		    {
		      struct TextFont  *oldWIPD_IconFont = NULL;
		      if (WIPD->WIPD_IconFont != NULL)
		      {
		        oldWIPD_IconFont = WIPD->WIPD_IconFont;
		      }
  		      WIPD->WIPD_IconFontTA.ta_Name  = fontprefs.fp_Name;
		      WIPD->WIPD_IconFontTA.ta_YSize = (fontprefs.fp_TextAttr_ta_YSize[0] << 8) + 
		    	    	                           fontprefs.fp_TextAttr_ta_YSize[1];
		      WIPD->WIPD_IconFontTA.ta_Style = fontprefs.fp_TextAttr_ta_Style;
		      WIPD->WIPD_IconFontTA.ta_Flags = fontprefs.fp_TextAttr_ta_Flags;

		      WIPD->WIPD_IconFont = OpenDiskFont(&WIPD->WIPD_IconFontTA);
D(bug("[wanderer] WandererPrefs_CheckFont: Trying to use Font '%s' @ %x\n", WIPD->WIPD_IconFontTA.ta_Name, WIPD->WIPD_IconFont));
		      if (oldWIPD_IconFont != NULL)
		      {
		        // Cause all windows to update their used font ..

		        // Then close the old font ..
		        CloseFont(oldWIPD_IconFont);
		      }
          }
		    
        } /* if (ReadChunkBytes(iff, &fontprefs, sizeof(fontprefs)) == sizeof(fontprefs)) */
		
      } /* if ((cn->cn_ID == ID_FONT) && (cn->cn_Size == sizeof(fontprefs))) */

    } /* while(ParseIFF(iff, IFFPARSE_SCAN) == 0) */
	    
    KillIFF(iff);
	
  } /* if ((iff = CreateIFF(filename))) */
}

IPTR InitWandererPrefs(void)
{
  struct WandererInternalPrefsData *WIPD = NULL;
  
D(bug("[wanderer] In InitWandererPrefs()\n"));  
  
  if ((WIPD = AllocVec(sizeof(struct WandererInternalPrefsData), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
  {
D(bug("[wanderer] InitWandererPrefs: Allocated PrefsIntern @ %x\n", WIPD)); 
    WandererPrefs_CheckFont(WIPD);
  }

  return WIPD;
}

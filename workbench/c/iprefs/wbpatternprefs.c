/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <prefs/prefhdr.h>
#include <prefs/wbpattern.h>
#include <datatypes/pictureclass.h>

#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************************************/

struct FileWBPatternPrefs
{
    UBYTE   wbp_Reserved[4 * 4];
    UBYTE   wbp_Which[2];
    UBYTE   wbp_Flags[2];
    UBYTE   wbp_Revision;
    UBYTE   wbp_Depth;
    UBYTE   wbp_DataLength[2];
};

struct LayerHookMsg
{
    struct Layer    	*lay;		/* not valid for layerinfo backfill hook!!! */
    struct Rectangle 	bounds;
    LONG    	    	offsetx;
    LONG    	    	offsety;
};

struct LayerHookData
{
    struct BitMap *pat_bm;
    ULONG pat_width;
    ULONG pat_height;
};

/*********************************************************************************************/

static LONG stopchunks[] =
{
    ID_PREF, ID_PTRN
};

/*********************************************************************************************/

static void RootPatternSetup(STRPTR filename);

/*********************************************************************************************/

void WBPatternPrefs_Handler(STRPTR filename)
{
    struct IFFHandle *iff;
    
    D(bug("In IPrefs:WBPatternPrefs_Handler\n"));
    
    if ((iff = CreateIFF(filename, stopchunks, 1)))
    {
	while(ParseIFF(iff, IFFPARSE_SCAN) == 0)
	{
	    struct ContextNode   *cn;
	    struct FileWBPatternPrefs wbpatternprefs;

	    cn = CurrentChunk(iff);

   	    D(bug("WBPatternPrefs_Handler: ParseIFF okay. Chunk Type = %c%c%c%c  ChunkID = %c%c%c%c\n",
		  cn->cn_Type >> 24,
		  cn->cn_Type >> 16,
		  cn->cn_Type >> 8,
		  cn->cn_Type,
		  cn->cn_ID >> 24,
		  cn->cn_ID >> 16,
		  cn->cn_ID >> 8,
		  cn->cn_ID));

	    if ((cn->cn_ID == ID_PTRN) && (cn->cn_Size > sizeof(wbpatternprefs)))
	    {
    	    	D(bug("WBPatternPrefs_Handler: ID_FONT chunk with correct size found.\n"));

		if (ReadChunkBytes(iff, &wbpatternprefs, sizeof(wbpatternprefs)) == sizeof(wbpatternprefs))
		{
		    UWORD   	    type;
		    UWORD   	    len;
		    STRPTR          filename;

    	    	    D(bug("WBPatternPrefs_Handler: Reading of ID_PTRN chunk okay.\n"));

		    type = (wbpatternprefs.wbp_Which[0] << 8) + wbpatternprefs.wbp_Which[1];
		    len = (wbpatternprefs.wbp_DataLength[0] << 8) + wbpatternprefs.wbp_DataLength[1];

		    D(bug("Type = %d  Len = %d\n", type, len));

		    if (sizeof(wbpatternprefs) + len == cn->cn_Size)
		    {
			filename = (STRPTR)AllocVec(len + 1, MEMF_ANY);
			if (filename != NULL)
			{
			    if (ReadChunkBytes(iff, filename, len) == len)
			    {
				filename[len] = 0;
				D(bug("Filename = %s\n", filename));
				switch(type)
				{
				    case WBP_ROOT:
					RootPatternCleanup();
					RootPatternSetup(filename);
					break;
				    case WBP_DRAWER:
					break;
				    case WBP_SCREEN:
					break;
				} /* switch(type) */
			    } /*  if (ReadChunkBytes(iff, filename, len + 1) == len + 1) */
			    FreeVec(filename);
			} /*  if (filename != NULL) */
		    } /*  if (sizeof(wbpatternprefs) + len + 1 == cn->cn_Size) */
		} /* if (ReadChunkBytes(iff, &wbpatternprefs, sizeof(wbpatternprefs)) == sizeof(wbpatternprefs)) */
		
	    } /* if ((cn->cn_ID == ID_FONT) && (cn->cn_Size == sizeof(wbpatternprefs))) */

	} /* while(ParseIFF(iff, IFFPARSE_SCAN) == 0) */
	    
   	KillIFF(iff);
	
    } /* if ((iff = CreateIFF(filename))) */
    
    
    D(bug("In IPrefs:WBPatternPrefs_Handler. Done.\n", filename));
}

/****************************************************************************************/

static void mybackfillfunc(struct Hook *hook,struct RastPort *rp, struct LayerHookMsg *msg)
{
    struct RastPort myrp;
    struct LayerHookData *data = (struct LayerHookData *)hook->h_Data;
    WORD    	    x1,y1,x2,y2,px,py,pw,ph;

    myrp = *rp;

    myrp.Layer = 0;

    x1 = msg->bounds.MinX;
    y1 = msg->bounds.MinY;
    x2 = msg->bounds.MaxX;
    y2 = msg->bounds.MaxY;

    px = x1 % data->pat_width;

    pw = data->pat_width - px;

    do
    {
	y1 = msg->bounds.MinY;
	py = y1  % data->pat_height;

	ph = data->pat_height - py;

	if (pw > (x2 - x1 + 1)) pw = x2 - x1 + 1;

	do
	{
	    if (ph > (y2 - y1 + 1)) ph = y2 - y1 + 1;

	    BltBitMap(data->pat_bm,
		      px,
		      py,
		      rp->BitMap,
		      x1,
		      y1,
		      pw,
		      ph,
		      192,
		      255,
		      0);

	    y1 += ph;

	    py = 0;
	    ph = data->pat_height;

	} while (y1 <= y2); /* while(y1 < y2) */

	x1 += pw;

	px = 0;
	pw = data->pat_width;

    } while (x1 <= x2); /* while (x1 < x2) */

}


/****************************************************************************************/

static struct Hook *installbackfillhook(struct Screen *scr, struct Hook *backfillhook,
					struct LayerHookData *data)
{
    struct Window *tempwin;
    struct Hook *oldhook;

    struct TagItem wintags[] =
    {
    	{WA_PubScreen	,(IPTR)scr			},
    	{WA_Left	,0				},
	{WA_Top		,0				},
	{WA_Width	,scr->Width			},
	{WA_Height	,scr->Height			},
	{WA_Borderless	,TRUE				},
	{WA_Backdrop	,TRUE				},
	{WA_BackFill	,(IPTR)LAYERS_NOBACKFILL	},
	{TAG_DONE					}
    };

    backfillhook->h_Entry    = HookEntry;
    backfillhook->h_SubEntry = (HOOKFUNC)mybackfillfunc;
    backfillhook->h_Data     = data;

    oldhook = InstallLayerInfoHook(&scr->LayerInfo, backfillhook);
    tempwin = OpenWindowTagList(NULL, wintags);
    if (tempwin) CloseWindow(tempwin);
    return oldhook;
}

/****************************************************************************************/

static void removebackfillhook(struct Screen *scr, struct Hook *oldhook)
{
    struct TagItem wintags[] =
    {
    	{ WA_PubScreen , (IPTR)scr		 },
    	{ WA_Left      , 0			 },
	{ WA_Top       , 0			 },
	{ WA_Width     , scr->Width		 },
	{ WA_Height    , scr->Height		 },
	{ WA_Borderless, TRUE			 },
	{ WA_Backdrop  , TRUE			 },
	{ WA_BackFill  , (IPTR)LAYERS_NOBACKFILL },
	{ TAG_DONE				 }
    };
    struct Window *tempwin;
    
    InstallLayerInfoHook(&scr->LayerInfo, oldhook);
    tempwin = OpenWindowTagList(NULL, wintags);
    if (tempwin) CloseWindow(tempwin);
}

/*********************************************************************************************/

static struct BitMap *LoadDTBitmap (CONST_STRPTR filename, struct Screen *scr,
				    Object **obj, ULONG *width, ULONG *height)
{
    struct BitMapHeader *bmhd;
    struct BitMap *bitmap = NULL;
    Object *o;
    struct Process *myproc = (struct Process *)FindTask(NULL);
    APTR oldwindowptr = myproc->pr_WindowPtr;

    myproc->pr_WindowPtr = (APTR)-1;

    o = NewDTObject((APTR)filename,
		    DTA_GroupID          , GID_PICTURE,
		    OBP_Precision        , PRECISION_EXACT,
		    PDTA_Screen          , (IPTR)scr,
		    PDTA_FreeSourceBitMap, TRUE,
		    PDTA_DestMode        , PMODE_V43,
		    PDTA_UseFriendBitMap , TRUE,
		    TAG_DONE);
	
    myproc->pr_WindowPtr = oldwindowptr;

    if (o)
    {
	struct FrameInfo fri = {0};
	DoMethod(o, DTM_FRAMEBOX, NULL, (IPTR)&fri, (IPTR)&fri, 
		 sizeof(struct FrameInfo), 0);
	
	if (fri.fri_Dimensions.Depth > 0)
	{
	    if (DoMethod(o, DTM_PROCLAYOUT, NULL, 1))
	    {
		*obj = o;
		GetDTAttrs(o, PDTA_BitMapHeader, (IPTR)&bmhd, TAG_DONE);
		if (bmhd)
		{
		    *width = bmhd->bmh_Width;
		    *height = bmhd->bmh_Height;
		    GetDTAttrs(o, PDTA_DestBitMap, (IPTR)&bitmap, TAG_DONE);
		    if (NULL == bitmap)
			GetDTAttrs(o, PDTA_BitMap, (IPTR)&bitmap, TAG_DONE);
		    if (bitmap)
			return bitmap;
		}
	    }

	}
	DisposeDTObject(o);
    }
    return NULL;
}

/*********************************************************************************************/

static Object        *myDTObject = NULL;
static struct Hook   *myOldHook = (APTR)-1; // NULL is a valid value
static struct Screen *myScreen = NULL;
static struct LayerHookData myData;
static struct Hook    myBackFillHook;

/*********************************************************************************************/

static void RootPatternSetup(STRPTR filename)
{
    struct BitMap *patternbm = NULL;
    ULONG w;
    ULONG h;

    if ((myScreen = LockPubScreen(NULL)) != NULL)
    {
	D(bug("loading '%s'\n", filename));
	patternbm = LoadDTBitmap(filename, myScreen, &myDTObject, &w, &h);

	if (patternbm)
	{
	    myData.pat_bm = patternbm;
	    myData.pat_width = w;
	    myData.pat_height = h;
    
	    myOldHook = installbackfillhook(myScreen, &myBackFillHook, &myData);
	    D(bug("oldhook=%p\n", myOldHook));
	}
    }
}

/*********************************************************************************************/

void RootPatternCleanup (void)
{
    if (myOldHook != (APTR)-1)
    {
	D(bug("Reinstalling old backfillhook\n"));
	removebackfillhook(myScreen, myOldHook);
	myOldHook = (APTR)-1;
    }
    if (myDTObject)
    {
	D(bug("Disposing DT obj\n"));
	DisposeDTObject(myDTObject);
	myDTObject = NULL;
    }
    if (myScreen)
    {
	D(bug("Unlock PubScreen\n"));
	UnlockPubScreen(NULL, myScreen);
	myScreen = NULL;
    }
}



/* Boopsi Button */

/* The Image structure as used by this class:
 *
 * struct Image {
 *
 * SHORT    LeftEdge;        <---- Offset
 * SHORT    TopEdge;
 *
 * SHORT    Width;           <---- } 4 chars 'Oops' used for identification!
 * SHORT    Height;          <---- }
 *
 * SHORT    Depth;           <---- maintained by boopsi (set to CUSTOMIMAGEDEPTH).
 *
 * USHORT   *ImageData;      <---- only used during init, then points to self!
 *
 * UBYTE    PlanePick;       <---- used for the foreground color
 *
 * UBYTE    PlaneOnOff;      <---- holds ToUpper (underlined char) (after creation!)
 *
 * struct Image *NextImage;  <---- pointer to the next image. Handled by DrawImage().
 * };
 *
 */

#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/sghooks.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <libraries/gadtools.h>
#include <graphics/monitor.h>
//#include <devices/audio.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <proto/layers.h>
#include <proto/utility.h>
#include <proto/console.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <string.h>

#ifdef __AROS__
#include <aros/asmcall.h>
#endif

#include "filereq.h"

/****************************************************************************************/

extern struct Library 		*GadToolsBase;
extern struct GfxBase 		*GfxBase;
extern struct IntuitionBase 	*IntuitionBase;
extern struct Window 		*win;
extern struct Screen 		*scr;
extern struct ExecBase 		*SysBase;
extern struct Device 		*ConsoleDevice;

extern void ShortDelay (void);

#ifndef __AROS__
extern ULONG ASM myBoopsiDispatch (
    REGPARAM(a0, Class *,),
    REGPARAM(a2, struct Image *,),
    REGPARAM(a1, struct impDraw *,));
#endif

extern Class *ButtonImgClass;

/****************************************************************************************/

/**********
* BUTTONS *
**********/

struct Gadget * REGARGS my_CreateButtonGadget (
    struct Gadget *gad,
    ULONG underscorechar,
    struct NewGadget *ng)
{
    struct InitData 	idata;
    struct Image 	*image;
    const char 		*label;

    label = ng->ng_GadgetText;
    ng->ng_GadgetText = NULL;
    
    if ((gad = myCreateGadget (GENERIC_KIND, gad, ng, TAG_END)))
    {

	/* set gadget attributes */
	gad->Flags |= GFLG_GADGIMAGE|GFLG_GADGHIMAGE;
	gad->GadgetType |= GTYP_BOOLGADGET;
	gad->Activation |= GACT_RELVERIFY;

	/* set image data */
	idata.idata_Gadget = gad;
	idata.idata_VisualInfo = ng->ng_VisualInfo;
	idata.idata_Label = label;
	idata.idata_TextAttr = ng->ng_TextAttr;
	idata.idata_Underscore = underscorechar;

	/* create image */

	image = NewObject (ButtonImgClass, NULL, IA_Data, (IPTR) &idata, TAG_END);

	gad->GadgetRender = gad->SelectRender = image;
    }
    
    ng->ng_GadgetText = label;

    return (gad);
}

/****************************************************************************************/

static struct Image *IsButtonGad (struct Gadget *gad)
{
    struct Image *im;
    union {
	struct {
	    WORD Width;
	    WORD Height;
	} size;
	ULONG magic;
    } __tmp;

    if (gad->Flags & (GFLG_GADGIMAGE|GFLG_GADGHIMAGE))
	if ((im = (struct Image *)gad->SelectRender))
	    if (im->Depth == CUSTOMIMAGEDEPTH)
	    {
		__tmp.size.Width = im->Width;
		__tmp.size.Height = im->Height;
		if (__tmp.magic == BUTTON_MAGIC_LONGWORD)
		    if (im->ImageData == (UWORD *)im) return (im);
	    }

    return (NULL);
}

/****************************************************************************************/

/**********
* STRGADS *
**********/

struct CombStringInfo
{
    ULONG 			magic;
    struct CombStringInfo 	*self;
    struct StringInfo 		strinfo;
    struct StringExtend 	strextend;
    struct Hook 		edithook;
};

/****************************************************************************************/

#define IEQUALIFIER_SHIFT		(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)

/****************************************************************************************/

#ifdef __AROS__
AROS_UFH3(ULONG, StrEditHookEntry,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct SGWork *, sgw, A2),
    AROS_UFHA(ULONG *, msg, A1))
{
    AROS_USERFUNC_INIT
#else
ULONG ASM SAVEDS
StrEditHookEntry (
    REGPARAM(a0, struct Hook *, hook),
    REGPARAM(a2, struct SGWork *, sgw),
    REGPARAM(a1, ULONG *, msg) )
{
#endif
    struct StrGadUserData	*userdata;
    WORD	qual, rawcode;

    if( msg[ 0 ] == SGH_KEY )
    {
	rawcode = sgw->IEvent->ie_Code;
	qual = sgw->IEvent->ie_Qualifier;

//kprintf("sgsg: editop = %d code = %x  quali = %x\n", sgw->EditOp, rawcode, qual);

	if( ( sgw->EditOp == EO_INSERTCHAR ) ||
	    ( sgw->EditOp == EO_REPLACECHAR ) ||
	    ( sgw->EditOp == EO_SPECIAL ) ||  /* CHECKME: AROS/AMIGAOS: ADDED THIS LINE */
	    ( sgw->EditOp == EO_NOOP ) ||  /* CHECKME: AROS/AMIGAOS: ADDED THIS LINE */
	    ( sgw->EditOp == EO_ENTER ) ||  /* CHECKME: AROS/AMIGAOS: ADDED THIS LINE */
	    ( sgw->EditOp == EO_BADFORMAT ) )
	{
//kprintf("sgsg2\n");
	    if( ( qual & IEQUALIFIER_RCOMMAND ) || ( sgw->Code == 27 ) )
	    {
		sgw->Actions &= ~( SGA_USE | SGA_BEEP | SGA_REDISPLAY );
		sgw->IEvent->ie_Qualifier &= ~IEQUALIFIER_RCOMMAND;

		if( !( qual & IEQUALIFIER_REPEAT ) )
		{
			sgw->Actions |= SGA_REUSE|SGA_END;
			sgw->Code = KEYB_SHORTCUT;
		}
	    }
	}

	if( ( userdata = ( struct StrGadUserData * ) sgw->Gadget->UserData ) )
	{
	    if( userdata->flags & USERFLAG_MATCH_FILE )
	    {
		if( sgw->Actions & SGA_USE )
		{
		    if( Stricmp( sgw->WorkBuffer, sgw->PrevBuffer ) )
		    {
			if( !userdata->fakeimsg.Micros )
			{
			    userdata->fakeimsg.Micros = 1;
			    PutMsg( userdata->msgport, ( struct Message * ) &userdata->fakeimsg );
			}
		    }
		}
	    }

	    if( userdata->flags & USERFLAG_UP_DOWN_ARROW )
	    {
		if( ( rawcode == RAWKEY_UP ) || ( rawcode == RAWKEY_DOWN ) )
		{
		    sgw->Actions &= ~( SGA_USE | SGA_BEEP | SGA_REDISPLAY );
		    sgw->Actions |= SGA_REUSE | SGA_END;
		    sgw->Code = KEYB_SHORTCUT;
		}
	    }
	    
	} /* if( ( userdata = ( struct StrGadUserData * ) sgw->Gadget->UserData ) ) */

	return( TRUE );
	
    } /* if( msg[ 0 ] == SGH_KEY ) */

    return( FALSE );
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/****************************************************************************************/

struct Hook stredithook =
{
    { NULL },
    ( HOOKFUNC ) StrEditHookEntry, NULL, ( APTR ) 0xff525421
};

/****************************************************************************************/

#ifndef GTST_EditHook
#define GTST_EditHook (GT_TagBase+55)
#define GTIN_EditHook GTST_EditHook
#endif

/****************************************************************************************/

struct Gadget * REGARGS my_CreateIntegerGadget (
    struct Gadget *gad,
    struct NewGadget *newgad,
    int maxchars,
    LONG value,
    ULONG just)
{
    struct Gadget *intgad;

    if ((intgad = myCreateGadget (INTEGER_KIND, gad, newgad,
	    GTIN_MaxChars, maxchars,
	    GTIN_Number, value, STRINGA_Justification, just,
	    GTST_EditHook, (IPTR) &stredithook,
    TAG_END)))
	    intgad->UserData = NULL;
	    
    return (intgad);
}

/****************************************************************************************/

struct Gadget * REGARGS my_CreateStringGadget (
    struct Gadget *gad,
    struct NewGadget *newgad,
    int maxchars,
    char *string)
{
    struct Gadget *strgad;

    if ((strgad = myCreateGadget (STRING_KIND, gad, newgad, GTST_MaxChars, maxchars,
				  GTST_String, (IPTR) string, GTST_EditHook, (IPTR) &stredithook, TAG_END)))
	    strgad->UserData = NULL;
 
    return (strgad);
}

/****************************************************************************************/

void REGARGS my_SetStringGadget (struct Window *win, struct Gadget *gad, char *str)
{
    if (!gad) return;
    
    myGT_SetGadgetAttrs (gad, win, NULL, GTST_String, (IPTR) str, TAG_END);
}

/****************************************************************************************/

void REGARGS my_SetIntegerGadget (struct Window *win, struct Gadget *gad, long val)
{
    if (!gad) return;
    
    myGT_SetGadgetAttrs (gad, win, NULL, GTIN_Number, val, TAG_END);
}

/****************************************************************************************/

void REGARGS my_FreeGadgets (struct Gadget *glist)
{
    struct Gadget *gad;
    struct Image  *im;

#ifdef USE_FORBID
    Forbid();
#else
    /* FIXME: Any reason for locking here?
       As far as I can tell all use of this function are reentrant and the
       glist is never attached to Window when called.
       If locking is needed for other reasons, semaphore would be better
       than Forbid(). -Piru */
#endif
    for (gad = glist; gad; gad = gad->NextGadget)
    {
	if ((im = IsButtonGad (gad))) DisposeObject (im);
    }
    FreeGadgets (glist);
#ifdef USE_FORBID
    Permit();
#else
    /* unlock */
#endif
}

/****************************************************************************************/

void REGARGS my_SelectGadget (struct Gadget *gad, struct Window *win)
{
    gad->Flags ^= GFLG_SELECTED;
    RefreshGList (gad, win, NULL, 1);
}

/****************************************************************************************/

void REGARGS my_DownGadget (struct Gadget *gad, UWORD code, struct KeyButtonInfo *info)
{
    my_SelectGadget (gad, info->win);
    ShortDelay();
    info->lastgad = gad;
    info->lastcode = code;
}

/****************************************************************************************/

#define SHIFT_KEY		0x60

/****************************************************************************************/

struct Gadget *REGARGS my_GetKeyGadget (UBYTE key, struct Gadget *glist)
{
    struct Gadget *gad;
    struct Image  *im;
    char 	  underkey;

    for (gad = glist; gad; gad = gad->NextGadget)
    {
	if ((im = IsButtonGad (gad)))
	    if ((underkey = im->PlaneOnOff))
		if (key == (UBYTE)ToUpper (underkey)) return (gad);
    }
    return (NULL);
}

/****************************************************************************************/

ULONG REGARGS CheckGadgetKey (int code, int qual, char *key,
					  struct KeyButtonInfo *info)
{
    struct InputEvent 	ev;
    struct Gadget 	*gad;
    int 		upkey = (code & IECODE_UP_PREFIX);

    *key = 0;
    if (!(code & ~IECODE_UP_PREFIX)) return (0);

    /* Convert RAW to ASCII */
    ev.ie_NextEvent = NULL;
    ev.ie_Class = IECLASS_RAWKEY;
    ev.ie_Code = code;
    /* Ignore alt qualifier */
    ev.ie_Qualifier = qual & ~(IEQUALIFIER_LALT|IEQUALIFIER_RALT);
    RawKeyConvert (&ev, key, 1, NULL);
    *key = ToUpper (*key);

    if (!(qual & IEQUALIFIER_REPEAT))
    {
	if (upkey)
	{
	    /* Gadget released ? */
	    if (code == (info->lastcode | IECODE_UP_PREFIX))
	    {
		if (!(info->lastgad->Activation & GACT_TOGGLESELECT))
		    my_SelectGadget (info->lastgad, info->win);
		info->lastcode = 0;
		return ((ULONG)info->lastgad->GadgetID);
		
	    }
	}
	else
	{
	    /* Shift pressed ? */
	    if ((code & ~1) == SHIFT_KEY)
	    {
		if (info->lastcode)
		{
		    my_SelectGadget (info->lastgad, info->win);
		    info->lastcode = 0;
		}
	    }
	    else
	    {
		/* No gadget down yet ? */
		if (!info->lastcode)
		{
		    /* Gadget down ? */
		    if ((gad = my_GetKeyGadget (*key, info->glist)))
		    {
			if (!(gad->Flags & GFLG_DISABLED))
			    my_DownGadget (gad, code, info);
			    
		    }
		    else /* return with keycode in 'key' */
			return (0);
			
		}
		
	    }
	    
	} /* if (upkey) else ... */
	
    } /* if (!(qual & IEQUALIFIER_REPEAT)) */
    
    *key = 0;
    return (0);
}

/****************************************************************************************/

/*********
* IMAGES *
*********/

/* Read VisualInfo and TextAttr from newgadget structure */
/* Underscore char is always '_' */

struct Image * REGARGS my_CreateGadgetLabelImage (
    struct Image *previm,
    struct NewGadget *ng,
    char *label,
    WORD left, WORD top,
    UWORD pen)
{
    struct InitData idata;

    if (!previm) return (NULL);
    
    idata.idata_Gadget = NULL;
    idata.idata_Label = label;
    idata.idata_VisualInfo = ng->ng_VisualInfo;
    idata.idata_TextAttr = ng->ng_TextAttr;
    idata.idata_Underscore = '_';
    
    return (previm->NextImage = NewObject (ButtonImgClass, NULL, IA_Data , (IPTR) &idata,
    								 IA_FGPen, pen   , 
								 IA_Left , left  ,
								 IA_Top  , top   ,
								 TAG_END));
}

/****************************************************************************************/

void REGARGS my_FreeLabelImages (struct Image *images)
{
    struct Image *im;

    images = images->NextImage;
    while (images)
    {
	im = images;
	images = images->NextImage;
	DisposeObject (im);
    }
}

/****************************************************************************************/

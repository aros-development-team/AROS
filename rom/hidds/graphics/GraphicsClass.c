/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics hidd class implementation.
    Lang: english
*/

/****************************************************************************************/

#include <aros/atomic.h>
#include <aros/config.h>
#include <aros/symbolsets.h>
#include <cybergraphx/cgxvideo.h>
#include <exec/lists.h>
#include <oop/static_mid.h>
#include <graphics/displayinfo.h>
#include <graphics/view.h>

#include "graphics_intern.h"

#include <string.h>
#include <stddef.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>
#include <exec/memory.h>

#include <utility/tagitem.h>

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define DPF(x)


#include <hidd/graphics.h>

/*****************************************************************************************

    NAME
	--background_graphics--

    LOCATION
	hidd.graphics.graphics

    NOTES
	When working with graphics drivers this is the first object you get.
	It allows you to create BitMap and GC (graphics context)
	object. The class' methods must be overidden by hardware-specific
	subclasses where documented to do so.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	--display_modes--

    LOCATION
	hidd.graphics.graphics

    NOTES
	Each display driver object internally stores a database of supported display mode
	IDs. This database is normally managed by base class, the driver does not need to
	reimplement respective methods.

	A display mode ID in AROS is a 32-bit integer value, the same as on AmigaOS(tm).
	However mode ID layout introduced by Commodore does not fit well for RTG systems.
	In order to overcome its limitations, display ID on AROS may have two forms:

	1. A chipset mode ID. These are standard IDs defined by Commodore. You may find
           their definitions in graphics/modeid.h.

	2. AROS RTG mode ID.

	An RTG mode ID is composed of three parts in the form:

	nnnn xx yy

	nnnn - monitor ID. This number is maintained by system libraries. IDs are
	       assigned in the order in which drivers are loaded and display hardware is
	       found. Drivers do not have to care about this part, and should normally
	       mask it out if they for some reason look at mode ID. In order to
	       distinguish between chipset mode IDs and RTG mode IDs, order number starts
	       not from zero, reserving some space for C= chipset mode IDs (which appear
	       to have order numbers from 0x0000 to 0x000A). Currently RTG monitor IDs
	       start from 0x0010, however with time this value may change. So don't rely
	       on some particular values in RTG IDs. Use cybergraphics.library/IsCyberModeID()
	       function if you want to know for sure if the given mode ID belongs to an
	       RTG driver.

	  xx - A sync object index in driver's mode database.
	  yy - A pixelformat object in driver's mode database.

	Normally the driver does not have to care about mode ID decoding. The mode
	database is maintained by base class. The only useful things for the driver are
	sync and pixelformat objects, from which it's possible to get different
	information about the mode. They can be obtained from the base class using
	HIDD_Gfx_GetMode().

	Note that the driver object by itself does not know its monitor ID. Different
	displays are served by different objects, any of which may belong to any class.
	So all driver methods which return mode IDs will set monitor ID to zero. All
	methods that take mode ID as argument are expected to ignore the monitor ID part
	and do not make any assumptions about its value.

*****************************************************************************************/

static BOOL create_std_pixfmts(struct class_static_data *_csd);
static VOID delete_pixfmts(struct class_static_data *_csd);
static BOOL register_modes(OOP_Class *cl, OOP_Object *o, struct TagItem *modetags);

static BOOL alloc_mode_db(struct mode_db *mdb, ULONG numsyncs, ULONG numpfs, OOP_Class *cl);
static VOID free_mode_db(struct mode_db *mdb, OOP_Class *cl);
static OOP_Object *create_and_init_object(OOP_Class *cl, UBYTE *data, ULONG datasize,
    	    	    	    	    	  struct class_static_data *_csd);

static struct pixfmt_data *find_pixfmt(HIDDT_PixelFormat *tofind
	, struct class_static_data *_csd);

static VOID copy_bm_and_colmap(OOP_Class *cl, OOP_Object *o,  OOP_Object *src_bm
	, OOP_Object *dst_bm, ULONG width, ULONG height);

BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf, ULONG attrcheck, struct class_static_data *_csd);

/****************************************************************************************/

#define COMPUTE_HIDD_MODEID(sync, pf)	\
    ( ((sync) << 8) | (pf) )
    
#define MODEID_TO_SYNCIDX(id) (((id) & 0X0000FF00) >> 8)
#define MODEID_TO_PFIDX(id)   ( (id) & 0x000000FF)

/****************************************************************************************/

OOP_Object *GFX__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct HIDDGraphicsData *data;
    BOOL    	    	    ok = FALSE;
    struct TagItem  	    *modetags;
    struct TagItem  	    gctags[] =
    {
    	{TAG_DONE, 0UL}
    };

    D(bug("Entering gfx.hidd::New\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    	return NULL;
    
    D(bug("Got object o=%x\n", o));

    data = OOP_INST_DATA(cl, o);
    
    InitSemaphore(&data->mdb.sema);
/*  data->curmode = vHidd_ModeID_Invalid; */

    /* Get the mode tags */
    modetags = (struct TagItem *)GetTagData(aHidd_Gfx_ModeTags, 0, msg->attrList);
    if (NULL != modetags)
    {
	/* Parse it and register the gfxmodes */
	if (register_modes(cl, o, modetags))
	{
	    ok = TRUE;
	}
	else
	    D(bug("Could not register modes\n"));
    }
    else {
	D(bug("Could not get ModeTags\n"));
	ok = TRUE;
    }

    /* Create a gc that we can use for some rendering */
    if (ok)
    {
	data->gc = OOP_NewObject(CSD(cl)->gcclass, NULL, gctags);
	if (NULL == data->gc)
	{
	    D(bug("Could not get gc\n"));
	    ok = FALSE;
	}
    }
    
    if (!ok)
    {
	D(bug("Not OK\n"));
	OOP_MethodID dispose_mid;
	
	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
	o = NULL;
    }
    
    D(bug("Leaving gfx.hidd::New o=%x\n", o));

    return o;
}

/****************************************************************************************/

VOID GFX__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct HIDDGraphicsData *data;
    
    data = OOP_INST_DATA(cl, o);
    
    /* free the mode db stuff */
    free_mode_db(&data->mdb, cl);

    /* Here we should unregister pixelformats registered in our New().
       However gfx drivers aren't supposed to be removed, so it's okay
       not to do it at all for now. */

    if (NULL != data->gc)
	OOP_DisposeObject(data->gc);

    OOP_DoSuperMethod(cl, o, msg);
}

/*****************************************************************************************

    NAME
	aoHidd_Gfx_IsWindowed

    SYNOPSIS
	[..G], BOOL

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Tells if the display driver is using hosted display in host OS' window, and mouse
	input is handled by host OS.

	Windowed displays may send activation events to AROS. This is needed in order to
	correctly handle display switch in a multi-display configuration (which means that
	the user has multiple windows on host OS desktop and can freely switch between them).

    NOTES
	Even in fullscreen mode drivers should still return TRUE if the host OS manages mouse
	input (for example, X11 driver). If mouse input is not managed by the host OS
	(for example, with Linux framebuffer driver), return FALSE.

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_ActiveCallBack, aoHidd_Gfx_ActiveCallBackData

    INTERNALS
	Base class always provides FALSE value

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_DMPSLevel

    SYNOPSIS
	[ISG], HIDDT_DPMSLevel

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Gets or sets current DPMS level for driver's display.
	A value can be one of:
	    vHidd_Gfx_DPMSLevel_On,
	    vHidd_Gfx_DPMSLevel_Standby,
	    vHidd_Gfx_DPMSLevel_Suspend,
	    vHidd_Gfx_DPMSLevel_Off

	If the driver does not support some state, it's up to the driver what to do.
	Usually it is expected to ignore the request.

	Getting this attribute should return real current state.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Base class always provides vHidd_Gfx_DPMSLevel_On value (comes from rootclass'
	Get() which sets the value to 0).

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_ModeTags

    SYNOPSIS
	[I..], struct TagItem *

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Specify a pointer to a taglist which contains description of display modes
	supported by the driver.

	This attribute is usually appended in moRoot_New method of the display driver
	class.
	
	This attribute is mandatory for the base class, otherwise driver object creation
	fails.
	
	Mode description taglist may contain the following tags:
	  - Any sync attributes - these attributes will specify values common for all sync
				  modes
	  - Any pixelformat attributes - these attributes will specify values common for
					 all pixelformat modes
	  - aoHidd_Gfx_SyncTags - specifies a pointer to another separate taglist containing
				  attributes for one sync (display) mode. If this tag
				  is not supplied at all, a set of default modes will be
				  generated for the driver.
	  - aoHidd_Gfx_PixFmtTags - specifies a pointer to another separate taglist containing
				    attributes for one pixelformat. This tag must be supplied
				    at least once, otherwise driver object will fail to create.
	
	  aoHidd_Gfx_SyncTags and aoHidd_Gfx_PixFmtTags can be specified multiple times in
	  order to associate more than one display mode with the driver. Note that common
	  values for sync and pixelformat objects need to be placed in the taglist before
	  aoHidd_Gfx_SyncTags and aoHidd_Gfx_PixFmtTags. You may specify them again between
	  these tags in order to alter common values.

    NOTES

    EXAMPLE
	Partial example code of display driver supporting a truecolor display with three
	resolutions:

	// Our pixelformat (24-bit 0BGR)
	struct TagItem pftags[] =
	{
    	    { aHidd_PixFmt_RedShift     , 24			   	},
	    { aHidd_PixFmt_GreenShift   , 16			   	},
	    { aHidd_PixFmt_BlueShift    , 8			   	},
	    { aHidd_PixFmt_AlphaShift   , 0			   	},
	    { aHidd_PixFmt_RedMask	, 0x000000FF		   	},
	    { aHidd_PixFmt_GreenMask    , 0x0000FF00		   	},
	    { aHidd_PixFmt_BlueMask     , 0x00FF0000		   	},
	    { aHidd_PixFmt_AlphaMask    , 0x00000000		   	},
	    { aHidd_PixFmt_ColorModel   , vHidd_ColorModel_TrueColor   	},
	    { aHidd_PixFmt_Depth	, 24			   	},
	    { aHidd_PixFmt_BytesPerPixel, 4				},
	    { aHidd_PixFmt_BitsPerPixel , 24			   	},
	    { aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_Native	},
	    { aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Chunky       },
	    { TAG_DONE  	    	, 0UL			   	} 
	};

	// 640x480 resolution
	struct TagItem tags_800_600[] = 
	{
	    { aHidd_Sync_HDisp  	, 640 	    	    	 },
	    { aHidd_Sync_VDisp  	, 480 	    	    	 },
	    { TAG_DONE  	    	, 0UL 	    	    	 }
	};

	// 800x600 resolution
	struct TagItem tags_800_600[] = 
	{
	    { aHidd_Sync_HDisp  	, 800 	    	    	 },
	    { aHidd_Sync_VDisp  	, 600 	    	    	 },
	    { TAG_DONE  	    	, 0UL 	    	    	 }
	};

	// 1024x768 resolution
	struct TagItem tags_1024_768[] = 
	{
    	    { aHidd_Sync_HDisp  	, 1024      	    	  },
	    { aHidd_Sync_VDisp  	, 768       	    	  },
	    { TAG_DONE  	    	, 0UL       	    	  }
	};

	// Mode description taglist itself
	struct TagItem mode_tags[] =
	{
	    // Our driver supports a single pixelformat
	    { aHidd_Gfx_PixFmtTags  , (IPTR)pftags		},

	    // Here go sync values common for all sync modes
	    { aHidd_Sync_HMin	    , 112			},
	    { aHidd_Sync_VMin	    , 112			},
	    { aHidd_Sync_HMax	    , 16384			},
	    { aHidd_Sync_VMax	    , 16384			},
	    { aHidd_Sync_Description, (IPTR)"Example: %hx%v"	},

	    // First resolution
	    { aHidd_Gfx_SyncTags    , (IPTR)tags_800_600	},

	    // Next two syncs will have HMax = 32768, as an example
	    { aHidd_Sync_HMax	    , 32768			},

	    // Two more resolutions
	    { aHidd_Gfx_SyncTags    , (IPTR)tags_800_600	},
	    { aHidd_Gfx_SyncTags    , (IPTR)tags_1024_768	},
	    { TAG_DONE  	    , 0UL 	    	    	}
	};
    
	// This is the attribute list which is given to New method
	// of the base class
	struct TagItem mytags[] =
	{
	    { aHidd_Gfx_ModeTags	, (IPTR)mode_tags	},
	    { TAG_DONE  	    	, NULL 			}
	};

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_NumSyncs

    SYNOPSIS
	[..G], ULONG

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Gets total number of sync objects in the internal display mode database.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_GetSync

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_SupportsHWCursor

    SYNOPSIS
	[..G], BOOL

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Tells whether the driver supports hardware mouse pointer sprite.

	If the driver provides TRUE value for this attribute, it is expected to implement
	HIDD_Gfx_SetCursorPos(), HIDD_Gfx_SetCursorShape() and HIDD_Gfx_SetCursorVisible()
	methods.

	Mouse pointer counts for one hardware sprite, so if the driver implements also
	HIDD_Gfx_ModeProperties(), it should set NumHWSprites to 1 in order to provide
	valid information about display modes.

	The driver must implement this attribute if it implements HIDD_Gfx_ModeProperties().
	Otherwise it will provide false information in graphics.library/GetDisplayInfoData().
	Base class can determine NumHWSprites based on this attribute value but not vice
	versa.

    NOTES
	Default implementation in the base class returns FALSE. This causes the system to
	use software sprite emulation.

	This attribute is obsolete and is used only by AROS graphics.library up to v41.2. In
	new drivers consider implementing aoHidd_Gfx_HWSpriteTypes attribute.

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_HWSpriteTypes, moHidd_Gfx_ModeProperties

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_NoFrameBuffer

    SYNOPSIS
	[..G], BOOL

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Tells whether the driver does not need a framebuffer.

	A framebuffer is a special bitmap in a fixed area of video RAM. If the framebuffer
	is used, the driver is expected to copy a new bitmap into it in HIDD_Gfx_Show()
	and optionally copy old bitmap back.

	A framebuffer is needed if the hardware does not have enough VRAM to store many
	bitmaps or does not have capabilities to switch the display between various VRAM
	regions.

	An example of driver using a framebuffer is hosted SDL driver. By design SDL works
	only with single display window, which is considered a framebuffer.

    NOTES
	Provides FALSE if not implemented in the driver.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_Show

    INTERNALS
	VGA and VESA do not use framebuffer, they use mirroring technique instead in order
	to prevents VRAM reading which is slow.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_HWSpriteTypes

    SYNOPSIS
	[..G], BOOL

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Return hardware sprite image types supported by the driver.
	
	The returned value is a combination of the following bit flags:
	  vHidd_SpriteType_3Plus1 - color 0 is transparent, 1-3 visible
				    (Amiga(tm) chipset sprite format)
	  vHidd_SpriteType_2Plus1 - color 0 is transparent, color 1 is undefined
				     (can be whatever, for example clear or inverse),
				     colors 2-3 visible.
	  vHidd_SpriteType_DirectColor - Hi- or truecolor image, or LUT image with own
					 palette, perhaps with alpha channel

    NOTES
	This attribute should return 0 if the driver does not support hardware mouse sprite
	at all. Software sprite emulation is done by graphics.library.

	Default implementation in the base class is based on aoHidd_Gfx_SupportsHWCursor
	value. This is done for backwards compatibility.

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_SupportsHWCursor

    INTERNALS
	Default implementation in the base class queries aoHidd_Gfx_SupportsHWCursor
	and provides (vHidd_SpriteType_3Plus1|vHidd_SpriteType_DirectColor) in case
	if it returns TRUE. Otherwise it returns zero. This is done for backwards
	compatibility with old drivers.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_MemorySize

    SYNOPSIS
	[..G], ULONG

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Query total size of video card memory in bytes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_MemoryClock

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_MemoryClock

    SYNOPSIS
	[..G], ULONG

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Query video card's memory clock in Hz. 0 is a valid value meaning 'unknown'.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_MemorySize

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_DriverName

    SYNOPSIS
	[..G], STRPTR

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Query CyberGraphX driver name. It is the same name which can be given to
	cybergraphics.library/BestCModeIDTagList() as CYBRBIDTG_BoardName value.

    NOTES
	By default base class returns class name as value of this attribute.
	However this can (and must for some drivers listed in BestCModeIDTagList()
	documentation) be overriden.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_ActiveCallBack

    SYNOPSIS
	[.S.], void (*)(APTR userdata, OOP_Object *bitmap)

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Set display activation interrupt handler.

	This handler needs to be called by hosted display driver, if host OS
	windowing system is used for the display and mouse input is handled by the
	host OS.

	This way the driver can tell AROS when a display window has been activated so that
	AROS will be able to switch current display correctly when working in a multi-display
	configuration.

	The function uses C calling convention and needs to be declared as follows:

	void ActivationHandler(APTR userdata, OOP_Object *bitmap);

	Parameters of this function will be:
	  userdata - Whatever is specified by aoHidd_Gfx_ActiveCallBackData attribute.
	  bitmap   - Currently reserved. Drivers need to set it to NULL.

	The function can be called from within an interrupt, so usual restrictions apply
	to it.

	Set this attribute to NULL in order to disable activation handling.

    NOTES
	When setting the activation callback function, be sure that you set correct
	userdata before you actually set the callback pointer. Otherwise your callback
	can be called with wrong data pointer.

	Only one activation handler can be installed. Installing a new handler replaces
	the previous one.

	Native displays do not need to implement this attribute because there can be
	no external activation events.

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_ActiveCallBackData, aoHidd_Gfx_IsWindowed

    INTERNALS
	This attribute needs to be implemented by the display driver. Base class contains
	no implementation.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_ActiveCallBackData

    SYNOPSIS
	[.S.], APTR

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Set user-defined data pointer for display activation handler.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_ActiveCallBack

    INTERNALS
	This attribute needs to be implemented by the display driver. Base class contains
	no implementation.

*****************************************************************************************/

VOID GFX__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HIDDGraphicsData *data;
    ULONG   	    	    idx;

    data = OOP_INST_DATA(cl, o);

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_Gfx_NumSyncs:
		*msg->storage = data->mdb.num_syncs;
		return;

	    case aoHidd_Gfx_IsWindowed:
	    case aoHidd_Gfx_SupportsHWCursor:
	    	*msg->storage = 0;
		return;

	    case aoHidd_Gfx_HWSpriteTypes:
	    {
		IPTR hwc;

		OOP_GetAttr(o, aHidd_Gfx_SupportsHWCursor, &hwc);
		*msg->storage = hwc ? (vHidd_SpriteType_3Plus1|vHidd_SpriteType_DirectColor) : 0;
		return;
	    }

	    case aoHidd_Gfx_DriverName:
		*msg->storage = (IPTR)OOP_OCLASS(o)->ClassNode.ln_Name;
		return;

	    default:	/* Keep compiler happy */
		break;
	}
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_NewGC

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_NewGC *msg);

	OOP_Object *HIDD_Gfx_NewGC(OOP_Object *gfxHidd, struct TagItem *tagList);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Create a GC (gfx context) object that may be used for rendering
	into a bitmap.

    INPUTS
	gfxHidd - A graphics driver object with which the GC will perform
	          the rendering operations.
	tagList - A list of GC attributes. See hidd.graphics.gc class
	          documentation for their description.

    RESULT
	gc - pointer to the newly created GC, ready for use for rendering
	     operations.

    NOTES
	A GC object is just a data storage. You may create a subclass of GC if
	you wish to, however there's usually no need to. Additionally, this may
	be not future-proof (since GC subclasses can not be interchanged between
	different drivers. Please avoid using custom GCs.

    EXAMPLE

    BUGS
	At the moment subclassing GCs is not supported because some parts of
	the operating system create GC objects directly. It is unclear whether
	subclassing GCs is actually needed.

    SEE ALSO
	moHidd_Gfx_DisposeGC

    INTERNALS

*****************************************************************************************/

OOP_Object *GFX__Hidd_Gfx__NewGC(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewGC *msg)
{
    OOP_Object *gc = NULL;

    EnterFunc(bug("HIDDGfx::NewGC()\n"));

    gc = OOP_NewObject(NULL, CLID_Hidd_GC, msg->attrList);

    ReturnPtr("HIDDGfx::NewGC", OOP_Object *, gc);
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_DisposeGC

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_DisposeGC *msg);

	VOID HIDD_Gfx_DisposeGC(OOP_Object *gfxHidd, OOP_Object *gc)

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Deletes a GC (Graphics Context) object previously created
	by HIDD_Gfx_NewGC().

	Subclasses do not have to override this method
	unless they allocate anything additional to a gc object in
	their HIDD_Gfx_NewGC() implementation.

    INPUTS
	gfxHidd - A driver object which was used for creating a GC.
	gc      - Pointer to gc object to delete.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_NewGC

    INTERNALS
	Basically just does OOP_DisposeObject(gc);

*****************************************************************************************/

VOID GFX__Hidd_Gfx__DisposeGC(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_DisposeGC *msg)
{
    EnterFunc(bug("HIDDGfx::DisposeGC()\n"));

    if (NULL != msg->gc) OOP_DisposeObject(msg->gc);

    ReturnVoid("HIDDGfx::DisposeGC");
}

/****************************************************************************************/

#define BMAO(x) aoHidd_BitMap_ ## x
#define BMAF(x) (1L << aoHidd_BitMap_ ## x)

#define BM_DIMS_AF  (BMAF(Width) | BMAF(Height))

#define SET_TAG(tags, idx, tag, val)	\
    tags[idx].ti_Tag = tag ; tags[idx].ti_Data = (IPTR)val;

#define SET_BM_TAG(tags, idx, tag, val)	\
    SET_TAG(tags, idx, aHidd_BitMap_ ## tag, val)

/*****************************************************************************************

    NAME
	moHidd_Gfx_NewBitMap

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_NewBitMap *msg);

	OOP_Object *HIDD_Gfx_NewBitMap(OOP_Object *gfxHidd, struct TagItem *tagList);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Create a bitmap object.

	One graphics driver represents at least one displayable bitmap class.
	Additionally it may represent more classes (for example some old drivers use
	a separate class for nondisplayable bitmaps).

	These classes are private to the driver. In order to be able to use them
	bitmap objects are never created directly. Instead they are created using the
	HIDD_Gfx_NewBitMap() call. An implementation of this method in the driver
	should examine bitmap attributes supplied and make a decision if the bitmap
	should be created using the driver's own class or one of the system classes.

	A typical implementation should pay attention to the following bitmap attributes:
    
	aHIDD_BitMap_ModeID - If this attribute is supplied, the bitmap needs to be
			      either displayable by this driver, or be a friend of a
			      displayable bitmap. A friend bitmap usually repeats the
			      internal layout of its friend so that the driver may
			      perform blitting operations quickly.

	aHIDD_BitMap_Displayable - If this attribute is supplied, the bitmap NEEDS to be
			           displayable by this driver. Usually this means that
			           the bitmap object will contain video hardware state
			           information. This attribute will always be accompanied
			           by aHIDD_BitMap_ModeID.

	aHIDD_BitMap_FrameBuffer - The bitmap needs to be a framebuffer bitmap. A
			           framebuffer bitmap is necessary for some kinds of
			           hardware which have a small fixed amount of video
			           RAM which can hold only one screen at a time. Setting
			           this attribute requires that a valid ModeID be also set.

	aHIDD_BitMap_Friend - If there's no ModeID supplied, you may wish to check class
			      of friend bitmap. This can be useful if your driver uses
			      different classes for displayable and non-displayable bitmaps.
			      By default base class will pick up friend's class and use it
			      for new bitmap if nothing is specified, here you may override
			      this behavior.

	If your driver wants to specify own class for the bitmap being created,
	it should prepend an aHIDD_BitMap_ClassPtr attribute to the supplied taglist
	and pass it to base class. It's not allowed to create bitmap objects directly
	since they need some more extra information which is added by the base class!

	This method must be implemented by your subclass. aHIDD_BitMap_ClassPtr or
	aHIDD_BitMap_ClassID must be provided to the base class for a displayable bitmap!

    INPUTS
	gfxHidd - A graphics driver object with which the GC will perform
	          the rendering operations.

	tagList - A list of bitmap attributes. See hidd.graphics.bitmap class
	          documentation for their description.

    RESULT
	gc - pointer to the newly created GC, ready for use for rendering
             operations.
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_DisposeBitMap

    INTERNALS
	The base class implementation currently does the folliwing in order to determine
	a class for a nondisplayable bitmap (in the listed order):

	1. Check aHIDD_BitMap_ClassPtr and aHIDD_BitMap_ClassID. If one of them is supplied,
	   the class is already set by a subclass.
	2. Check aHIDD_BitMap_StdPixFmt. If this attribute is supplied, figure out type of
	   the pixelformat (chunky or planar), and use one of two system's default classes.
	3. Check aHIDD_BitMap_Friend. If friend bitmap is supplied, obtain its class from
	   aHIDD_BitMap_ClassPtr value of friend bitmap.
	4. If everything fails, bitmap creation fails too.

	This behavior is subject to change, but will maintain backwards compatibility.

*****************************************************************************************/

OOP_Object * GFX__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o,
    	    	    	    	      struct pHidd_Gfx_NewBitMap *msg)
{
    struct TagItem  	    bmtags[8];
    
    IPTR    	    	    attrs[num_Total_BitMap_Attrs];
    STRPTR  	    	    classid = NULL;
    OOP_Class 	    	    *classptr = NULL;
    BOOL    	    	    displayable = FALSE; /* Default attr value */
    BOOL    	    	    framebuffer = FALSE;
    OOP_Object      	    *pf = NULL, *sync;
    HIDDT_ModeID    	    modeid = 0;
    OOP_Object      	    *bm;
    IPTR    	    	    depth = 0;
    struct HIDDGraphicsData *data;
    
    DECLARE_ATTRCHECK(bitmap);
    
    BOOL    	    	    gotclass = FALSE;

    data = OOP_INST_DATA(cl, o);
    
    if (0 != OOP_ParseAttrs(msg->attrList, attrs, num_Total_BitMap_Attrs,
    	    	    	    &ATTRCHECK(bitmap), HiddBitMapAttrBase))
    {
	D(bug("!!! FAILED TO PARSE ATTRS IN Gfx::NewBitMap !!!\n"));
	return NULL;
    }
    
    if (GOT_BM_ATTR(PixFmt))
    {
	D(bug("!!! Gfx::NewBitMap: USER IS NOT ALLOWED TO PASS aHidd_BitMap_PixFmt !!!\n"));
	return NULL;
    }
    
    /* Get class supplied by superclass */
    if (GOT_BM_ATTR(ClassPtr))
    {
    	classptr	= (OOP_Class *)attrs[BMAO(ClassPtr)];
	gotclass = TRUE;
    }
    else
    {
	if (GOT_BM_ATTR(ClassID))
	{
    	    classid	= (STRPTR)attrs[BMAO(ClassID)];
	    gotclass = TRUE;
	}
    }
		
    if (GOT_BM_ATTR(Displayable))
	displayable = (BOOL)attrs[BMAO(Displayable)];

    if (GOT_BM_ATTR(FrameBuffer))
    {
    	framebuffer = (BOOL)attrs[BMAO(FrameBuffer)];
	if (framebuffer) displayable = TRUE;
    }

    if (GOT_BM_ATTR(ModeID))
    {
	modeid = attrs[BMAO(ModeID)];
	
	/* Check that it is a valid mode */
	if (!HIDD_Gfx_GetMode(o, modeid, &sync, &pf))
	{
	    D(bug("!!! Gfx::NewBitMap: USER PASSED INVALID MODEID !!!\n"));
	}
    }
    if (GOT_BM_ATTR(Depth))
    	depth = attrs[BMAO(Depth)];

    /* First argument is gfxhidd */    
    SET_BM_TAG(bmtags, 0, GfxHidd, o);
    SET_BM_TAG(bmtags, 1, Displayable, displayable);
    
	
    if (displayable || framebuffer)
    {
	/* The user has to supply a modeid */
	if (!GOT_BM_ATTR(ModeID))
	{
	    D(bug("!!! Gfx::NewBitMap: USER HAS NOT PASSED MODEID FOR DISPLAYABLE BITMAP !!!\n"));
	    return NULL;
	}
	
	if (!gotclass)
	{
	    D(bug("!!! Gfx::NewBitMap: SUBCLASS DID NOT PASS CLASS FOR DISPLAYABLE BITMAP !!!\n"));
	    return NULL;
	}
	
	SET_BM_TAG(bmtags, 2, ModeID, modeid);
	SET_BM_TAG(bmtags, 3, PixFmt, pf);
	
	if (framebuffer)
	{
	    SET_BM_TAG(bmtags, 4, FrameBuffer, TRUE);
	}
	else
	{
	    SET_TAG(bmtags, 4, TAG_IGNORE, 0UL);
	}
	if (!depth)
	    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	SET_BM_TAG(bmtags, 5, Depth, depth);
	SET_TAG(bmtags, 6, TAG_MORE, msg->attrList);
	
    }
    else
    { /* if (displayable) */
	IPTR width, height;
    
	/* To get a pixfmt for an offscreen bitmap we either need 
	    (ModeID || ( (Width && Height) && StdPixFmt) || ( (Width && Height) && Friend))
	*/
	    
	if (GOT_BM_ATTR(ModeID))
	{   
	    /* We have allredy gotten pixelformat and sync for the modeid case */
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
	}
	else
	{
	    /* Next to look for is StdPixFmt */
	    
	    /* Check that we have width && height */
	    if (BM_DIMS_AF != (BM_DIMS_AF & ATTRCHECK(bitmap)))
	    {
		D(bug("!!! Gfx::NewBitMap() MISSING WIDTH/HEIGHT TAGS !!!\n"));
		return NULL;
	    }
	    
	    width  = attrs[BMAO(Width)];
	    height = attrs[BMAO(Height)];
	    	    
	    if (GOT_BM_ATTR(StdPixFmt))
	    {
		pf = HIDD_Gfx_GetPixFmt(o, (HIDDT_StdPixFmt)attrs[BMAO(StdPixFmt)]);
		if (NULL == pf)
		{
		    D(bug("!!! Gfx::NewBitMap(): USER PASSED BOGUS StdPixFmt !!!\n"));
		    return NULL;
		}
	    }
	    else
	    {
		/* Last alternative is that the user passed a friend bitmap */
		if (GOT_BM_ATTR(Friend))
		{
		    OOP_Object *friend_bm = (OOP_Object *)attrs[BMAO(Friend)];
		    OOP_GetAttr(friend_bm, aHidd_BitMap_PixFmt, (IPTR *)&pf);
		    /* Try to grab the class from friend bitmap (if not already specified).
		       We do it because friend bitmap may be a display HIDD bitmap */
		    if (!gotclass) {
			/* Another weirdness is that we have to use this attribute instead of
			   simple getting OOP_OCLASS(friend_bm). We can't get class directly
			   from the object, because the framebuffer bitmap object may be a
			   fakegfx.hidd object, which is even not a bitmap at all. Attempt
			   to create a bitmap of this class causes system-wide breakage.
			   Perhaps fakegfx HIDD should be fixed in order to handle this correctly.
			*/
			OOP_GetAttr(friend_bm, aHidd_BitMap_ClassPtr, (IPTR *)&classptr);
			D(bug("[GFX] Friend bitmap is 0x%p has ClassPtr 0x%p\n", friend_bm, classptr));
			if (classptr)
		            gotclass = TRUE;
		    }
		}
		else
		{
		    D(bug("!!! Gfx::NewBitMap: UNSIFFICIENT ATTRS TO CREATE OFFSCREEN BITMAP !!!\n"));
		    return NULL;
		}
	    }
	}
	
	/* Did the subclass provide an offbitmap class for us ? */
	if (!gotclass)
	{
	    /* Have to find a suitable class ourselves */
	    HIDDT_BitMapType bmtype;
		
	    OOP_GetAttr(pf, aHidd_PixFmt_BitMapType, &bmtype);
	    switch (bmtype)
	    {
	        case vHidd_BitMapType_Chunky:
		    classptr = CSD(cl)->chunkybmclass;
		    break;
		    
	        case vHidd_BitMapType_Planar:
		    classptr = CSD(cl)->planarbmclass;
		    break;
		    
	        default:
	    	    D(bug("!!! Gfx::NewBitMap: UNKNOWN BITMAPTYPE %d !!!\n", bmtype));
		    return NULL;
		    
	    }
	    D(bug("[GFX] Bitmap type is %u, using class 0x%p\n", bmtype, classptr));
	    
	} /* if (!gotclass) */
	
	if (!depth)
	    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

	/* Set the tags we want to pass to the selected bitmap class */
	SET_BM_TAG(bmtags, 2, Width,  width);
	SET_BM_TAG(bmtags, 3, Height, height);
	SET_BM_TAG(bmtags, 4, Depth, depth);
	SET_BM_TAG(bmtags, 5, PixFmt, pf);

	if (GOT_BM_ATTR(Friend))
	{
	    SET_BM_TAG(bmtags, 6, Friend, attrs[BMAO(Friend)]);
	}
	else
	{
	    SET_TAG(bmtags, 6, TAG_IGNORE, 0UL);
	}
	SET_TAG(bmtags, 7, TAG_MORE, msg->attrList);
	
    } /* if (!displayable) */
    

    bm = OOP_NewObject(classptr, classid, bmtags);

    if (framebuffer)
    	data->framebuffer = bm;
	
    return bm;
    
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_DisposeBitMap
	
    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_DisposeBitMap *msg);

	VOID HIDD_Gfx_DisposeBitMap(OOP_Object *gfxHidd, OOP_Object *bitMap);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Deletes a bitmap object previously created by HIDD_Gfx_NewBitMap().

	Subclasses do not have to override this method
	unless they allocate anything additional to a bitmap object in
	their HIDD_Gfx_NewBitMap() implementation.

    INPUTS
	gfxHidd - A driver object which was used for creating a bitmap.
	bitMap  - Pointer to a bitmap object to delete.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_NewBitMap

    INTERNALS
	Basically just does OOP_DisposeObject(bitMap);

******************************************************************************************/

VOID GFX__Hidd_Gfx__DisposeBitMap(OOP_Class *cl, OOP_Object *o,
				  struct pHidd_Gfx_DisposeBitMap *msg)
{
    if (NULL != msg->bitMap)
	OOP_DisposeObject(msg->bitMap);
}

/****************************************************************************************/

#define SD(x) 	    	    	    ((struct sync_data *)x)
#define PF(x) 	    	    	    ((HIDDT_PixelFormat *)x)

#define XCOORD_TO_BYTEIDX(x) 	    ( (x) >> 3)
#define COORD_TO_BYTEIDX(x, y, bpr) ( ( (y) * bpr ) + XCOORD_TO_BYTEIDX(x) )
#define XCOORD_TO_MASK(x)   	    (1L << (7 - ((x) & 0x07) ))
#define WIDTH_TO_BYTES(width) 	    ( (( (width) - 1) >> 3) + 1)

/****************************************************************************************/

/* modebm functions pfidx is x and syncidx is y coord in the bitmap */

/****************************************************************************************/

static inline BOOL alloc_mode_bm(struct mode_bm *bm, ULONG numsyncs, ULONG numpfs,
    	    	    	    	 OOP_Class *cl)
{
    bm->bpr = WIDTH_TO_BYTES(numpfs);
    
    bm->bm = AllocVec(bm->bpr * numsyncs, MEMF_CLEAR);
    if (NULL == bm->bm)
	return FALSE;
	
    /* We initialize the mode bitmap to all modes valid */
    memset(bm->bm, 0xFF, bm->bpr * numsyncs);
    
    return TRUE;
}

/****************************************************************************************/

static inline VOID free_mode_bm(struct mode_bm *bm, OOP_Class *cl)
{
    FreeVec(bm->bm);
    bm->bm  = NULL;
    bm->bpr = 0;
}

/****************************************************************************************/

static inline BOOL is_valid_mode(struct mode_bm *bm, ULONG syncidx, ULONG pfidx)
{
    if (0 != (XCOORD_TO_MASK(pfidx) & bm->bm[COORD_TO_BYTEIDX(pfidx, syncidx, bm->bpr)]))
	return TRUE;
	
    return FALSE;
}

/****************************************************************************************/

static inline VOID set_valid_mode(struct mode_bm *bm, ULONG syncidx, ULONG pfidx,
    	    	    	    	  BOOL valid)
{
    if (valid)
	bm->bm[COORD_TO_BYTEIDX(pfidx, syncidx, bm->bpr)] |= XCOORD_TO_MASK(pfidx);
    else
	bm->bm[COORD_TO_BYTEIDX(pfidx, syncidx, bm->bpr)] &= ~XCOORD_TO_MASK(pfidx);

    return;
}

/****************************************************************************************/

static BOOL alloc_mode_db(struct mode_db *mdb, ULONG numsyncs, ULONG numpfs, OOP_Class *cl)
{
    BOOL ok = FALSE;
    
    if (0 == numsyncs || 0 == numpfs)
	return FALSE;
	
    ObtainSemaphore(&mdb->sema);
    /* free_mode_bm() needs this */
    mdb->num_pixfmts	= numpfs;
    mdb->num_syncs	= numsyncs;

    mdb->syncs	 = AllocMem(sizeof (OOP_Object *) * numsyncs, MEMF_CLEAR);
    
    if (NULL != mdb->syncs)
    {
	mdb->pixfmts = AllocMem(sizeof (OOP_Object *) * numpfs,   MEMF_CLEAR);
	
	if (NULL != mdb->pixfmts)
	{
	    if (alloc_mode_bm(&mdb->orig_mode_bm, numsyncs, numpfs, cl))
	    {
		if (alloc_mode_bm(&mdb->checked_mode_bm, numsyncs, numpfs, cl))
		{
		    ok = TRUE;
		}
	    }
	}
    }
    
    if (!ok)
	free_mode_db(mdb, cl);
	
    ReleaseSemaphore(&mdb->sema);
    
    return ok;
}

/****************************************************************************************/

static VOID free_mode_db(struct mode_db *mdb, OOP_Class *cl)
{
    ULONG i;
    
    ObtainSemaphore(&mdb->sema);
    
    if (NULL != mdb->pixfmts)
    {
    
	/* Pixelformats are shared objects and never freed */
	FreeMem(mdb->pixfmts, sizeof (OOP_Object *) * mdb->num_pixfmts);
	mdb->pixfmts = NULL; mdb->num_pixfmts = 0;
    }

    if (NULL != mdb->syncs)
    {
    	for (i = 0; i < mdb->num_syncs; i ++)
	{
	    if (NULL != mdb->syncs[i])
	    {
	    
	    	OOP_DisposeObject(mdb->syncs[i]);
		mdb->syncs[i] = NULL;
	    }
	}
	
	FreeMem(mdb->syncs, sizeof (OOP_Object *) * mdb->num_syncs);
	mdb->syncs = NULL; mdb->num_syncs = 0;
    }
    
    if (NULL != mdb->orig_mode_bm.bm)
    {
	free_mode_bm(&mdb->orig_mode_bm, cl);
    }
    
    if (NULL != mdb->checked_mode_bm.bm)
    {
	free_mode_bm(&mdb->checked_mode_bm, cl);
    }
    
    ReleaseSemaphore(&mdb->sema);
    
    return;
}

/****************************************************************************************/

/* Initializes default tagarray. in numtags the TAG_MORE is not accounted for,
   so the array must be of size NUM_TAGS + 1
*/

/****************************************************************************************/

static VOID init_def_tags(struct TagItem *tags, ULONG numtags)
{
    ULONG i;
    
    for (i = 0; i < numtags; i ++)
    {
	tags[i].ti_Tag = TAG_IGNORE;
	tags[i].ti_Data = 0UL;
    }
    
    tags[i].ti_Tag  = TAG_MORE;
    tags[i].ti_Data = 0UL;
    
    return;
}

/****************************************************************************************/

#define MAKE_SYNC(name,clock,hdisp,hstart,hend,htotal,vdisp,vstart,vend,vtotal,descr)   \
    struct TagItem sync_ ## name[]={            \
        { aHidd_Sync_PixelClock,    clock*1000  },  \
        { aHidd_Sync_HDisp,         hdisp   },  \
        { aHidd_Sync_HSyncStart,    hstart  },  \
        { aHidd_Sync_HSyncEnd,      hend    },  \
        { aHidd_Sync_HTotal,        htotal  },  \
        { aHidd_Sync_VDisp,         vdisp   },  \
        { aHidd_Sync_VSyncStart,    vstart  },  \
        { aHidd_Sync_VSyncEnd,      vend    },  \
        { aHidd_Sync_VTotal,        vtotal  },  \
        { aHidd_Sync_Description,       (IPTR)descr},   \
        { TAG_DONE, 0UL }}

static BOOL register_modes(OOP_Class *cl, OOP_Object *o, struct TagItem *modetags)
{
    struct TagItem  	    *tag, *tstate;
    struct HIDDGraphicsData *data;
    
    MAKE_SYNC(640x480_60,   25174,
         640,  656,  752,  800,
         480,  490,  492,  525,
         "Default:640x480");

    MAKE_SYNC(800x600_56,   36000,  // 36000
         800,  824,  896, 1024,
         600,  601,  603,  625,
         "Default:800x600");

    MAKE_SYNC(1024x768_60, 65000,   //78654=60kHz, 75Hz. 65000=50kHz,62Hz
        1024, 1048, 1184, 1344,
         768,  771,  777,  806,
        "Default:1024x768");

    MAKE_SYNC(1152x864_60, 80000,
        1152, 1216, 1328, 1456,
         864,  870,  875,  916,
        "Default:1152x864");

    MAKE_SYNC(1280x1024_60, 108880,
        1280, 1360, 1496, 1712,
        1024, 1025, 1028, 1060,
        "Default:1280x1024");

    MAKE_SYNC(1600x1200_60, 155982,
        1600, 1632, 1792, 2048,
        1200, 1210, 1218, 1270,
        "Default:1600x1200");

    /* "new" 16:10 modes */

    MAKE_SYNC(1280x800_60, 83530,
    	1280, 1344, 1480, 1680,
    	800, 801, 804, 828,
    	"Default:1280x800");

    MAKE_SYNC(1440x900_60, 106470,
		1440, 1520, 1672, 1904,
		900, 901, 904, 932,
		"Default:1440x900");

    MAKE_SYNC(1680x1050_60, 147140,
		1680, 1784, 1968, 2256,
		1050, 1051, 1054, 1087,
		"Default:1680x1050");

    MAKE_SYNC(1920x1080_60, 173000,
		1920, 2048, 2248, 2576,
		1080, 1083, 1088, 1120,
		"Default:1920x1080");

    MAKE_SYNC(1920x1200_60, 154000,
		1920, 1968, 2000, 2080,
		1200, 1203, 1209, 1235,
		"Default:1920x1200");

    struct mode_db  	    *mdb;
    
    HIDDT_PixelFormat 	    pixfmt_data;

    struct TagItem  	    def_sync_tags[num_Hidd_Sync_Attrs     + 1];
    struct TagItem  	    def_pixfmt_tags[num_Hidd_PixFmt_Attrs + 1];
    
    ULONG   	    	    numpfs = 0,numsyncs	= 0;
    ULONG   	    	    pfidx = 0, syncidx = 0;
    
    struct TagItem temporary_tags[] = {
        { aHidd_Gfx_SyncTags,   (IPTR)sync_640x480_60   },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_800x600_56   },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1024x768_60  },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1152x864_60  },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1280x1024_60 },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1600x1200_60 },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1280x800_60 },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1440x900_60 },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1680x1050_60 },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1920x1080_60 },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1920x1200_60 },
        { TAG_MORE, 0UL }
    };

    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;
    InitSemaphore(&mdb->sema);

    memset(&pixfmt_data, 0, sizeof (pixfmt_data));

    init_def_tags(def_sync_tags,	num_Hidd_Sync_Attrs);
    init_def_tags(def_pixfmt_tags,	num_Hidd_PixFmt_Attrs);

    def_sync_tags[aoHidd_Sync_GfxHidd].ti_Tag = aHidd_Sync_GfxHidd;
    def_sync_tags[aoHidd_Sync_GfxHidd].ti_Data = (IPTR)o;
    
    /* First we need to calculate how much memory we are to allocate by counting supplied
       pixel formats and syncs */
    
    for (tstate = modetags; (tag = NextTagItem((const struct TagItem **)&tstate));)
    {
	ULONG idx;
	
	if (IS_GFX_ATTR(tag->ti_Tag, idx))
	{
	    switch (idx)
	    {
		case aoHidd_Gfx_PixFmtTags:
		    numpfs++;
		    break;
		    
		case aoHidd_Gfx_SyncTags:
		    numsyncs ++;
		    break;
		    
		default:
		    break;
	    }
	}
    }
    
    if (0 == numpfs)
    {
    	D(bug("!!! WE MUST AT LEAST HAVE ONE PIXFMT IN Gfx::RegisterModes() !!!\n"));
    }

    if (0 == numsyncs)
    {
    	D(bug("!!! NO SYNC IN Gfx::RegisterModes() !!!\n!!! USING DEFAULT MODES !!!\n"));
    	temporary_tags[11].ti_Tag = TAG_MORE;
    	temporary_tags[11].ti_Data = (IPTR)modetags;
    	modetags = &temporary_tags[0];
    	numsyncs = 11;
    }

    ObtainSemaphore(&mdb->sema);
    
    /* Allocate memory for mode db */
    if (!alloc_mode_db(&data->mdb, numsyncs, numpfs, cl))
	goto failure;
    
    
    for (tstate = modetags; (tag = NextTagItem((const struct TagItem **)&tstate));)
    {
	/* Look for Gfx, PixFmt and Sync tags */
	ULONG idx;
	
	if (IS_GFX_ATTR(tag->ti_Tag, idx))
	{
	    switch (idx)
	    {
		case aoHidd_Gfx_PixFmtTags:
		    def_pixfmt_tags[num_Hidd_PixFmt_Attrs].ti_Data = tag->ti_Data;
		    mdb->pixfmts[pfidx] = HIDD_Gfx_RegisterPixFmt(o, def_pixfmt_tags);
		    
		    if (NULL == mdb->pixfmts[pfidx])
		    {
			D(bug("!!! UNABLE TO CREATE PIXFMT OBJECT IN Gfx::RegisterModes() !!!\n"));
			goto failure;
		    }
		    
		    pfidx ++;
		    break;

		case aoHidd_Gfx_SyncTags:
		    def_sync_tags[num_Hidd_Sync_Attrs].ti_Data = tag->ti_Data;

		    mdb->syncs[syncidx] = OOP_NewObject(CSD(cl)->syncclass, NULL, def_sync_tags);
		    if (!mdb->syncs[syncidx]) {
			D(bug("!!! UNABLE TO CREATE SYNC OBJECT IN Gfx::RegisterModes() !!!\n"));
			goto failure;
		    }

		    syncidx ++;
		    break;
	    }
	    
	}
	else if (IS_SYNC_ATTR(tag->ti_Tag, idx))
	{
	    if (idx >= num_Hidd_Sync_Attrs)
	    {
		D(bug("!!! UNKNOWN SYNC ATTR IN Gfx::New(): %d !!!\n", idx));
	    }
	    else
	    {
		def_sync_tags[idx].ti_Tag  = tag->ti_Tag;
		def_sync_tags[idx].ti_Data = tag->ti_Data;
	    }
	    
	}
	else if (IS_PIXFMT_ATTR(tag->ti_Tag, idx))
	{
	    if (idx >= num_Hidd_PixFmt_Attrs)
	    {
		D(bug("!!! UNKNOWN PIXFMT ATTR IN Gfx::New(): %d !!!\n", idx));
	    }
	    else
	    {
		def_pixfmt_tags[idx].ti_Tag  = tag->ti_Tag;
		def_pixfmt_tags[idx].ti_Data = tag->ti_Data;
	    }
	}
    }
    
    ReleaseSemaphore(&mdb->sema);
    
    return TRUE;
    
failure:

    /*	mode db is freed in dispose */
    ReleaseSemaphore(&mdb->sema);
    
    return FALSE;
}

/****************************************************************************************/

struct modequery
{
    struct mode_db  *mdb;
    ULONG   	    minwidth;
    ULONG   	    maxwidth;
    ULONG   	    minheight;
    ULONG   	    maxheight;
    HIDDT_StdPixFmt *stdpfs;
    ULONG   	    numfound;
    ULONG   	    pfidx;
    ULONG   	    syncidx;
    BOOL    	    dims_ok;
    BOOL    	    stdpfs_ok;
    BOOL    	    check_ok;
    OOP_Class 	    *cl;
};

/****************************************************************************************/

/* This is a recursive function that looks for valid modes */

/****************************************************************************************/

static HIDDT_ModeID *querymode(struct modequery *mq)
{
    HIDDT_ModeID    	*modeids;
    register OOP_Object *pf;
    register OOP_Object *sync;
    BOOL    	    	mode_ok = FALSE;
    ULONG   	    	syncidx, pfidx;
    
    mq->dims_ok	  = FALSE;
    mq->stdpfs_ok = FALSE;
    mq->check_ok  = FALSE;
    
    /* Look at the supplied idx */
    if (mq->pfidx >= mq->mdb->num_pixfmts)
    {
	mq->pfidx = 0;
	mq->syncidx ++;
    }

    if (mq->syncidx >= mq->mdb->num_syncs)
    {
	/* We have reached the end of the recursion. Allocate memory and go back 
	*/
	
	modeids = AllocVec(sizeof (HIDDT_ModeID) * (mq->numfound + 1), MEMF_ANY);
	/* Get the end of the array */
	modeids += mq->numfound;
	*modeids = vHidd_ModeID_Invalid;
	
	return modeids;
    }

    syncidx = mq->syncidx;
    pfidx   = mq->pfidx;
    /* Get the pf and sync objects */
    pf   = mq->mdb->pixfmts[syncidx];
    sync = mq->mdb->syncs[pfidx];
    

    /* Check that the mode is really usable */
    if (is_valid_mode(&mq->mdb->checked_mode_bm, syncidx, pfidx))
    {
	mq->check_ok = TRUE;
    
    
	/* See if this mode matches the criterias set */
	
	if (	SD(sync)->hdisp  >= mq->minwidth
	     && SD(sync)->hdisp  <= mq->maxwidth
	     && SD(sync)->vdisp >= mq->minheight
	     && SD(sync)->vdisp <= mq->maxheight	)
	{
	     
	     
	    mq->dims_ok = TRUE;

	    if (NULL != mq->stdpfs)
	    {
		register HIDDT_StdPixFmt *stdpf = mq->stdpfs;
		while (*stdpf)
		{
		    if (*stdpf == PF(pf)->stdpixfmt)
		    {
		       	mq->stdpfs_ok  = TRUE;
		    }
		    stdpf ++;
		}
	    }
	    else
	    {
	    	mq->stdpfs_ok = TRUE;
	    }
	}
    }
    
    
    if (mq->dims_ok && mq->stdpfs_ok && mq->check_ok)
    {
	mode_ok = TRUE;
	mq->numfound ++;
    }
    
    mq->pfidx ++;
    
    modeids = querymode(mq);

    if (NULL == modeids)
	return NULL;
	
    if (mode_ok)
    {
	/* The mode is OK. Add it to the list */
	modeids --;
	*modeids = COMPUTE_HIDD_MODEID(syncidx, pfidx);
    }
    
    return modeids;
	
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_QueryModeIDs

    SYNOPSIS
        HIDDT_ModeID *OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_QueryModeIDs *msg);

	HIDDT_ModeID *HIDD_Gfx_QueryModeIDs(OOP_Object *gfxHidd, struct TagItem *queryTags);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Obtain a table of all supported display mode IDs
    
	The returned address points to an array of HIDDT_ModeID containing all ModeIDs
	supported by this driver. The array is terminated with vHidd_ModeID_Invalid.

    INPUTS
	gfxHidd   - A driver object which to query.
	querytags - An optional taglist containing query options. Can be NULL.
                    The following tags are supported:

		    tHidd_GfxMode_MinWidth  (ULONG) - A minimum width of modes you are
		                                      interested in
		    tHidd_GfxMode_MaxWidth  (ULONG) - A maximum width of modes you are
		                                      interested in
		    tHidd_GfxMode_MinHeight (ULONG) - A minimum height of modes you are
		                                      interested in
		    tHidd_GfxMode_MaxHeight (ULONG) - A maximum height of modes you are
		                                      interested in
		    tHidd_GfxMode_PixFmts   (HIDDT_StdPifXmt *) - A pointer to an array
			of standard pixelformat indexes. If supplied, only mode IDs whose
			pixelformat numbers match any of given ones will be returned.

    RESULT
	A pointer to an array of ModeIDs or NULL in case of failure

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Gfx_ReleaseModeIDs, moHidd_Gfx_NextModeID

    INTERNALS

*****************************************************************************************/

HIDDT_ModeID *GFX__Hidd_Gfx__QueryModeIDs(OOP_Class *cl, OOP_Object *o,
    	    	    	    	    	  struct pHidd_Gfx_QueryModeIDs *msg)
{
    struct TagItem  	    *tag, *tstate;
    
    HIDDT_ModeID    	    *modeids;
    struct HIDDGraphicsData *data;
    struct mode_db  	    *mdb;
    
    struct modequery 	    mq =
    {
	NULL,		/* mode db (set later)	*/
	0, 0xFFFFFFFF, 	/* minwidth, maxwidth	*/
	0, 0xFFFFFFFF,	/* minheight, maxheight	*/
	NULL,		/* stdpfs		*/
	0,		/* numfound		*/
	0, 0,		/* pfidx, syncidx	*/
	FALSE, FALSE,	/* dims_ok, stdpfs_ok	*/
	FALSE,		/* check_ok		*/
	NULL		/* class (set later)	*/
	
    };
    

    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;
    mq.mdb = mdb;
    mq.cl  = cl;
    
    for (tstate = msg->queryTags; (tag = NextTagItem((const struct TagItem **)&tstate)); )
    {
	switch (tag->ti_Tag)
	{
	    case tHidd_GfxMode_MinWidth:
	    	mq.minwidth = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MaxWidth:
	    	mq.maxwidth = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MinHeight:
	    	mq.minheight = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MaxHeight:
	    	mq.maxheight = (ULONG)tag->ti_Tag;
		break;
		
	    case tHidd_GfxMode_PixFmts:
	    	mq.stdpfs = (HIDDT_StdPixFmt *)tag->ti_Data;
		break;

	}
    }

    ObtainSemaphoreShared(&mdb->sema);

    /* Recursively check all modes */    
    modeids = querymode(&mq);
    
    ReleaseSemaphore(&mdb->sema);
    
    return modeids;
     
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_ReleaseModeIDs

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_ReleaseModeIDs *msg);

	VOID HIDD_Gfx_ReleaseModeIDs(OOP_Object *gfxHidd, HIDDT_ModeID *modeIDs);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Free array of display mode IDs returned by HIDD_Gfx_QueryModeIDs()

    INPUTS
	gfxHidd - A driver object used to obtain the array
	modeIDs - A pointer to an array

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_QueryModeIDs

    INTERNALS

*****************************************************************************************/

VOID GFX__Hidd_Gfx__ReleaseModeIDs(OOP_Class *cl, OOP_Object *o,
    	    	    	    	   struct pHidd_Gfx_ReleaseModeIDs *msg)
{
    FreeVec(msg->modeIDs);
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_NextModeID

    SYNOPSIS
        HIDDT_ModeID OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_NextModeID *msg);

	HIDDT_ModeID HIDD_Gfx_NextModeID(OOP_Object *gfxHidd, HIDDT_ModeID modeID,
                                         OOP_Object **syncPtr, OOP_Object **pixFmtPtr);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Iterate driver's internal display mode database.

    INPUTS
	gfxHidd   - A driver object to query
	modeID    - A previous mode ID or vHidd_ModeID_Invalid for start of the iteration
	syncPtr   - A pointer to a storage where pointer to sync object will be placed
	pixFmtPtr - A pointer to a storage where pointer to pixelformat object will be placed

    RESULT
        Next available mode ID or vHidd_ModeID_Invalid if there are no more display modes.
	If the function returns vHidd_ModeID_Invalid, sync and pixelformat pointers will
	be set to NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_GetMode

    INTERNALS

*****************************************************************************************/

HIDDT_ModeID GFX__Hidd_Gfx__NextModeID(OOP_Class *cl, OOP_Object *o,
    	    	    	    	       struct pHidd_Gfx_NextModeID *msg)
{
    struct HIDDGraphicsData *data;
    struct mode_db  	    *mdb;
    ULONG   	    	    syncidx, pfidx;
    HIDDT_ModeID    	    return_id = vHidd_ModeID_Invalid;
    BOOL    	    	    found = FALSE;
    
    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;

    ObtainSemaphoreShared(&mdb->sema);    
    if (vHidd_ModeID_Invalid == msg->modeID)
    {
	pfidx	= 0;
	syncidx = 0;	
    }
    else
    {
	pfidx 	= MODEID_TO_PFIDX( msg->modeID );
	syncidx	= MODEID_TO_SYNCIDX( msg->modeID );

	/* Increament one from the last call */
	pfidx ++;
	if (pfidx >= mdb->num_pixfmts)
	{
	    pfidx = 0;
	    syncidx ++;
	}
    }
    
    /* Search for a new mode. We only accept valid modes */
    for (; syncidx < mdb->num_syncs; syncidx ++)
    {
	/* We only return valid modes */
	for (; pfidx < mdb->num_pixfmts; pfidx ++)
	{
	    if (is_valid_mode(&mdb->checked_mode_bm, syncidx, pfidx))
	    {
		found = TRUE;
		break;
	    }
	}
	if (found)
	    break;
    }
    
    if (found)
    {
	return_id = COMPUTE_HIDD_MODEID(syncidx, pfidx);
	*msg->syncPtr	= mdb->syncs[syncidx];
	*msg->pixFmtPtr	= mdb->pixfmts[pfidx];
    }
    else
    {
	*msg->syncPtr = *msg->pixFmtPtr = NULL;
    }
	
    ReleaseSemaphore(&mdb->sema);
    
    return return_id;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_GetMode

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_GetMode *msg);

	BOOL HIDD_Gfx_GetMode(OOP_Object *gfxHidd, HIDDT_ModeID modeID,
	                      OOP_Object **syncPtr, OOP_Object **pixFmtPtr);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Get sync and pixelformat objects for a particular display ModeID.

    INPUTS
	gfxHidd   - pointer to a driver object which this ModeID belongs to
	syncPtr   - pointer to a storage where sync object pointer will be placed
	pixFmtPtr - pointer to a storage where pixelformat object pointer will be placed

    RESULT
	TRUE upon success, FALSE in case of failure (e.g. given mode does not exist in
	driver's internal database). If the function returns FALSE, sync and pixelformat
	pointers will be set to NULL.

    NOTES
	Every display mode is associated with some sync and pixelformat object. If the
	method returns TRUE, object pointers are guaranteed to be valid.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_NextModeID

    INTERNALS

*****************************************************************************************/

BOOL GFX__Hidd_Gfx__GetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetMode *msg)
{
    ULONG   	    	    pfidx, syncidx;
    struct HIDDGraphicsData *data;
    struct mode_db  	    *mdb;
    BOOL    	    	    ok = FALSE;
    
    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;
    
    pfidx	= MODEID_TO_PFIDX(msg->modeID);
    syncidx	= MODEID_TO_SYNCIDX(msg->modeID);
    
    ObtainSemaphoreShared(&mdb->sema);
    
    if (! (pfidx >= mdb->num_pixfmts || syncidx >= mdb->num_syncs) )
    {
	if (is_valid_mode(&mdb->checked_mode_bm, syncidx, pfidx))
	{
	    ok = TRUE;
	    *msg->syncPtr	= mdb->syncs[syncidx];
	    *msg->pixFmtPtr	= mdb->pixfmts[pfidx];
	}
    }
    
    ReleaseSemaphore(&mdb->sema);
    
    if (!ok)
    {
	*msg->syncPtr = *msg->pixFmtPtr = NULL;
    }
    
    return ok;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_SetMode

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_SetMode *msg);

	BOOL HIDD_Gfx_SetMode(OOP_Object *gfxHidd, OOP_Object *sync);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Update display mode according to changed sync object

    INPUTS
	gfxHidd - A display driver to operate on
	sync    - A modified sync object pointer

    RESULT
	TRUE if everything went OK and FALSE in case of some error

    NOTES
	This method is used to inform the driver that some external program has changed
	sync data and wants to update the display if needed. It's up to the implementation to
	check that current display is really using this sync (frontmost screen uses this mode).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Base class implementation just returns FALSE indicating that this method is
	not supported.

*****************************************************************************************/

BOOL GFX__Hidd_Gfx__SetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetMode *msg)
{
    return FALSE;
}

/****************************************************************************************/

static VOID copy_bm_and_colmap(OOP_Class *cl, OOP_Object *o,  OOP_Object *src_bm,
    	    	    	       OOP_Object *dst_bm, ULONG width, ULONG height)
{
    struct HIDDGraphicsData *data;
    ULONG   	    	    i;
    IPTR                    numentries;
    OOP_Object      	    *src_colmap;
    APTR		    psrc_colmap = &src_colmap;
    
    data = OOP_INST_DATA(cl, o);
    
    /* We have to copy the colormap into the framebuffer bitmap */
    OOP_GetAttr(src_bm, aHidd_BitMap_ColorMap, (IPTR *)psrc_colmap);
    OOP_GetAttr(src_colmap, aHidd_ColorMap_NumEntries, &numentries);
	
    for (i = 0; i < numentries; i ++)
    {
    	HIDDT_Color col;
	
	HIDD_CM_GetColor(src_colmap, i, &col);
	HIDD_BM_SetColors(dst_bm, &col, i, 1);
    }    

    HIDD_Gfx_CopyBox(o
    	, src_bm
	, 0, 0
    	, dst_bm
	, 0, 0
	, width, height
	, data->gc
    );
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_Show

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_Show *msg);

	OOP_Object *HIDD_Gfx_Show(OOP_Object *gfxHidd, OOP_Object *bitMap, ULONG flags);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Change currently displayed bitmap on the screen.

	The bitmap object supplied must have been created with aHidd_BitMap_Displayable
	attribute set to TRUE.

	The function's behavior differs a lot depending on whether the driver uses a
	framebuffer or video hardware is able to switch screens itself.

	If the driver uses a framebuffer bitmap, it is supposed to copy the supplied bitmap
	into the framebuffer and return a framebuffer pointer. It also can be asked to
	copy back old framebuffer contents into previous bitmap object. It is driver's
	job to keep track of which bitmap object was displayed last time. This is what
	default implementation does. Note that it is very basic, and even does not support
	changing display resolution. It's not recommended to rely on it in production
	drivers (unless your video hardware supports only one mode).

	If the driver does not use a framebuffer, it is supposed to reprogram the hardware
	here to display an appropriate region of video RAM. Do not call the base class
	in this case, its implementation relies on framebuffer existance and will always
	return NULL which indicates an error.

	It is valid to get NULL value in bitMap parameter. This means that there is
	nothing to display and the screen needs to be blanked out. It is valid for
	non-framebuffer-based driver to return NULL as a reply then. In all other cases
	NULL return value means an error.

	Please avoid returning errors at all. graphics.library/LoadView() has no error
	indication. An error during showing a bitmap would leave the display in
	unpredictable state.

	If the driver does not use a framebuffer, consider using HIDD_Gfx_ShowViewPorts().
	It's more straightforward, flexible and offers support for screen composition.

    INPUTS
	gfxHidd - a display driver object, whose display you wish to change.
	bitMap  - a pointer to a bitmap object which needs to be shown or NULL.
	flags   - currently only one flag is defined:

	fHidd_Gfx_Show_CopyBack - Copy back the image data from framebuffer bitmap
				  to old displayed bitmap. Used only if the driver
				  needs a framebuffer.

    RESULT
	A pointer to a currently displayed bitmap object or NULL (read FUNCTION paragraph for
	detailed description)

    NOTES
	Drivers which use mirrored video data buffer do not have to update the display
	immediately in this method. moHidd_BitMap_UpdateRect will be sent to the returned
	bitmap if it's not NULL. Of course display blanking (if NULL bitmap was received)
	needs to be performed immediately.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_ShowViewPorts, graphics.library/LoadView()

    INTERNALS

*****************************************************************************************/

OOP_Object *GFX__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    struct HIDDGraphicsData *data;
    OOP_Object      	    *bm;
    IPTR    	    	    displayable;
    IPTR		    oldwidth  = 0;
    IPTR		    oldheight = 0;
    IPTR		    newwidth  = 0;
    IPTR		    newheight = 0;
    struct TagItem  	    gctags[] =
    {
    	{ aHidd_GC_DrawMode  , vHidd_GC_DrawMode_Copy},
	{ aHidd_GC_Foreground, 0		     },
	{ TAG_DONE  	     , 0UL   	    	     }
    };
    
    data = OOP_INST_DATA(cl, o);
    bm = msg->bitMap;
    
    /* We have to do some consistency checking */
    if (bm)
    {
    	OOP_GetAttr(bm, aHidd_BitMap_Displayable, &displayable);
    }
    
    if (bm && !displayable)
    	/* We cannot show a non-displayable bitmap */
	return NULL;
	
    if (NULL == data->framebuffer)
	return NULL;

    OOP_SetAttrs(data->gc, gctags);
    if (NULL != data->shownbm)
    {
	OOP_GetAttr(data->shownbm, aHidd_BitMap_Width, &oldwidth);
        OOP_GetAttr(data->shownbm, aHidd_BitMap_Height, &oldheight);
	/* Copy the framebuffer data back into the old shown bitmap */
	if (msg->flags & fHidd_Gfx_Show_CopyBack)
	    copy_bm_and_colmap(cl, o, data->framebuffer, data->shownbm, oldwidth, oldheight);
    }

    if (bm) {
        OOP_GetAttr(bm, aHidd_BitMap_Width, &newwidth);
        OOP_GetAttr(bm, aHidd_BitMap_Height, &newheight);
    	copy_bm_and_colmap(cl, o, bm, data->framebuffer, newwidth, newheight);
    }
    /* Clear remaining parts of the framebuffer (if previous bitmap was larger than new one) */
    if (oldheight) {
        if (newwidth < oldwidth)
	    HIDD_BM_FillRect(data->framebuffer, data->gc, newwidth, 0, oldwidth - 1, oldheight - 1);
        if ((newheight < oldheight) && newwidth)
	    HIDD_BM_FillRect(data->framebuffer, data->gc, 0, newheight, newwidth - 1, oldheight);
    }

    data->shownbm = bm;

    return data->framebuffer;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_ShowViewPorts

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_ShowViewPorts *msg);

	ULONG HIDD_Gfx_ShowViewPorts(OOP_Object *gfxHidd, struct HIDD_ViewPortData *data, struct View *view);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Show one or more bitmaps on the screen.

	It is completely up to the driver how to implement this function. The driver may
	or may not support hardware-assisted screens composition. Bitmaps are sorted
	in the list in descending z-order. The driver is expected to put at least frontmost
	bitmap on display.

	It is valid to get NULL pointer as data parameter. This means that there's
	nothing to show and the screen should go blank.

	Bitmaps display offsets are stored in their aHidd_BitMap_LeftEdge and
	aHidd_BitMap_TopEdge attributes. This function is not expected to modify their
	values somehow. They are assumed to be preserved between calls unless changed
	explicitly by the system.

	If you implement this method, you don't have to implement HIDD_Gfx_Show() because
	it will never be called.

	Note that there is no more error indication - the driver is expected to be
	error-free here.

    INPUTS
	gfxHidd - a display driver object, whose display you wish to change.
	data    - a singly linked list of bitmap objects to show

    RESULT
	TRUE if this method is supported by the driver, FALSE otherwise

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_Show

    INTERNALS
	Default base class implementation simply returns FALSE. This causes
	the system to use HIDD_Gfx_Show() instead.

*****************************************************************************************/

ULONG GFX__Hidd_Gfx__ShowViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ShowViewPorts *msg)
{
    /* By default we don't support screen composition (and this method too) */
    return FALSE;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_SetCursorShape

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_SetCursorShape *msg);

	BOOL HIDD_Gfx_SetCursorShape(OOP_Object *gfxHidd, OOP_Object *shape,
	                             LONG xoffset, LONG yoffset);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Set mouse pointer shape.

	A pointer image is contained in the specified bitmap object. The bitmap object
	may contain a colormap if the system wants to specify own colors for the pointer.
	The supplied colormap will also contain alpha channel values.

	It is up to driver what to do if, for example, alpha channel is not supported by
	the hardware. Or if given bitmap type is not supported (for example truecolor
	bitmap on LUT-only hardware). It is expected that the driver converts bitmap
	data to a more appropriate form in such a case.

	A hotspot is given as an offset from the actual hotspot to the top-left corner
	of the pointer image. It is generally needed only for hosted display drivers
	which utilize host's support for mouse pointer.

	The default implementation in the base class just does nothing. A software mouse
	pointer is implemented in a special layer calles fakegfx.hidd inside
	graphics.library. If a software pointer emulation is used, this method will
	never be called.

    INPUTS
	gfxHidd - a display driver object, for whose display you wish to change the pointer
	shape   - a pointer to a bitmap object, containing pointer bitmap
	xoffset - a horizontal hotspot offset
	yoffset - a vertical hotspot offset

    RESULT
	TRUE on success, FALSE on failure

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_SetCursorPos, moHidd_Gfx_SetCursorVisible

    INTERNALS

*****************************************************************************************/

BOOL GFX__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o,
    	    	    	    	   struct pHidd_Gfx_SetCursorShape *msg)
{
    /* We have no clue how to render the cursor */
    return TRUE;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_SetCursorVisible

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_SetCursorVisible *msg);

	VOID HIDD_Gfx_SetCursorVisible(OOP_Object *gfxHidd, BOOL visible);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Control mouse pointer visiblity.

	The default implementation in the base class does nothing. If a software pointer
	emulation is used, this method will never be called.

    INPUTS
	gfxHidd - a display driver object, on whose display you wish to turn pointer or on off
	visible - TRUE to enable pointer display, FALSE to disable it

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_SetCursorPos, moHidd_Gfx_SetCursorVisible

    INTERNALS

*****************************************************************************************/

VOID GFX__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{

}

/*****************************************************************************************

    NAME
	moHidd_Gfx_SetCursorPos

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_SetCursorPos *msg);

	BOOL HIDD_Gfx_SetCursorPos(OOP_Object *gfxHidd, LONG x, LONG y);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Set current mouse pointer position.

	This is a real position on top-left image corner relative to top-left corner of
	the physical display. Neither logical screen origin nor hotspot are taken into
	account here.

	The default implementation in the base class does nothing and just returns TRUE.
	If a software pointer emulation is used, this method will never be called.

    INPUTS
	gfxHidd - a display driver object, on whose display you wish to position the pointer
	x       - An x coordinate of the pointer (relative to the physical screen origin)
	y       - An y coordinate of the pointer (relative to the physical screen origin)

    RESULT
	Always TRUE. Reserved for future, do not use it.

    NOTES
	This method is called by graphics.library/MoveSprite() which has no return value.
	However, for historical reasons, this method has a return value. Drivers should
	always return TRUE in order to ensure future compatibility.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_SetCursorShape, moHidd_Gfx_SetCursorVisible, graphics.library/MoveSprite()

    INTERNALS

*****************************************************************************************/

BOOL GFX__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    return TRUE;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_CopyBox

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_CopyBox *msg);

	VOID HIDD_Gfx_CopyBox(OOP_Object *gfxHidd, OOP_Object *src, WORD srcX, WORD srcY,
	                      OOP_Object *dest, WORD destX, WORD destY, UWORD width, UWORD height, 
	                      OOP_Object *gc);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Perform rectangle copy (blit) operation from one bitmap to another.
    
	Given bitmaps may belong to different display drivers. The driver may attempt to
	use hardware for acceleration (if available), and if it's impossible, pass the
	operation on to the base class.
    
	Always check class of the supplied bitmap before attempting to look at its
	private data.
    
	A GC is used in order to specify raster operation performed between the source
	and destination according to its aHidd_GC_DrawMode attribute value.

    INPUTS
	gfxHidd - a display driver object that you are going to use for copying
	src     - a pointer to source bitmap object
	srcX    - an X coordinate of the source rectangle
	srcY    - an Y coordinate of the source rectangle
	dest    - a pointer to destination bitmap object
	destX   - an X coordinate of the destination rectangle
	destY   - an Y coordinate of the destination rectangle
	width   - width of the rectangle to copy
	height  - height of the rectangle to copy
	gc      - graphics context holding draw mode on the destination

    RESULT
	None.

    NOTES
	You must specify valid coordinates (non-negative and inside the actual bitmap
	area), no checks are done.
	
	It is valid to specify two overlapped areas of the same bitmap as source
	and destination.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID GFX__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *obj, struct pHidd_Gfx_CopyBox *msg)
{
    WORD    	    	    	    x, y;
    WORD    	    	    	    srcX = msg->srcX, destX = msg->destX;
    WORD    	    	    	    srcY = msg->srcY, destY = msg->destY;
    WORD    	    	    	    startX, endX, deltaX, startY, endY, deltaY;
    ULONG   	    	    	    memFG;
    
    HIDDT_PixelFormat 	    	    *srcpf, *dstpf;
    OOP_Object      	    	    *dest, *src;
    
    OOP_Object      	    	    *gc;
#if USE_FAST_GETPIXEL
    struct pHidd_BitMap_GetPixel    get_p;
#endif

#if USE_FAST_DRAWPIXEL
    struct pHidd_BitMap_DrawPixel   draw_p;
    
    draw_p.mID	= CSD(cl)->drawpixel_mid;
    draw_p.gc	= msg->gc;
#endif

#if USE_FAST_GETPIXEL
    get_p.mID	= CSD(cl)->getpixel_mid;
#endif
    
    dest = msg->dest;
    src  = msg->src;

    /* If source/dest overlap, direction of operation is important */
    
    if (srcX < destX)
    {
    	startX = msg->width - 1;  endX = -1; deltaX = -1;
    }
    else
    {
    	startX = 0; endX = msg->width;  deltaX = 1;
    }
 
    if (srcY < destY)
    {
	startY = msg->height - 1; endY = -1; deltaY = -1;
    }
    else
    {
	startY = 0; endY = msg->height; deltaY = 1;
    }
    
    /* Get the source pixel format */
    srcpf = (HIDDT_PixelFormat *)HBM(src)->prot.pixfmt;
    
    /* bug("COPYBOX: SRC PF: %p, obj=%p, cl=%s, OOP_OCLASS: %s\n", srcpf, obj
	    , cl->ClassNode.ln_Name, OOP_OCLASS(obj)->ClassNode.ln_Name);
    */
    
#if 0
    {
	IPTR sw, sh, dw, dh;
	D(bug("COPYBOX: src=%p, dst=%p, width=%d, height=%d\n"
	    , obj, msg->dest, msg->width, msg->height));

	OOP_GetAttr(obj, aHidd_BitMap_Width, &sw);
	OOP_GetAttr(obj, aHidd_BitMap_Height, &sh);
	OOP_GetAttr(msg->dest, aHidd_BitMap_Width, &dw);
	OOP_GetAttr(msg->dest, aHidd_BitMap_Height, &dh);
	D(bug("src dims: %d, %d  dest dims: %d, %d\n", sw, sh, dw, dh));
    }
#endif

    dstpf = (HIDDT_PixelFormat *)HBM(dest)->prot.pixfmt;
    
    /* Compare graphtypes */
    if (HIDD_PF_COLMODEL(srcpf) == HIDD_PF_COLMODEL(dstpf))
    {
    	/* It is ok to do a direct copy */
    }
    else
    {
    	/* Find out the gfx formats */
	if (  IS_PALETTIZED(srcpf) && IS_TRUECOLOR(dstpf))
	{
	
	}
	else if (IS_TRUECOLOR(srcpf) && IS_PALETTIZED(dstpf))
	{
	
	}
	else if (IS_PALETTE(srcpf) && IS_STATICPALETTE(dstpf)) 
	{
	
	}
	else if (IS_STATICPALETTE(srcpf) && IS_PALETTE(dstpf))
	{
	
	}
    }
    
    gc = msg->gc;
    
    memFG = GC_FG(msg->gc);
    
    /* All else have failed, copy pixel by pixel */


    if (HIDD_PF_COLMODEL(srcpf) == HIDD_PF_COLMODEL(dstpf))
    {
    	if (IS_TRUECOLOR(srcpf))
	{
    	    // bug("COPY FROM TRUECOLOR TO TRUECOLOR\n");
	    for(y = startY; y != endY; y += deltaY)
	    {
		HIDDT_Color col;
		
		/* if (0 == strcmp("CON: Window", FindTask(NULL)->tc_Node.ln_Name))
		    bug("[%d,%d] ", memSrcX, memDestX);
		*/    
		for(x = startX; x != endX; x += deltaX)
		{
		    HIDDT_Pixel pix;
		    
    	    	#if USE_FAST_GETPIXEL
		    get_p.x = srcX + x;
		    get_p.y = srcY + y;
		    pix = GETPIXEL(src, &get_p);
    	    	#else
		    pix = HIDD_BM_GetPixel(obj, srcX + x, srcY + y);
    	    	#endif

    	    	#if COPYBOX_CHECK_FOR_ALIKE_PIXFMT
		    if (srcpf == dstpf)
		    {
			GC_FG(gc) = pix;
		    } 
		    else 
		    {
    	    	#endif
		    HIDD_BM_UnmapPixel(src, pix, &col);
		    GC_FG(gc) = HIDD_BM_MapColor(msg->dest, &col);
    	    	#if COPYBOX_CHECK_FOR_ALIKE_PIXFMT
		    }
    	    	#endif

    	    // #if 0

    	    	#if USE_FAST_DRAWPIXEL
		    draw_p.x = destX + x;
		    draw_p.y = destY + y;
		    DRAWPIXEL(dest, &draw_p);
    	    	#else
		    
		    HIDD_BM_DrawPixel(msg->dest, gc, destX + x, destY + y);
    	    	#endif

    	    // #endif
		}
		/*if (0 == strcmp("CON: Window", FindTask(NULL)->tc_Node.ln_Name))
		    bug("[%d,%d] ", srcY, destY);
		*/
	    }
	    
        } /* if (IS_TRUECOLOR(srcpf)) */
	else
	{
	     /* Two palette bitmaps.
	        For this case we do NOT convert through RGB,
		but copy the pixel indexes directly
	     */
    	    // bug("COPY FROM PALETTE TO PALETTE\n");

    	    /* FIXME: This might not work very well with two StaticPalette bitmaps */

	    for(y = startY; y != endY; y += deltaY)
	    {
		for(x = startX; x != endX; x += deltaX)
		{
		    GC_FG(gc) = HIDD_BM_GetPixel(src, srcX + x, srcY + y);
		    
		    HIDD_BM_DrawPixel(msg->dest, gc, destX + x, destY + y);
		    
		}
 	    }
	     
	} /* if (IS_TRUECOLOR(srcpf)) else ... */

    } /* if (HIDD_PF_COLMODEL(srcpf) == HIDD_PF_COLMODEL(dstpf)) */
    else
    {
    	/* Two unlike bitmaps */
	if (IS_TRUECOLOR(srcpf))
	{
    	    /* FIXME: Implement this */
	     D(bug("!! DEFAULT COPYING FROM TRUECOLOR TO PALETTIZED NOT IMPLEMENTED IN BitMap::CopyBox\n"));
	}
	else if (IS_TRUECOLOR(dstpf))
	{
	    /* Get the colortab */
	    HIDDT_Color *ctab = ((HIDDT_ColorLUT *)HBM(src)->colmap)->colors;

    	    // bug("COPY FROM PALETTE TO TRUECOLOR, DRAWMODE %d, CTAB %p\n", GC_DRMD(gc), ctab);
	    
	    for(y = startY; y != endY; y += deltaY)
	    {		
		for(x = startX; x != endX; x += deltaX)
		{
		    register HIDDT_Pixel pix;
		    register HIDDT_Color *col;
		    
		    pix = HIDD_BM_GetPixel(src, srcX + x, srcY + y);
		    col = &ctab[pix];
	
		    GC_FG(gc) = HIDD_BM_MapColor(msg->dest, col);
		    HIDD_BM_DrawPixel(msg->dest, gc, destX + x, destY + y);
		    
		}
	    }	
	}
	
    } /* if (HIDD_PF_COLMODEL(srcpf) == HIDD_PF_COLMODEL(dstpf)) else ... */
    
    GC_FG(gc) = memFG;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_ShowImminentReset

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, OOP_Msg msg);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Indicate upcoming machine reset. Obsolete.

	Since graphics.library v41.4 this method is not used any more. Considered
	reserved. Do not use it in any way.

    INPUTS
	None.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID GFX__Hidd_Gfx__ShowImminentReset(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
}

/****************************************************************************************/

OOP_Object *GFX__Hidd_Gfx__RegisterPixFmt(OOP_Class *cl, OOP_Object *o,
    	    	    	    	    	  struct pHidd_Gfx_RegisterPixFmt *msg)
{
    HIDDT_PixelFormat 	    cmp_pf;
    struct class_static_data *data;
    struct pixfmt_data 	    *retpf = NULL;

    memset(&cmp_pf, 0, sizeof(cmp_pf));

    data = CSD(cl);
    if (!parse_pixfmt_tags(msg->pixFmtTags, &cmp_pf, 0, CSD(cl)))
    {
    	D(bug("!!! FAILED PARSING TAGS IN Gfx::RegisterPixFmt() !!!\n"));
	return FALSE;
    }

    DPF(bug("Gfx::RegisterPixFmt(): Registering pixelformat:\n"));
    DPF(bug("(%d, %d, %d, %d), (%x, %x, %x, %x), %d, %d, %d, %d\n"
	  , PF(&cmp_pf)->red_shift
	  , PF(&cmp_pf)->green_shift
	  , PF(&cmp_pf)->blue_shift
	  , PF(&cmp_pf)->alpha_shift
	  , PF(&cmp_pf)->red_mask
	  , PF(&cmp_pf)->green_mask
	  , PF(&cmp_pf)->blue_mask
	  , PF(&cmp_pf)->alpha_mask
	  , PF(&cmp_pf)->bytes_per_pixel
	  , PF(&cmp_pf)->size
	  , PF(&cmp_pf)->depth
	  , PF(&cmp_pf)->stdpixfmt));

    retpf = find_pixfmt(&cmp_pf, CSD(cl));

    DPF(bug("Found matching pixelformat: 0x%p\n", retpf));
    if (retpf)
	/* Increase pf refcount */
	AROS_ATOMIC_INC(retpf->refcount);
    else {
    	/* Could not find an alike pf, Create a new pfdb node  */	    
	/* Since we pass NULL as the taglist below, the PixFmt class will just create a dummy pixfmt */
	retpf = OOP_NewObject(CSD(cl)->pixfmtclass, NULL, NULL);
	if (retpf) {
	    /* We have one user */
	    retpf->refcount = 1;
		
	    /* Initialize the pixfmt object the "ugly" way */
	    memcpy(retpf, &cmp_pf, sizeof (HIDDT_PixelFormat));
		
	    DPF(bug("(%d, %d, %d, %d), (%x, %x, %x, %x), %d, %d, %d, %d\n"
			, PF(&cmp_pf)->red_shift
			, PF(&cmp_pf)->green_shift
			, PF(&cmp_pf)->blue_shift
			, PF(&cmp_pf)->alpha_shift
			, PF(&cmp_pf)->red_mask
			, PF(&cmp_pf)->green_mask
			, PF(&cmp_pf)->blue_mask
			, PF(&cmp_pf)->alpha_mask
			, PF(&cmp_pf)->bytes_per_pixel
			, PF(&cmp_pf)->size
			, PF(&cmp_pf)->depth
			, PF(&cmp_pf)->stdpixfmt));

    	    ObtainSemaphore(&data->pfsema);
	    AddTail((struct List *)&data->pflist, (struct Node *)&retpf->node);
    	    ReleaseSemaphore(&data->pfsema);
	    
	}
    }
    DPF(bug("New refcount is %u\n", retpf->refcount));
    return (OOP_Object *)retpf; 
}

/****************************************************************************************/

VOID GFX__Hidd_Gfx__ReleasePixFmt(OOP_Class *cl, OOP_Object *o,
    	    	    	    	  struct pHidd_Gfx_ReleasePixFmt *msg)
{
    struct class_static_data *data;
    struct pixfmt_data *pixfmt = (struct pixfmt_data *)msg->pixFmt;

    DPF(bug("release_pixfmt 0x%p\n", pixfmt));

    data = CSD(cl);

    ObtainSemaphore(&data->pfsema);

    /* If refcount is already 0, this object was never registered in the database,
       don't touch it */
    DPF(bug("Old reference count is %u\n", pixfmt->refcount));
    if (pixfmt->refcount) {
        if (--pixfmt->refcount == 0) {
	    Remove((struct Node *)&pixfmt->node);
	    OOP_DisposeObject((OOP_Object *)pixfmt);
	}
    }

    ReleaseSemaphore(&data->pfsema);
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_CheckMode

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_CheckMode *msg);

	BOOL HIDD_Gfx_CheckMode(OOP_Object *gfxHidd, HIDDT_ModeID modeID,
	                        OOP_Object *sync, OOP_Object *pixFmt);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Check if given display mode is supported by the driver.

	Normally any resolution (sync) can be used together with any pixelformat. However
	on some hardware there may be exceptions from this rule. In such a case this
	method should be implemented, and check should be performed.

	The information provided by this method is used in order to exclude unsupported
	modes from the database

	Default implementation in the base class just returns TRUE for all supplied values.

	Note that this method can not be used in order to chech that the given mode is
	really present in the database and it really refers to the given sync and
	pixelformat objects. Use HIDD_Gfx_GetMode() for mode ID validation.

    INPUTS
	gfxHidd - A display driver object
	modeID  - A display mode ID
	sync    - A pointer to a sync object associated with this mode
	pixFmt  - A pointer to a pixelformat object associated with this mode

    RESULT
	TRUE if this mode is supported and FALSE if it's not.

    NOTES

    EXAMPLE

    BUGS
	Currently base class does not call this method after driver object creation.
	This needs to be fixed.

    SEE ALSO
	moHidd_Gfx_GetMode

    INTERNALS

*****************************************************************************************/

BOOL GFX__Hidd_Gfx__CheckMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CheckMode *msg)
{
    /* As a default we allways return TRUE, ie. the mode is OK */
    return TRUE;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_GetPixFmt

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *o, struct pHidd_Gfx_GetPixFmt *msg); 

	OOP_Object *HIDD_Gfx_GetPixFmt(OOP_Object *gfxHidd, HIDDT_StdPixFmt pixFmt);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Get a standard pixelformat descriptor from internal pixelformats database.

    INPUTS
	gfxHidd - A display driver object
	pixFmt  - An index of pixelformat (one of vHIDD_StdPixFmt_... values)

    RESULT
	A pointer to a pixelformat object or NULL if lookup failed

    NOTES
	Pixelformat objects are stored in a global system-wide database. They are not
	linked with a particular driver in any way and completely sharable between all
	drivers.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	This operation can never fail because all standard pixelformats are registered
	during early system initialization.

*****************************************************************************************/

OOP_Object *GFX__Hidd_Gfx__GetPixFmt(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetPixFmt *msg) 
{
    OOP_Object *fmt;
    
    if (!IS_REAL_STDPIXFMT(msg->stdPixFmt))
    {
    	D(bug("!!! Illegal pixel format passed to Gfx::GetPixFmt(): %d\n", msg->stdPixFmt));
	return NULL;
    } 
    else 
    {
    	fmt = (OOP_Object *)CSD(cl)->std_pixfmts[REAL_STDPIXFMT_IDX(msg->stdPixFmt)];
    }

    return fmt;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_GetSync

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_GetSync *msg);

	OOP_Object *HIDD_Gfx_GetSync(OOP_Object *gfxHidd, ULONG num);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Get a sync object from internal display mode database by index

    INPUTS
	gfxHidd - A display driver object to query
	num     - An index of sync object starting from 0

    RESULT
	A pointer to a sync object or NULL if there's no sync with such index

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

OOP_Object *GFX__Hidd_Gfx__GetSync(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetSync *msg) 
{
    struct HIDDGraphicsData *data = OOP_INST_DATA(cl, o);
    
    if (msg->num < data->mdb.num_syncs)
    	return data->mdb.syncs[msg->num];
    else {
    	D(bug("!!! Illegal sync index passed to Gfx::GetSync(): %d\n", msg->num));
	return NULL;
    }
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_ModeProperties

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_ModeProperties *msg);

	ULONG HIDD_Gfx_ModeProperties(OOP_Object *gfxHidd, HIDDT_ModeID modeID,
	                              struct HIDD_ModeProperties *props, ULONG propsLen);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Obtain an information about the video mode.

	Video mode description structure may grow in future, so be careful and always check
	propsLen parameter value. A system may ask you for less data than you can provide.
	Always return an actual value. Do not just zero out fields you don't know about,
	this is not expected to be backwards compatible.

    INPUTS
	gfxHidd  - a pointer to a display driver object whose display mode you want to query
	modeID   - a mode ID to query
	props    - a pointer to a storage area where HIDD_ModeProperties structure will be put
	propsLen - A length of the supplied buffer in bytes.

    RESULT
	An actual length of obtained structure

    NOTES
	Returned data must reflect only real hardware capabilities. For example, do not
	count emulated sprites. The system takes care about emulated features itself.

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_HWSpriteTypes, aoHidd_Gfx_SupportsHWCursor

    INTERNALS
	Default implementation in the base class relies on aHidd_Gfx_HWSpriteTypes attribute,
	not vice versa. If you override this method, do not forget to override this attribute too.

*****************************************************************************************/

ULONG GFX__Hidd_Gfx__ModeProperties(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ModeProperties *msg)
{
    struct HIDD_ModeProperties props = {0, 0, 0};
    IPTR has_hw_cursor = 0;
    ULONG len = msg->propsLen;

    D(bug("[GFXHIDD] Hidd::Gfx::ModeProperties(0x%08lX, 0x%p, %u)\n", msg->modeID, msg->props, msg->propsLen));
    OOP_GetAttr(o, aHidd_Gfx_HWSpriteTypes, &has_hw_cursor);
    if (has_hw_cursor) {
	D(bug("[GFXHIDD] Driver has hardware mouse cursor implementation\n"));
        props.DisplayInfoFlags = DIPF_IS_SPRITES;
	props.NumHWSprites = 1;
    }

    if (len > sizeof(props))
        len = sizeof(props);
    D(bug("[GFXHIDD] Copying %u bytes\n", len));
    CopyMem(&props, msg->props, len);

    return len;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_GetGamma

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_Gamma *msg);

	BOOL HIDD_Gfx_GetGamma(OOP_Object *gfxHidd, UBYTE *Red, UBYTE *Green, UBYTE *Blue);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Get current gamma table for the display.

	A gamma table consists of three 256-byte tables: one for red component, one for
	green and one for blue.

	A user should supply three pointers to preallocated 256-byte tables which will
	be filled in. Any ot these pointers may have NULL value, in this case the
	respective component will be ignored.

    INPUTS
	gfxHidd - A display driver object
	Red     - A pointer to a 256-byte array for red component or NULL
	Green   - A pointer to a 256-byte array for green component or NULL
	Blue    - A pointer to a 256-byte array for blue component or NULL

    RESULT
	FALSE if the driver doesn't support gamma correction, otherwise TRUE

    NOTES
	This method can be used just to query if the driver supports gamma correction.
	Just set Red, Green and Blue to NULL for this.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_SetGamma

    INTERNALS

*****************************************************************************************/

BOOL GFX__Hidd_Gfx__GetGamma(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Gamma *msg)
{
    return FALSE;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_SetGamma

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_Gamma *msg);

	BOOL HIDD_Gfx_SetGamma(OOP_Object *gfxHidd, UBYTE *Red, UBYTE *Green, UBYTE *Blue);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Set current gamma table for the display.

	A gamma table consists of three 256-byte tables: one for red component, one for
	green and one for blue.

	A user should supply three pointers to 256-byte tables from which gamma values
	will be picked up. Any ot these pointers may have NULL value, in this case the
	respective component will be ignored.

    INPUTS
	gfxHidd - A display driver object
	Red     - A pointer to a 256-byte array for red component or NULL
	Green   - A pointer to a 256-byte array for green component or NULL
	Blue    - A pointer to a 256-byte array for blue component or NULL

    RESULT
	FALSE if the driver doesn't support gamma correction, otherwise TRUE

    NOTES
    	This method can be used just to query if the driver supports gamma correction.
	Just set Red, Green and Blue to NULL for this.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_GetGamma

    INTERNALS

*****************************************************************************************/

BOOL GFX__Hidd_Gfx__SetGamma(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Gamma *msg)
{
    return FALSE;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_QueryHardware3D

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_QueryHardware3D *msg);

	BOOL HIDD_Gfx_QueryHardware3D(OOP_Object *gfxHidd, OOP_Object *pixFmt);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Query if the driver supports hardware-accelerated 3D graphics for the given
	pixelformat.

    INPUTS
	gfxHidd - A display driver object
	pixFmt  - A pointer to a pixelformat descriptor object

    RESULT
	TRUE if the driver supports hardware-accelerated 3D for the given pixelformat,
	FALSE otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

BOOL GFX__Hidd_Gfx__QueryHardware3D(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_QueryHardware3D *msg)
{
    return FALSE;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_GetMaxSpriteSize

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_GetMaxSpriteSize *msg);

	BOOL HIDD_Gfx_GetMaxSpriteSize(OOP_Object *gfxHidd, ULONG Type, ULONG *Width, ULONG *Height);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Query maximum allowed size for the given sprite type.

    INPUTS
	gfxHidd - A display driver object
	Type	- Type of the sprite image (one of vHidd_SpriteType_... values)
	Width	- A pointer to ULONG where width will be placed.
	Height	- A pointer to ULONG where height will be placed.

    RESULT
	FALSE is the given sprite type is not supported, otherwise TRUE.

    NOTES
	Default implementation in the base class just return some small values
	which it hopes can be supported by every driver if the driver supports given
	sprite type. It is strongly suggested to reimplement this method in the display
	driver.

	Width and Height are considered undefined if the method returns FALSE.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

BOOL GFX__Hidd_Gfx__GetMaxSpriteSize(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetMaxSpriteSize *msg)
{
    IPTR types;

    OOP_GetAttr(o, aHidd_Gfx_HWSpriteTypes, &types);

    if (types & msg->Type) {
	*msg->Width  = 16;
	*msg->Height = 32;
	return TRUE;
    } else
	return FALSE;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_NewOverlay

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_NewOverlay *msg);

	OOP_Object *HIDD_Gfx_NewOverlay(OOP_Object *gfxHidd, struct TagItem *tagList);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Create a video overlay object.

    INPUTS
	gfxHidd - A graphics driver object on whose display you want to create an overlay.
	tagList - A list of overlay attributes. See overlay class documentation for
		  their description.

    RESULT
	Pointer to the newly created overlay object or NULL in case of failure.

    NOTES
	Default implementation in the base class always sets VOERR_INVSCRMODE error and
	returns NULL meaning that hardware overlays are not supported. There's no sense
	in software implementation because the software is supposed to handle software
	rendering itself.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_DisposeOverlay

    INTERNALS

*****************************************************************************************/

OOP_Object *GFX__Hidd_Gfx__NewOverlay(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewOverlay *msg)
{
    ULONG *err = (ULONG *)GetTagData(aHidd_Overlay_Error, 0, msg->attrList);

    if (err)
	*err = VOERR_INVSCRMODE;

    return NULL;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_DisposeOverlay

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_DisposeOverlay *msg);

	VOID HIDD_Gfx_DisposeOverlay(OOP_Object *gfxHidd, OOP_Object *Overlay)

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Deletes an overlay previously created by moHidd_Gfx_NewOverlay.

	Subclasses do not have to override this method
	unless they allocate anything additional to an overlay object in
	their HIDD_Gfx_NewOverlay() implementation.

    INPUTS
	gfxHidd - A driver object which was used for creating a GC.
	Overlay - Pointer to an overlay object to delete.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_NewGC

    INTERNALS
	Basically just does OOP_DisposeObject(Overlay);

*****************************************************************************************/

VOID GFX__Hidd_Gfx__DisposeOverlay(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_DisposeOverlay *msg)
{
    OOP_DisposeObject(msg->Overlay);
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_MakeViewPort

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_MakeViewPort *msg);

	ULONG HIDD_Gfx_MakeViewPort(OOP_Object *gfxHidd, struct HIDD_ViewPortData *data)

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Performs driver-specific setup on a given ViewPort.

    INPUTS
	gfxHidd - A display driver object.
	data    - a pointer to a HIDD_ViewPortDats structure.

    RESULT
	The same code as used as return value for graphics.library/MakeVPort().

    NOTES
    	When graphics.library calls this method, a complete view is not built yet.
    	This means that data->Next pointer contains invalid data and needs to be
    	ignored.

    	It is valid to keep private data pointer in data->UserData accross calls.
    	Newly created HIDD_ViewPortData is guraranteed to have NULL there.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_CleanViewPort

    INTERNALS
	Base class implementation just does nothing. This function is mainly intended
	to provide support for copperlist maintenance by Amiga(tm) chipset driver.

*****************************************************************************************/

ULONG GFX__Hidd_Gfx__MakeViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_MakeViewPort *msg)
{
    D(bug("Gfx::MakeViewPort: object 0x%p, data 0x%p\n", o, msg->Data));

    return MVP_OK;
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_CleanViewPort

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_CleanViewPort *msg);

	ULONG HIDD_Gfx_CleanViewPort(OOP_Object *gfxHidd, struct HIDD_ViewPortData *data)

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Performs driver-specific cleanup on a given ViewPort.

    INPUTS
	gfxHidd - A display driver object.
	data    - a pointer to a HIDD_ViewPortDats structure.

    RESULT
	The same code as used as return value for graphics.library/MakeVPort().

    NOTES
    	When graphics.library calls this method, the ViewPort is already unlinked
    	from its view, and the bitmap can already be deallocated.
    	This means that both data->Next and data->Bitmap pointers can contain invalid
    	values.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_MakeViewPort

    INTERNALS
	Base class implementation just does nothing. This function is mainly intended
	to provide support for copperlist disposal by Amiga(tm) chipset driver.

*****************************************************************************************/

void GFX__Hidd_Gfx__CleanViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CleanViewPort *msg)
{
    D(bug("Gfx::CleanViewPort: object 0x%p, data 0x%p\n", o, msg->Data));
}

/*****************************************************************************************

    NAME
	moHidd_Gfx_PrepareViewPorts

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_PrepareViewPorts *msg);

	ULONG HIDD_Gfx_PrepareViewPorts(OOP_Object *gfxHidd, struct HIDD_ViewPortData *data, struct View *view)

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Performs driver-specific setup on a given view.

    INPUTS
	gfxHidd - A display driver object.
	data    - a pointer to a chain of HIDD_ViewPortData structures.
	view	- A pointer to graphics.library View structure being prepared.

    RESULT
	MCOP_OK if there was no error or MCOP_NO_MEM otherwise.
	MCOP_NOP is not allowed as a return value of this method.

    NOTES
    	graphics.library calls this method in MrgCop() after the complete view
    	is built. data->Next pointer contains valid data.

    	This function can be repeatedly called several times, and there is no
    	cleanup counterpart for it. This should be taken into account in method
    	implementation.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Base class implementation just does nothing. This function is mainly intended
	to provide support for copperlist maintenance by Amiga(tm) chipset driver.

*****************************************************************************************/

ULONG GFX__Hidd_Gfx__PrepareViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ShowViewPorts *msg)
{
    return MCOP_OK;
}

#undef csd

/****************************************************************************************/

static int GFX_ClassInit(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    
    __IHidd_PixFmt  	= OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    __IHidd_BitMap  	= OOP_ObtainAttrBase(IID_Hidd_BitMap);
    __IHidd_Gfx     	= OOP_ObtainAttrBase(IID_Hidd_Gfx);
    __IHidd_Sync    	= OOP_ObtainAttrBase(IID_Hidd_Sync);
    __IHidd_GC      	= OOP_ObtainAttrBase(IID_Hidd_GC);
    __IHidd_Overlay    	= OOP_ObtainAttrBase(IID_Hidd_Overlay);
    __IHidd_ColorMap 	= OOP_ObtainAttrBase(IID_Hidd_ColorMap);
    __IHidd_PlanarBM	= OOP_ObtainAttrBase(IID_Hidd_PlanarBM);
    __IHidd_ChunkyBM	= OOP_ObtainAttrBase(IID_Hidd_ChunkyBM);
    
    if (!__IHidd_PixFmt     ||
     	!__IHidd_BitMap     ||
	!__IHidd_Gfx 	    ||
	!__IHidd_Sync 	    ||
	!__IHidd_GC 	    ||
	!__IHidd_ColorMap   ||
	!__IHidd_PlanarBM   ||
	!__IHidd_ChunkyBM
       )
    {
	goto failexit;
    }

    D(bug("Creating std pixelfmts\n"));
    if (!create_std_pixfmts(csd))
    	goto failexit;
    D(bug("Pixfmts created\n"));

    /* Get two methodis required for direct method execution */
#if USE_FAST_PUTPIXEL
    csd->putpixel_mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutPixel);
#endif
#if USE_FAST_GETPIXEL
    csd->getpixel_mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetPixel);
#endif
#if USE_FAST_DRAWPIXEL
    csd->drawpixel_mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawPixel);
#endif

    ReturnInt("init_gfxhiddclass", ULONG, TRUE);
    
failexit:
    ReturnInt("init_gfxhiddclass", ULONG, FALSE);
}

/****************************************************************************************/

static int GFX_ClassFree(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    
    EnterFunc(bug("free_gfxhiddclass(csd=%p)\n", csd));
    
    if(NULL != csd)
    {
	delete_pixfmts(csd);
        
    	OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
    	OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    	OOP_ReleaseAttrBase(IID_Hidd_Gfx);
    	OOP_ReleaseAttrBase(IID_Hidd_Sync);
    	OOP_ReleaseAttrBase(IID_Hidd_GC);
	OOP_ReleaseAttrBase(IID_Hidd_Overlay);
    	OOP_ReleaseAttrBase(IID_Hidd_ColorMap);
    	OOP_ReleaseAttrBase(IID_Hidd_PlanarBM);
    	OOP_ReleaseAttrBase(IID_Hidd_ChunkyBM);
    }
    
    ReturnInt("free_gfxhiddclass", BOOL, TRUE);
}

/****************************************************************************************/

ADD2INITLIB(GFX_ClassInit, 0)
ADD2EXPUNGELIB(GFX_ClassFree, 0)

/****************************************************************************************/

/* Since the shift/mask values of a pixel format are designed for pixel
   access, not byte access, they are endianess dependant */
   
#if AROS_BIG_ENDIAN
#include "stdpixfmts_be.h"
#else
#include "stdpixfmts_le.h"
#endif

/****************************************************************************************/

static BOOL create_std_pixfmts(struct class_static_data *csd)
{
    ULONG i;
    struct pixfmt_data *pf;
    
    memset(csd->std_pixfmts, 0, sizeof (OOP_Object *) * num_Hidd_StdPixFmt);
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++)
    {
        pf = (struct pixfmt_data *)create_and_init_object(csd->pixfmtclass, (UBYTE *)&stdpfs[i],  sizeof (stdpfs[i]), csd);

	if (!pf)
	{
	    D(bug("FAILED TO CREATE PIXEL FORMAT %d\n", i));
	    delete_pixfmts(csd);
	    ReturnBool("create_stdpixfmts", FALSE);
	}

	csd->std_pixfmts[i] = &pf->pf;
	/* We don't use semaphore protection here because we do this only during class init stage */
	pf->refcount = 1;
	AddTail((struct List *)&csd->pflist, (struct Node *)&pf->node);
    }
    ReturnBool("create_stdpixfmts", TRUE);
}

/****************************************************************************************/

static VOID delete_pixfmts(struct class_static_data *csd)
{
    struct Node *n, *safe;

    ForeachNodeSafe(&csd->pflist, n, safe)
	OOP_DisposeObject((OOP_Object *)PIXFMT_OBJ(n));
}

/****************************************************************************************/

static inline BOOL cmp_pfs(HIDDT_PixelFormat *tmppf, HIDDT_PixelFormat *dbpf)
{
    /* Just compare everything except stdpixfmt */
    /* Compare flags first (because it's a fast check) */
    if (tmppf->flags != dbpf->flags)
        return FALSE;
    /* If they match, compare the rest of things */
    return !memcmp(tmppf, dbpf, offsetof(HIDDT_PixelFormat, stdpixfmt));
}

/****************************************************************************************/

/*
    Parses the tags supplied in 'tags' and puts the result into 'pf'.
    It also checks to see if all needed attrs are supplied.
    It uses 'attrcheck' for this, so you may find attrs outside
    of this function, and mark them as found before calling this function
*/

#define PFAF(x) (1L << aoHidd_PixFmt_ ## x)
#define PF_COMMON_AF (   PFAF(Depth) | PFAF(BitsPerPixel) | PFAF(BytesPerPixel)	\
		       | PFAF(ColorModel) | PFAF(BitMapType) )
		       
#define PF_TRUECOLOR_AF ( PFAF(RedMask)  | PFAF(GreenMask)  | PFAF(BlueMask)  | PFAF(AlphaMask) | \
			  PFAF(RedShift) | PFAF(GreenShift) | PFAF(BlueShift) | PFAF(AlphaShift))
			  
#define PF_PALETTE_AF ( PFAF(CLUTMask) | PFAF(CLUTShift) | PFAF(RedMask) | PFAF(GreenMask) | \
			PFAF(BlueMask) )
		       
#define PFAO(x) (aoHidd_PixFmt_ ## x)
  
/****************************************************************************************/

BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf,
    	    	       ULONG ATTRCHECK(pixfmt), struct class_static_data *csd)
{
    IPTR attrs[num_Hidd_PixFmt_Attrs] = {0};

    if (0 != OOP_ParseAttrs(tags, attrs, num_Hidd_PixFmt_Attrs,
    	    	    	    &ATTRCHECK(pixfmt), HiddPixFmtAttrBase))
    {
	D(bug("!!! parse_pixfmt_tags: ERROR PARSING TAGS THROUGH OOP_ParseAttrs !!!\n"));
	return FALSE;
    }

    if (PF_COMMON_AF != (PF_COMMON_AF & ATTRCHECK(pixfmt)))
    {
	D(bug("!!! parse_pixfmt_tags: Missing PixFmt attributes passed to parse_pixfmt_tags(): %x !!!\n", ATTRCHECK(pixfmt)));
	return FALSE;
    }
    
    /* Set the common attributes */
    pf->depth		= attrs[PFAO(Depth)];
    pf->size		= attrs[PFAO(BitsPerPixel)];
    pf->bytes_per_pixel	= attrs[PFAO(BytesPerPixel)];
    /* Fill in only real StdPixFmt specification. Special values (Native and Native32)
       are not allowed here */
    if (attrs[PFAO(StdPixFmt)] >= num_Hidd_PseudoStdPixFmt)
        pf->stdpixfmt = attrs[PFAO(StdPixFmt)];
    
    SET_PF_COLMODEL(  pf, attrs[PFAO(ColorModel)]);
    SET_PF_BITMAPTYPE(pf, attrs[PFAO(BitMapType)]);
    
    if (ATTRCHECK(pixfmt) & PFAF(SwapPixelBytes))
    {
    	SET_PF_SWAPPIXELBYTES_FLAG(pf, attrs[PFAO(SwapPixelBytes)]);
    }
    
    /* Set the colormodel specific stuff */
    switch (HIDD_PF_COLMODEL(pf))
    {
    	case vHidd_ColorModel_TrueColor:
	    /* Check that we got all the truecolor describing stuff */
	    if (PF_TRUECOLOR_AF != (PF_TRUECOLOR_AF & ATTRCHECK(pixfmt)))
	    {
		 D(bug("!!! Unsufficient true color format describing attrs to pixfmt in parse_pixfmt_tags() !!!\n"));
		 return FALSE;
	    }
	    
	    /* Set the truecolor stuff */
	    pf->red_mask	= attrs[PFAO(RedMask)];
	    pf->green_mask	= attrs[PFAO(GreenMask)];
	    pf->blue_mask	= attrs[PFAO(BlueMask)];
	    pf->alpha_mask	= attrs[PFAO(AlphaMask)];
	    
	    pf->red_shift	= attrs[PFAO(RedShift)];
	    pf->green_shift	= attrs[PFAO(GreenShift)];
	    pf->blue_shift	= attrs[PFAO(BlueShift)];
	    pf->alpha_shift	= attrs[PFAO(AlphaShift)];
	    break;
	
	case vHidd_ColorModel_Palette:
	case vHidd_ColorModel_StaticPalette:
	    if ( PF_PALETTE_AF != (PF_PALETTE_AF & ATTRCHECK(pixfmt)))
	    {
		 D(bug("!!! Unsufficient palette format describing attrs to pixfmt in parse_pixfmt_tags() !!!\n"));
		 return FALSE;
	    }
	    
	    /* set palette stuff */
	    pf->clut_mask	= attrs[PFAO(CLUTMask)];
	    pf->clut_shift	= attrs[PFAO(CLUTShift)];

	    pf->red_mask	= attrs[PFAO(RedMask)];
	    pf->green_mask	= attrs[PFAO(GreenMask)];
	    pf->blue_mask	= attrs[PFAO(BlueMask)];

	    break;
	    
    } /* shift (colormodel) */
    
    return TRUE;
}

/****************************************************************************************/

/* 
    Create an empty object and initialize it the "ugly" way. This only works with
    CLID_Hidd_PixFmt and CLID_Hidd_Sync classes
*/

/****************************************************************************************/

static OOP_Object *create_and_init_object(OOP_Class *cl, UBYTE *data, ULONG datasize,
    	    	    	    	    	  struct class_static_data *csd)
{
    OOP_Object *o;
			
    o = OOP_NewObject(cl, NULL, NULL);
    if (NULL == o)
    {
	D(bug("!!! UNABLE TO CREATE OBJECT IN create_and_init_object() !!!\n"));
	return NULL;
    }
	    
    memcpy(o, data, datasize);
    
    return o;
}
	
/****************************************************************************************/

static struct pixfmt_data *find_pixfmt(HIDDT_PixelFormat *tofind, struct class_static_data *csd)
{
    struct pixfmt_data 	*retpf = NULL;
    HIDDT_PixelFormat 	*db_pf;
    struct Node   	*n;

    /* Go through the pixel format list to see if a similar pf allready exists */
    ObtainSemaphoreShared(&csd->pfsema);

    ForeachNode(&csd->pflist, n)
    {
    	db_pf = PIXFMT_OBJ(n);
	DPF(bug("find_pixfmt(): Trying pixelformat 0x%p\n", db_pf));
	DPF(bug("(%d, %d, %d, %d), (%x, %x, %x, %x), %d, %d, %d, %d\n",
	      db_pf->red_shift, db_pf->green_shift, db_pf->blue_shift, db_pf->alpha_shift,
	      db_pf->red_mask, db_pf->green_mask, db_pf->blue_mask, db_pf->alpha_mask,
	      db_pf->bytes_per_pixel, db_pf->size, db_pf->depth, db_pf->stdpixfmt));
	if (cmp_pfs(tofind, db_pf))
	{
	    DPF(bug("Match!\n"));
	    retpf = (struct pixfmt_data *)db_pf;
	    break;
	}
    }

    ReleaseSemaphore(&csd->pfsema);
    return retpf;
}

/****************************************************************************************/

/* Stubs for private methods */

/****************************************************************************************/

OOP_Object *HIDD_Gfx_RegisterPixFmt(OOP_Object *o, struct TagItem *pixFmtTags)
{
   STATIC_MID;  
   struct pHidd_Gfx_RegisterPixFmt p, *msg = &p;
   
   if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_RegisterPixFmt);
   
   p.mID = static_mid;
   
   p.pixFmtTags = pixFmtTags;
   
   return (OOP_Object *)OOP_DoMethod(o, (OOP_Msg)msg);
   
}

/****************************************************************************************/

VOID HIDD_Gfx_ReleasePixFmt(OOP_Object *o, OOP_Object *pixFmt)
{
   STATIC_MID;  
   struct pHidd_Gfx_ReleasePixFmt p, *msg = &p;
   
   if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_ReleasePixFmt);
   
   p.mID = static_mid;
   
   p.pixFmt = pixFmt;
   
   OOP_DoMethod(o, (OOP_Msg)msg);
   
}

/****************************************************************************************/

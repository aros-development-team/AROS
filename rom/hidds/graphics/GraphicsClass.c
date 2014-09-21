/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics hidd class implementation.
    Lang: english
*/

/****************************************************************************************/

#define DEBUG 0
#define SDEBUG 0
#define DPF(x)
#define DCOPYBOX(x)

#include <aros/atomic.h>
#include <aros/debug.h>
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
	, OOP_Object *dst_bm, UWORD width, UWORD height);

BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf, ULONG attrcheck, struct class_static_data *_csd);

/****************************************************************************************/

#define COMPUTE_HIDD_MODEID(sync, pf)	\
    ( ((sync) << 8) | (pf) )
    
#define MODEID_TO_SYNCIDX(id) (((id) & 0X0000FF00) >> 8)
#define MODEID_TO_PFIDX(id)   ( (id) & 0x000000FF)

/****************************************************************************************/

OOP_Object *GFX__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct TagItem gctags[] =
    {
        {aHidd_GC_Foreground, 0},
    	{TAG_DONE           , 0}
    };

    D(bug("Entering gfx.hidd::New\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    D(bug("Got object o=%x\n", o));

    if (o)
    {
        struct HIDDGraphicsData *data = OOP_INST_DATA(cl, o);
        struct TagItem *tstate = msg->attrList;
        struct TagItem *modetags = NULL;
        struct TagItem *tag;
	BOOL ok;

        InitSemaphore(&data->mdb.sema);
        InitSemaphore(&data->fbsem);
        data->fbmode = -1;

        D(bug("[GFX] attrList 0x%p\n", msg->attrList));

        while ((tag = NextTagItem(&tstate)))
        {
            ULONG idx;

            Hidd_Gfx_Switch(tag->ti_Tag, idx)
            {
            case aoHidd_Gfx_ModeTags:
                modetags = (struct TagItem *)tag->ti_Data;
                break;

            case aoHidd_Gfx_FrameBufferType:
                data->fbmode = tag->ti_Data;
                break;
            }
        }

        /* Register modes only after other attributes are initialized */
        ok = modetags ? register_modes(cl, o, modetags) : TRUE;

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
            OOP_MethodID dispose_mid = msg->mID - moRoot_New + moRoot_Dispose;

	    D(bug("Not OK\n"));
            OOP_CoerceMethod(cl, o, &dispose_mid);
            return NULL;
        }
    }

    D(bug("Leaving gfx.hidd::New o=%x\n", o));
    return o;
}

/****************************************************************************************/

VOID GFX__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
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

        Since v1.2 this attribute is obsolete. Please use aoHidd_Gfx_FrameBufferType
        in new code.

    NOTES
	Provides FALSE if not implemented in the driver.

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_FrameBufferType, moHidd_Gfx_Show

    INTERNALS

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

/*****************************************************************************************

    NAME
	aoHidd_Gfx_DefaultGC

    SYNOPSIS
	[..G], OOP_Object *

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Get a pointer to shared default GC object.

    NOTES
        The returned GC is preset to the following:

          DrawMode = Copy
          FG       = 0
          BG       = 0
          LinePat  = ~0
          ColMask  = ~0

        You must not alter these settings even temporarily, because this GC is shared between
        bitmaps and between different tasks which may perform the rendering into different
        regions of the same bitmap (two windows on one screen, for example). This GC is intended
        to be used for internal copying operations.

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_ActiveCallBack

    INTERNALS
	This attribute needs to be implemented by the display driver. Base class contains
	no implementation.

*****************************************************************************************/

static UBYTE get_fbmode(OOP_Class *cl, OOP_Object *o)
{
    struct HIDDGraphicsData *data = OOP_INST_DATA(cl, o);

    if (data->fbmode == -1)
    {
        struct Library *OOPBase = CSD(cl)->cs_OOPBase;

        /*
         * This attribute has never been set.
         * Fall back to obsolete NoFrameBuffer.
         */
        data->fbmode = OOP_GET(o, aHidd_Gfx_NoFrameBuffer) ? vHidd_FrameBuffer_None : vHidd_FrameBuffer_Direct;
    }

    return data->fbmode;
}

/*****************************************************************************************

    NAME
	aoHidd_Gfx_FrameBufferType

    SYNOPSIS
	[I.G], UBYTE

    LOCATION
	hidd.graphics.graphics

    FUNCTION
        Specifies fixed framebuffer type used by the driver. The value can be one of the following:

          vHidd_FrameBuffer_None     - the driver does not use framebuffer.
          vHidd_FrameBuffer_Direct   - the driver uses framefuffer which can be accessed
                                       directly for both reads and writes.
          vHidd_FrameBuffer_Mirrored - the driver uses write-only framebuffer.

        This attribute has to be specified during driver object creation. If this is not done,
        the OS will use value of old aoHidd_Gfx_NoFrameBuffer attribute in order to distinguish
        between vHidd_FrameBuffer_Direct (for FALSE) and vHidd_FrameBuffer_None (for TRUE).

    NOTES
        A fixed framebuffer is a special bitmap in a fixed area of video RAM. If the
        framebuffer is used, the driver is expected to copy a new bitmap into it in
        HIDD_Gfx_Show() and optionally copy old bitmap back.

        A framebuffer is needed if the hardware does not have enough VRAM to store many
        bitmaps or does not have capabilities to switch the display between various VRAM
        regions.

        Some hardware suffers from slow VRAM reading. In this case you should use mirrored
        mode. If you use it, the system will hold a bitmap in the memory buffer, and
        update VRAM on demand (hence the name).

        An example of driver using a framebuffer is hosted SDL driver. By design SDL works
        only with single display window, which is considered a framebuffer.

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Gfx_NoFrameBuffer

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Gfx_SupportsGamma

    SYNOPSIS
	[..G], UBYTE

    LOCATION
	hidd.graphics.graphics

    FUNCTION
        Specifies if the driver supports gamma correction tables. Default implementation
        in base class returns FALSE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Gfx_SetGamma

    INTERNALS

*****************************************************************************************/

VOID GFX__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDGraphicsData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Gfx_Switch (msg->attrID, idx)
    {
    case aoHidd_Gfx_NumSyncs:
	*msg->storage = data->mdb.num_syncs;
	return;

    case aoHidd_Gfx_IsWindowed:
    case aoHidd_Gfx_SupportsHWCursor:
    case aoHidd_Gfx_SupportsGamma:
        *msg->storage = 0;
        return;

    case aoHidd_Gfx_HWSpriteTypes:
        /* Fall back to obsolete SupportsHWCursor */
        *msg->storage = OOP_GET(o, aHidd_Gfx_SupportsHWCursor) ? (vHidd_SpriteType_3Plus1|vHidd_SpriteType_DirectColor) : 0;
        return;

    case aoHidd_Gfx_DriverName:
        *msg->storage = (IPTR)OOP_OCLASS(o)->ClassNode.ln_Name;
        return;

    case aoHidd_Gfx_DefaultGC:
        *msg->storage = (IPTR)data->gc;
        return;

    case aoHidd_Gfx_FrameBufferType:
        *msg->storage = get_fbmode(cl, o);
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;

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

#define COPY_BM_TAG(tags, idx, tag, obj)	\
    tags[idx].ti_Tag = aHidd_BitMap_ ## tag;	\
    OOP_GetAttr(obj, aHidd_BitMap_ ## tag , &tags[idx].ti_Data)

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
	it should prepend an aoHidd_BitMap_ClassPtr attribute to the supplied taglist
	and pass it to base class. It's not allowed to create bitmap objects directly
	since they need some more extra information which is added by the base class!

	This method must be implemented by your subclass. aHIDD_BitMap_ClassPtr or
	aHIDD_BitMap_ClassID must be provided to the base class for a displayable bitmap!

    INPUTS
	gfxHidd - The graphics driver object that will provide the bitmap.

	tagList - A list of bitmap attributes. See hidd.graphics.bitmap class
	          documentation for their descriptions.

    RESULT
	bm - pointer to the newly created bitmap.
    
    NOTES
        Drivers which do not use framebuffer must always specify own class using either
        moHidd_BitMap_ClassPtr or moHidd_BitMap_ClassID attribute. Drivers making use of
        framebuffer may omit this, in this case framebuffer's class will be used for
        displayable bitmaps.

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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct HIDDGraphicsData *data = OOP_INST_DATA(cl, o);

    struct TagItem bmtags[] =
    {
    	{aHidd_BitMap_GfxHidd, 0},	/* 0 */
    	{aHidd_BitMap_PixFmt , 0},	/* 1 */
    	{TAG_IGNORE	     , 0},	/* 2 */
    	{TAG_IGNORE	     , 0},	/* 3 */
    	{TAG_IGNORE	     , 0},	/* 4 */
    	{TAG_IGNORE	     , 0},	/* 5 */
    	{TAG_MORE	     , 0},	/* 6 */
    };
    OOP_Object   *bm;

    struct TagItem *tag, *tstate = msg->attrList;
    ULONG idx;

    STRPTR        classid     = NULL;
    OOP_Class    *classptr    = NULL;
    BOOL          displayable = FALSE;
    BOOL          framebuffer = FALSE;
    HIDDT_StdPixFmt pixfmt    = vHidd_StdPixFmt_Unknown;
    OOP_Object   *friend_bm   = NULL;
    OOP_Object   *sync	      = NULL;
    OOP_Object   *pf	      = NULL;

    BOOL    	  gotclass   = FALSE;
    BOOL	  got_width  = FALSE;
    BOOL	  got_height = FALSE;
    BOOL	  got_depth  = FALSE;

    while ((tag = NextTagItem(&tstate)))
    {
    	if (IS_BITMAP_ATTR(tag->ti_Tag, idx))
    	{
    	    switch (idx)
    	    {
		case aoHidd_BitMap_Displayable:
		    displayable = tag->ti_Data;
		    break;
	
		case aoHidd_BitMap_FrameBuffer:
		    framebuffer = tag->ti_Data;
		    break;
	
		case aoHidd_BitMap_Width:
		    got_width = TRUE;
		    break;
	
		case aoHidd_BitMap_Height:
		    got_height = TRUE;
		    break;
	
		case aoHidd_BitMap_Depth:
		    got_depth = TRUE;
		    break;
	
		case aoHidd_BitMap_ModeID:
		    /* Make sure it is a valid mode, and retrieve sync/pixelformat data */
		    if (!HIDD_Gfx_GetMode(o, tag->ti_Data, &sync, &pf))
		    {
			D(bug("!!! Gfx::NewBitMap: USER PASSED INVALID MODEID !!!\n"));
			return NULL;
		    }
		    break;
	
		case aoHidd_BitMap_Friend:
		    friend_bm = (OOP_Object *)tag->ti_Data;
		    break;
		    
		case aoHidd_BitMap_PixFmt:
		    D(bug("!!! Gfx::NewBitMap: USER IS NOT ALLOWED TO PASS aHidd_BitMap_PixFmt !!!\n"));
		    return NULL;
	
		case aoHidd_BitMap_StdPixFmt:
		    pixfmt = tag->ti_Data;
		    break;
	
		case aoHidd_BitMap_ClassPtr:
		    classptr = (OOP_Class *)tag->ti_Data;
		    gotclass = TRUE;
		    break;
	
		case aoHidd_BitMap_ClassID:
		    classid  = (STRPTR)tag->ti_Data;
		    gotclass = TRUE;
		    break;
	    }
	}
    }

    if (friend_bm)
    {
	/* If we have a friend bitmap, we can inherit some attributes from it */
    	if (!got_width)
    	{
    	    COPY_BM_TAG(bmtags, 2, Width, friend_bm);
    	    got_width = TRUE;
    	}
    	
    	if (!got_height)
    	{
    	    COPY_BM_TAG(bmtags, 3, Height, friend_bm);
    	    got_height = TRUE;
    	}

    	if (!got_depth)
	{
	    COPY_BM_TAG(bmtags, 4, Depth, friend_bm);
	}
    }

    if (framebuffer)
    {
        /* FrameBuffer implies Displayable */
	SET_BM_TAG(bmtags, 5, Displayable, TRUE);
    	displayable = TRUE;
    }
    else if (displayable)
    {
        /*
         * Displayable, but not framebuffer (user's screen).
         * If we are working in framebuffer mode, we treat all such
         * bitmaps as framebuffer's friends and can inherit its class.
         */
        if ((!gotclass) && data->framebuffer && (get_fbmode(cl, o) != vHidd_FrameBuffer_None))
        {
            classptr = OOP_OCLASS(data->framebuffer);
            gotclass = TRUE;

            D(bug("[GFX] Using class 0x%p (%s) for displayable bitmap\n", classptr, classptr->ClassNode.ln_Name));
        }
    }

    if (displayable)
    {
	/* Displayable bitmap. Here we must have ModeID and class. */
	if (!sync)
	{
	    D(bug("!!! Gfx::NewBitMap: USER HAS NOT PASSED MODEID FOR DISPLAYABLE BITMAP !!!\n"));
	    return NULL;
	}

	if (!gotclass)
	{
	    D(bug("!!! Gfx::NewBitMap: SUBCLASS DID NOT PASS CLASS FOR DISPLAYABLE BITMAP !!!\n"));
	    return NULL;
	}
    }
    else /* if (!displayable) */
    {
	/*
	 * This is an offscreen bitmap and we need to guess its pixelformat.
	 * In order to do this we need one of (in the order of preference):
	 * - ModeID
	 * - StdPixFmt
	 * - Friend
	 */

	if (sync)
	{
	    /*
	     * We have alredy got sync for the modeid case.
	     * Obtain missing size information from it.
	     */
	    if (!got_width)
	    {
	    	bmtags[2].ti_Tag = aHidd_BitMap_Width;
	    	OOP_GetAttr(sync, aHidd_Sync_HDisp, &bmtags[2].ti_Data);
	    }

	    if (!got_height)
	    {
		bmtags[3].ti_Tag = aHidd_BitMap_Height;
	    	OOP_GetAttr(sync, aHidd_Sync_VDisp, &bmtags[3].ti_Data);
	    }
	}
	else if (pixfmt != vHidd_StdPixFmt_Unknown)
	{
	    /* Next to look for is StdPixFmt */
	    pf = HIDD_Gfx_GetPixFmt(o, pixfmt);
	    if (NULL == pf)
	    {
		D(bug("!!! Gfx::NewBitMap(): USER PASSED BOGUS StdPixFmt !!!\n"));
		return NULL;
	    }
	}
	else if (friend_bm)
	{
	    /* Last alternative is that the user passed a friend bitmap */

	    OOP_GetAttr(friend_bm, aHidd_BitMap_PixFmt, (IPTR *)&pf);

	    /*
	     * Inherit the class from friend bitmap (if not already specified).
	     * We do it because friend bitmap may be a display HIDD bitmap
	     */
	    if (!gotclass)
	    {
	    	classptr = OOP_OCLASS(friend_bm);
	    	gotclass  = TRUE;

		D(bug("[GFX] Friend bitmap is 0x%p has class 0x%p (%s)\n", friend_bm, classptr, classptr->ClassNode.ln_Name));
	    }
	}
	else
	{
	    D(bug("!!! Gfx::NewBitMap: INSUFFICIENT ATTRS TO CREATE OFFSCREEN BITMAP !!!\n"));
	    return NULL;
	}

	/* Did the subclass provide an offbitmap class for us? */
	if (!gotclass)
	{
	    /* Have to find a suitable class ourselves from the pixelformat */
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

    } /* if (!displayable) */

    /* Set the tags we want to pass to the selected bitmap class */
    bmtags[0].ti_Data = (IPTR)o;
    bmtags[1].ti_Data = (IPTR)pf;
    bmtags[6].ti_Data = (IPTR)msg->attrList;

    bm = OOP_NewObject(classptr, classid, bmtags);

    if (framebuffer)
    {
    	/* Remember the framebuffer. It can be needed for default Show() implementation. */
    	data->framebuffer = bm;
    }

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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;

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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
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
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
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
    
    for (tstate = modetags; (tag = NextTagItem(&tstate));)
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
    
    
    for (tstate = modetags; (tag = NextTagItem(&tstate));)
    {
	/* Look for Gfx, PixFmt and Sync tags */
	ULONG idx;
	
	if (IS_GFX_ATTR(tag->ti_Tag, idx))
	{
	    switch (idx)
	    {
		case aoHidd_Gfx_PixFmtTags:
		    def_pixfmt_tags[num_Hidd_PixFmt_Attrs].ti_Data = tag->ti_Data;
		    mdb->pixfmts[pfidx] = GFX__Hidd_Gfx__RegisterPixFmt(cl, def_pixfmt_tags);
		    
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
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
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
    
    for (tstate = msg->queryTags; (tag = NextTagItem(&tstate)); )
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
    	    	    	       OOP_Object *dst_bm, UWORD width, UWORD height)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
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

    if (data->fbmode == vHidd_FrameBuffer_Mirrored)
    {
        /*
         * Mirror mode, just turn on visibility.
         * The data will be copied to the framebuffer later,
         * when graphics.library calls UpdateRect() after Show().
         */
        BM__Hidd_BitMap__SetVisible(CSD(cl)->bitmapclass, src_bm, TRUE);
    }
    else
    {
        /*
         * Direct framebuffer mode.
         * graphics.library will call UpdateRect() on the framebuffer object.
         * So we need to copy the data now.
         * We don't support scrolling for this mode, so we simply do the
         * bulk copy and ignore all offsets.
         */
        HIDD_Gfx_CopyBox(o, src_bm, 0, 0,
                         dst_bm, 0, 0, width, height, data->gc);
    }
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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDGraphicsData *data = OOP_INST_DATA(cl, o);
    OOP_Object      	    *bm = msg->bitMap;
    IPTR oldwidth  = 0;
    IPTR oldheight = 0;
    IPTR newwidth  = 0;
    IPTR newheight = 0;

    if (NULL == data->framebuffer)
    {
    	/* We require framebuffer. Don't call us otherwise. */
	return NULL;
    }

    ObtainSemaphore(&data->fbsem);

    if (data->shownbm)
    {
        /* Get size of old bitmap */
        OOP_GetAttr(data->shownbm, aHidd_BitMap_Width, &oldwidth);
        OOP_GetAttr(data->shownbm, aHidd_BitMap_Height, &oldheight);

        if (data->fbmode == vHidd_FrameBuffer_Mirrored)
        {
            BM__Hidd_BitMap__SetVisible(CSD(cl)->bitmapclass, data->shownbm, FALSE);
        }
        else if (msg->flags & fHidd_Gfx_Show_CopyBack)
	{
	    /* Copy the framebuffer data back into the old shown bitmap */
	    copy_bm_and_colmap(cl, o, data->framebuffer, data->shownbm, oldwidth, oldheight);
        }
    }

    if (bm == data->framebuffer)
    {
	/* If showing the framebuffer itself, just detach from old bitmap and that's all. */
    	data->shownbm = NULL;
        ReleaseSemaphore(&data->fbsem);

    	return data->framebuffer;
    }

    if (bm)
    {
    	IPTR modeid;

	/*
	 * Switch framebuffer display mode.
	 * This operation can fail if the bitmap has inappropriate mode.
	 */
    	OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);
    	if (!OOP_SetAttrsTags(data->framebuffer, aHidd_BitMap_ModeID, modeid, TAG_DONE))
    	    return NULL;

    	/* Get size of new bitmap */
        OOP_GetAttr(bm, aHidd_BitMap_Width, &newwidth);
        OOP_GetAttr(bm, aHidd_BitMap_Height, &newheight);

        /* Copy it into the framebuffer */
    	copy_bm_and_colmap(cl, o, bm, data->framebuffer, newwidth, newheight);
    }

    /*
     * Clear remaining parts of the framebuffer (if previous bitmap was larger than new one)
     * Note that if the new bitmap is NULL, newwidth and newheight will both be zero.
     * This will cause clearing the whole display.
     */
    if (oldheight) /* width and height can be zero only together, check one of them */
    {
        if (newwidth < oldwidth)
	    HIDD_BM_FillRect(data->framebuffer, data->gc, newwidth, 0, oldwidth - 1, oldheight - 1);
        if ((newheight < oldheight) && newwidth)
	    HIDD_BM_FillRect(data->framebuffer, data->gc, 0, newheight, newwidth - 1, oldheight);
    }

    /* Remember new displayed bitmap */
    data->shownbm = bm;

    ReleaseSemaphore(&data->fbsem);

    /* Return the actual bitmap to perform further operations on */
    return (data->fbmode == vHidd_FrameBuffer_Mirrored) ? bm : data->framebuffer;
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
	                             WORD xoffset, WORD yoffset);

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
	pointer is implemented in a special layer called fakegfx.hidd inside
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
	gfxHidd - a display driver object, on whose display you wish to turn
	    pointer on or off
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

	BOOL HIDD_Gfx_SetCursorPos(OOP_Object *gfxHidd, WORD x, WORD y);

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
	y       - A y coordinate of the pointer (relative to the physical screen origin)

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
	srcY    - a Y coordinate of the source rectangle
	dest    - a pointer to destination bitmap object
	destX   - an X coordinate of the destination rectangle
	destY   - a Y coordinate of the destination rectangle
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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    WORD    	    	    	    x, y;
    WORD    	    	    	    srcX = msg->srcX, destX = msg->destX;
    WORD    	    	    	    srcY = msg->srcY, destY = msg->destY;
    WORD    	    	    	    startX, endX, deltaX, startY, endY, deltaY;
    ULONG   	    	    	    memFG;
    HIDDT_PixelFormat 	    	    *srcpf, *dstpf;
    OOP_Object      	    	    *dest, *src;
    OOP_Object      	    	    *gc;
    APTR			    srcPixels  = NULL;
    APTR			    destPixels = NULL;

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

    DCOPYBOX(bug("COPYBOX: obj=0x%p (%s), src=0x%p at (%d, %d), dst=0x%p at (%d, %d), size=%dx%d\n", obj, OOP_OCLASS(obj)->ClassNode.ln_Name,
		 msg->src, srcX, srcY, msg->dest, destX, destY, msg->width, msg->height));
    DCOPYBOX(bug("COPYBOX: GC=0x%p, DrawMode %ld, ColMask 0x%08X\n", msg->gc, GC_DRMD(msg->gc), GC_COLMASK(msg->gc)));

#ifdef COPYBOX_DUMP_DIMS
    {
	IPTR sw, sh, dw, dh;

	OOP_GetAttr(obj, aHidd_BitMap_Width, &sw);
	OOP_GetAttr(obj, aHidd_BitMap_Height, &sh);
	OOP_GetAttr(msg->dest, aHidd_BitMap_Width, &dw);
	OOP_GetAttr(msg->dest, aHidd_BitMap_Height, &dh);

	bug("src dims: %dx%d  dest dims: %dx%d\n", sw, sh, dw, dh);
    }
#endif

    dstpf = (HIDDT_PixelFormat *)HBM(dest)->prot.pixfmt;

    OOP_GetAttr(msg->src,  aHidd_ChunkyBM_Buffer, (APTR)&srcPixels);
    OOP_GetAttr(msg->dest, aHidd_ChunkyBM_Buffer, (APTR)&destPixels);

    if (srcPixels && destPixels)
    {
	/*
    	 * Both bitmaps are chunky ones and they have directly accessible buffer.
    	 * We can use optimized routines to do the copy.
    	 */
	IPTR src_bytesperline, dest_bytesperline;

	OOP_GetAttr(msg->src, aHidd_BitMap_BytesPerRow, &src_bytesperline);
	OOP_GetAttr(msg->dest, aHidd_BitMap_BytesPerRow, &dest_bytesperline);

    	switch(GC_DRMD(msg->gc))
	{
	case vHidd_GC_DrawMode_Copy:
	    /* At the moment we optimize only bulk copy */

    	    if (srcpf == dstpf)
    	    {
    	    	/*
    	    	 * The same pixelformat. Extremely great!
    	    	 *
    	    	 * FIXME: Bulk copy to the same pixelformat is also handled in ConvertPixels very well
    	    	 *	  (optimized to either per-line or bulk memcpy()). But it can't handle
    	    	 *	  overlapping regions (which seems to be a requirement for CopyBox).
    	    	 *	  If this is fixed, we can even throw away HIDD_BM_CopyMemBoxXX at all, reducing
    	    	 *	  kickstart size.
    	    	 */
	    	switch(srcpf->bytes_per_pixel)
		{
		case 1:
		    /*
		     * In fact all these methods are static, they ignore object pointer, and it's
		     * needed only for OOP_DoMethod() to fetch class information.
		     * We use destination bitmap pointer, we can also source one.
		     */
	    	    HIDD_BM_CopyMemBox8(msg->dest,
		    	    		srcPixels, msg->srcX, msg->srcY,
					destPixels, msg->destX, msg->destY,
					msg->width, msg->height,
					src_bytesperline, dest_bytesperline);
		    return;

		case 2:
	    	    HIDD_BM_CopyMemBox16(msg->dest,
		    	    		 srcPixels, msg->srcX, msg->srcY,
					 destPixels, msg->destX, msg->destY,
					 msg->width, msg->height,
					 src_bytesperline, dest_bytesperline);
		    return;

		case 3:
	    	    HIDD_BM_CopyMemBox24(msg->dest,
		    	    		 srcPixels, msg->srcX, msg->srcY,
		    	    		 destPixels, msg->destX, msg->destY,
					 msg->width, msg->height,
					 src_bytesperline, dest_bytesperline);
		    return;

		case 4:
	    	    HIDD_BM_CopyMemBox32(msg->dest,
		    	    		 srcPixels, msg->srcX, msg->srcY,
					 destPixels, msg->destX, msg->destY,
					 msg->width, msg->height,
					 src_bytesperline, dest_bytesperline);
		    return;

	    	} /* switch(srcpf->bytes_per_pixel) */
	   } /* srcpf == dstpf */
	   else
	   {
	   	/*
	   	 * Pixelformats are different. This can't be the same bitmap,
	   	 * and it's safe to use ConvertPixels method (see FIXME above).
	   	 */
	   	srcPixels  += (msg->srcY  * src_bytesperline ) + (msg->srcX  * srcpf->bytes_per_pixel);
	   	destPixels += (msg->destY * dest_bytesperline) + (msg->destX * dstpf->bytes_per_pixel);

		/*
		 * Supply NULL pixlut. In this case bitmap's own colormap will be used
		 * for color lookup (if needed).
		 */
	   	HIDD_BM_ConvertPixels(msg->dest,
	   			      &srcPixels, srcpf, src_bytesperline,
	   			      &destPixels, dstpf, dest_bytesperline,
	   			      msg->width, msg->height, NULL);

	   	return;
	   }

    	   break;

	/* TODO: Optimize other DrawModes here */

    	} /* switch(mode) */  

    } /* srcPixels && destPixels */

    gc = msg->gc;

    memFG = GC_FG(msg->gc);
    
    /* All else have failed, copy pixel by pixel */
    if (HIDD_PF_COLMODEL(srcpf) == HIDD_PF_COLMODEL(dstpf))
    {
    	if (IS_TRUECOLOR(srcpf))
	{
    	    DCOPYBOX(bug("COPY FROM TRUECOLOR TO TRUECOLOR\n"));

	    for(y = startY; y != endY; y += deltaY)
	    {
		HIDDT_Color col;

		/* if (0 == strcmp("CON: Window", FindTask(NULL)->tc_Node.ln_Name))
		    bug("[%d,%d] ", memSrcX, memDestX);
		*/    
		for(x = startX; x != endX; x += deltaX)
		{
		    HIDDT_Pixel pix = GETPIXEL(cl, src, srcX + x, srcY + y);

#if COPYBOX_CHECK_FOR_ALIKE_PIXFMT
		    if (srcpf == dstpf)
		    {
			GC_FG(gc) = pix;
		    }
		    else
#endif
		    {
		    	HIDD_BM_UnmapPixel(src, pix, &col);
		    	GC_FG(gc) = HIDD_BM_MapColor(msg->dest, &col);
		    }

		    DRAWPIXEL(cl, dest, gc, destX + x, destY + y);
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
    	    DCOPYBOX(bug("COPY FROM PALETTE TO PALETTE\n"));

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
           for(y = startY; y != endY; y += deltaY)
           {
               for(x = startX; x != endX; x += deltaX)
               {
                   HIDDT_Pixel pix;
                   HIDDT_Color col;

                   pix = HIDD_BM_GetPixel(src, srcX + x, srcY + y);
                   HIDD_BM_UnmapPixel(src, pix, &col);

                   HIDD_BM_PutPixel(dest, destX + x, destY + y,
                       HIDD_BM_MapColor(dest, &col));
               }
           }
	}
	else if (IS_TRUECOLOR(dstpf))
	{
	    /* Get the colortab */
	    HIDDT_Color *ctab = ((HIDDT_ColorLUT *)HBM(src)->colmap)->colors;

    	    DCOPYBOX(bug("COPY FROM PALETTE TO TRUECOLOR, DRAWMODE %d, CTAB %p\n", GC_DRMD(gc), ctab));

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
	moHidd_Gfx_CopyBoxMasked

    SYNOPSIS
        IPTR OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_CopyBoxMasked *msg);

	IPTR HIDD_Gfx_CopyBoxMasked(OOP_Object *gfxHidd, OOP_Object *src, WORD srcX, WORD srcY,
	                      OOP_Object *dest, WORD destX, WORD destY, UWORD width, UWORD height, 
	                      PLANEPTR mask, OOP_Object *gc);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Perform rectangle copy (blit) operation from one bitmap to another,
	using a cookie cutter mask.
    
	Given bitmaps must be on the same display driver.
    
	A GC is used in order to specify raster operation performed between the source
	and destination according to its aHidd_GC_DrawMode attribute value.

    INPUTS
	gfxHidd - a display driver object that you are going to use for copying
	src     - a pointer to source bitmap object
	srcX    - an X coordinate of the source rectangle
	srcY    - a Y coordinate of the source rectangle
	dest    - a pointer to destination bitmap object
	destX   - an X coordinate of the destination rectangle
	destY   - a Y coordinate of the destination rectangle
	width   - width of the rectangle to copy
	height  - height of the rectangle to copy
	mask    - single bitplane mask
	gc      - graphics context holding draw mode on the destination

    RESULT
	TRUE is the operation succeeded and FALSE in case of some error, for example
	if the system was too low on memory.

    NOTES
	You must specify valid coordinates (non-negative and inside the actual bitmap
	area), no checks are done.

	It is valid to specify two overlapped areas of the same bitmap as source
	and destination.

	Mask size must correspond to full source bitmap size (including alignment).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/* Nominal size of the pixel conversion buffer */
#ifdef __mc68000
#define NUMPIX 4096 	/* Not that much room to spare */
#else
#define NUMPIX 100000
#endif

IPTR GFX__Hidd_Gfx__CopyBoxMasked(OOP_Class *cl, OOP_Object *obj, struct pHidd_Gfx_CopyBoxMasked *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    ULONG pixfmt = vHidd_StdPixFmt_Native32;
    OOP_Object *src_pf, *dest_pf;
    HIDDT_ColorModel src_colmod, dest_colmod;
    HIDDT_Color *ctab = NULL;
    ULONG bytes_per_line, lines_per_step, doing_lines, lines_done;
    IPTR mask_align, mask_bpr;
    UBYTE *srcbuf;

    OOP_GetAttr(msg->src , aHidd_BitMap_PixFmt, (IPTR *)&src_pf);
    OOP_GetAttr(msg->dest, aHidd_BitMap_PixFmt, (IPTR *)&dest_pf);

    OOP_GetAttr(src_pf,  aHidd_PixFmt_ColorModel, &src_colmod);
    OOP_GetAttr(dest_pf, aHidd_PixFmt_ColorModel, &dest_colmod);

    if ((src_colmod == vHidd_ColorModel_Palette) && (dest_colmod == vHidd_ColorModel_TrueColor))
    {
	/*
	 * LUT->truecolor conversion. We need a colormap (palette).
	 * If the source is displayable (screen) bitmap, it will have own palette. In this case
	 * we should use it.
	 * If it's just a bitmap in memory, it won't have a palette (colmap will be NULL). In this
	 * case destination bitmap needs to have a palette. This will happen if it's either a displayable
	 * bitmap, or is a friend of displayable bitmap (will share friend's palette then).
	 */
    	HIDDT_ColorLUT *colmap = (HIDDT_ColorLUT *)HBM(msg->src)->colmap;

    	if (!colmap)
    	    colmap = (HIDDT_ColorLUT *)HBM(msg->dest)->colmap;

	if (!colmap)
	{
	    D(bug("BltMaskBitMapRastPort could not retrieve pixel table for blit from palette to truecolor bitmap"));
	    return FALSE;
	}

	ctab = colmap->colors;
    }
    else if ((src_colmod == vHidd_ColorModel_TrueColor) && (dest_colmod == vHidd_ColorModel_TrueColor))
    {
    	if (src_pf != dest_pf)
    	{
    	    /*
    	     * Pixelformats are different, conversion needed.
    	     * First, we use two operations on destination bitmap (get and put), and only one
    	     * operation on source bitmap (get).
    	     * Second, we map LUT source pixels on destination bitmap's pixelformat.
    	     * Resume: we use destination's pixelformat for the whole operation. Conversion
    	     * happens during GetImage on the source bitmap.
    	     */
    	    OOP_GetAttr(dest_pf, aHidd_PixFmt_StdPixFmt, (IPTR *)&pixfmt);
    	}
    }
    else if ((src_colmod == vHidd_ColorModel_TrueColor) && (dest_colmod == vHidd_ColorModel_Palette))
    {
    	D(bug("BltMaskBitMapRastPort from truecolor bitmap to palette bitmap not supported!"));
    	return FALSE;
    }

    /* Mask width in pixels corresponds to full bitmap width (including alignment) */
    OOP_GetAttr(msg->src, aHidd_BitMap_Width, &mask_bpr);
    OOP_GetAttr(msg->src, aHidd_BitMap_Align, &mask_align);

    mask_align--;
    mask_bpr = ((mask_bpr + mask_align) & ~mask_align) >> 3;

    /*
     * TODO: Here we use a temporary buffer to perform the operation. This is slow and
     * actually unnecessary is many cases. If one of source or destination bitmaps is
     * directly accessible, we should use their own buffers. This will increase the speed
     * and decrease memory usage, because of omitted copy operations.
     */

    /* Based on the NUMPIX advice, figure out how many
     * lines per step we can allocate
     */
    bytes_per_line = msg->width * sizeof(HIDDT_Pixel);
    lines_per_step = NUMPIX / bytes_per_line;
    if (lines_per_step == 0)
    	lines_per_step = 1;

    /* Allocate a temporary buffer */
    srcbuf = AllocMem(2 * lines_per_step * bytes_per_line, MEMF_ANY);

    /* Try line-at-a-time if we can't allocate a big buffer */
    if (!srcbuf && lines_per_step > 1)
    {
    	lines_per_step = 1;
    	srcbuf = AllocMem(2 * lines_per_step * bytes_per_line, MEMF_ANY);
    }

    if (srcbuf)
    {
    	UBYTE *destbuf = srcbuf + lines_per_step * bytes_per_line;
    	HIDDT_DrawMode drawmode = GC_DRMD(msg->gc);

	/* PutImage needs to be called in Copy mode for proper operation */
	GC_DRMD(msg->gc) = vHidd_GC_DrawMode_Copy;

	for (lines_done = 0; lines_done != msg->height; lines_done += doing_lines)
	{
	    HIDDT_Pixel *srcpixelbuf;
	    HIDDT_Pixel *destpixelbuf;
	    UBYTE   	*mask;
	    UWORD	 x, y;

	    doing_lines = lines_per_step;
	    if (lines_done + doing_lines > msg->height)
	    	doing_lines = msg->height - lines_done;

	    HIDD_BM_GetImage(msg->src, srcbuf, bytes_per_line,
			     msg->srcX, msg->srcY + lines_done,
			     msg->width, doing_lines, pixfmt);

	    HIDD_BM_GetImage(msg->dest, destbuf, bytes_per_line,
			     msg->destX, msg->destY + lines_done,
			     msg->width, doing_lines, pixfmt);

	    mask = &msg->mask[COORD_TO_BYTEIDX(0, msg->srcY + lines_done, mask_bpr)];

	    srcpixelbuf  = (HIDDT_Pixel *)srcbuf;
	    destpixelbuf = (HIDDT_Pixel *)destbuf;

	    switch (drawmode)
	    {
	    case vHidd_GC_DrawMode_Or:	 /* (ABC|ABNC|ANBC) if copy source and blit thru mask */
	    case vHidd_GC_DrawMode_Copy: /* (ABC|ABNC = 0xC0) - compatibility with AOS3 */
		for (y = 0; y < doing_lines; y++)
		{
		    for (x = 0; x < msg->width; x++)
		    {
			if (mask[XCOORD_TO_BYTEIDX(msg->srcX + x)] & XCOORD_TO_MASK(msg->srcX + x))
			{
			    HIDDT_Pixel pix = *srcpixelbuf;

			    if (ctab)
			    {
			    	/*
			    	 * TODO:
			    	 * Here and in several other places we use colormap data for palette->truecolor conversion.
			    	 * The algorithm is as follows: take LUT pixel value (which is effectively color index), find RGB
			    	 * entry in the LUT, then compose a pixel from RGB triplet using MapColor method on the destination
			    	 * bitmap.
			    	 * This two-step operation is slow. graphics.library internally uses cached version of the palette
			    	 * (HIDDT_PixLut), where all colors are already decoded into pixel values.
			    	 * Since HIDD subsystem also needs this, this pixlut (called pixtab - pixel table in graphics.library),
			    	 * should be moved here. In fact a good place for it is a bitmap object (since pixlut = colormap + pixfmt,
			    	 * and pixfmt is a bitmap's property.
			    	 * graphics.library has pixlut attached to a BitMap structure (using HIDD_BM_PIXTAB() macro).
			    	 */
				pix = HIDD_BM_MapColor(msg->dest, &ctab[pix]);
			    }

			    *destpixelbuf = pix;
			}
			srcpixelbuf++;
			destpixelbuf++;
		    }
		    mask += mask_bpr;
		}
	 	break;

	    case vHidd_GC_DrawMode_AndInverted: /* (ANBC) if invert source and blit thru mask */
		D(bug("CopyBoxMasked does not support ANBC minterm yet"));
		break;

	    default:
		D(bug("CopyBoxMasked: DrawMode 0x%x not handled.\n", drawmode));
		break;
	    }

	    HIDD_BM_PutImage(msg->dest, msg->gc, destbuf, bytes_per_line, 
			     msg->destX, msg->destY + lines_done,
			     msg->width, doing_lines, pixfmt);

	} /* for(lines_done = 0; lines_done != height; lines_done += doing_lines) */

	/* Restore GC state */
	GC_DRMD(msg->gc) = drawmode;

    	/* Free our temporary buffer */
    	FreeMem(srcbuf, 2 * lines_per_step * bytes_per_line);

	return TRUE;
    } /* if (lines_per_step) */
    else
    {
    	/* urk :-( pixelbuffer too small to hold two lines) */
	D(bug("BltMaskBitMapRastPort found pixelbuffer to be too small"));
	return FALSE;
    }    
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

/*
 * The following two methods are private, static, and not virtual.
 * They operate only on static data and don't need object pointer.
 */

OOP_Object *GFX__Hidd_Gfx__RegisterPixFmt(OOP_Class *cl, struct TagItem *pixFmtTags)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    HIDDT_PixelFormat 	    cmp_pf;
    struct class_static_data *data;
    struct pixfmt_data 	    *retpf = NULL;

    memset(&cmp_pf, 0, sizeof(cmp_pf));

    data = CSD(cl);
    if (!parse_pixfmt_tags(pixFmtTags, &cmp_pf, 0, CSD(cl)))
    {
    	D(bug("!!! FAILED PARSING TAGS IN Gfx::RegisterPixFmt() !!!\n"));
	return FALSE;
    }

    /*
     * Our alpha-less R8G8B8 pixelformats are defined as having depth
     * and size = 24, not 32 bits. Nevertheless, many hardware reports
     * 32 bits in such cases.
     * In order to avoid confusion we attempt to detect this situation and
     * fix up pixelformat definition. If we don't do it, we get nonstandard
     * pixelformat with no corresponding CGX code, which can misbehave.
     */
    if ((cmp_pf.flags == PF_GRAPHTYPE(TrueColor, Chunky)) &&
        (cmp_pf.bytes_per_pixel == 4) && (cmp_pf.alpha_mask == 0) &&
        (cmp_pf.red_mask << cmp_pf.red_shift == 0xFF000000) &&
        (cmp_pf.green_mask << cmp_pf.green_shift == 0xFF000000) &&
        (cmp_pf.blue_mask << cmp_pf.blue_shift == 0xFF000000))
    {
        DPF(bug("Gfx::RegisterPixFmt(): 4-byte R8G8B8 detected\n"));

        if (cmp_pf.depth > 24)
        {
            DPF(bug("Gfx::RegisterPixFmt(): Fixing up depth %d > 24\n", cmp_pf.depth));
            cmp_pf.depth = 24;
        }

        if (cmp_pf.size > 24)
        {
            DPF(bug("Gfx::RegisterPixFmt(): Fixing up size %d > 24\n", cmp_pf.size));
            cmp_pf.size = 24;
        }
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
    else
    {
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

/* This method doesn't need object pointer, it's static. */

VOID GFX__Hidd_Gfx__ReleasePixFmt(OOP_Class *cl, OOP_Object *pf)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct class_static_data *data;
    struct pixfmt_data *pixfmt = (struct pixfmt_data *)pf;

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
	propsLen - length of the supplied buffer in bytes.

    RESULT
	Actual length of obtained structure

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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
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

        This method was neither ever implemented nor used. Currently obsolete and
        considered reserved.

    INPUTS

    RESULT

    NOTES

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

    INPUTS
	gfxHidd - A display driver object
	Red     - A pointer to a 256-byte array for red component
	Green   - A pointer to a 256-byte array for green component
	Blue    - A pointer to a 256-byte array for blue component

    RESULT
	FALSE if the driver doesn't support gamma correction, otherwise TRUE

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

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

	BOOL HIDD_Gfx_GetMaxSpriteSize(OOP_Object *gfxHidd, ULONG Type, UWORD *Width, UWORD *Height);

    LOCATION
	hidd.graphics.graphics

    FUNCTION
	Query maximum allowed size for the given sprite type.

    INPUTS
	gfxHidd - A display driver object
	Type	- Type of the sprite image (one of vHidd_SpriteType_... values)
	Width	- A pointer to UWORD where width will be placed.
	Height	- A pointer to UWORD where height will be placed.

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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;

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
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
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
	data    - a pointer to a HIDD_ViewPortData structure.

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

/****************************************************************************************/

/* This is a private nonvirtual method */
void GFX__Hidd_Gfx__UpdateBitMap(OOP_Class *cl, OOP_Object *o, OOP_Object *bm,
                                 struct pHidd_BitMap_UpdateRect *msg, struct Rectangle *display)
{
    struct HIDDGraphicsData *data = OOP_INST_DATA(cl, o);

    /*
     * We check data->shownbm twice in order to avoid unnecessary locking
     * when our bitmap is not on display (it may be not displayable at all).
     * However the second check is still needed in order to make sure that
     * this bitmap is still on display, because we could be preempted between
     * the test and ObtainSemaphore() by concurrently running Show() call.
     * We use shared lock because it's safe to have two concurrently running
     * updates even on the same region.
     */
    if ((data->fbmode == vHidd_FrameBuffer_Mirrored) && (bm == data->shownbm))
    {
        ObtainSemaphoreShared(&data->fbsem);

        if (bm == data->shownbm)
        {
            /*
             * Complete update rectangle.
             * Display rectangle is already in bitmap's coordinates.
             */
            UWORD srcX = msg->x;
            UWORD srcY = msg->y;
            UWORD xLimit = srcX + msg->width;
            UWORD yLimit = srcY + msg->height;

            /* Intersect rectangles */
            if (display->MinX > srcX)
                srcX = display->MinX;
            if (display->MinY > srcY)
                srcY = display->MinY;
            if (display->MaxX < xLimit)
                xLimit = display->MaxX;
            if (display->MaxY < yLimit)
                yLimit = display->MaxY;

            if ((xLimit > srcX) && (yLimit > srcY))
            {
                HIDD_Gfx_CopyBox(o, bm, srcX, srcY,
                                 data->framebuffer, srcX - display->MinX, srcY - display->MinY,
                                 xLimit - srcX, yLimit - srcY, data->gc);
            }
        }

        ReleaseSemaphore(&data->fbsem);
    }
}

#undef csd

/****************************************************************************************/

static ULONG ObtainAttrBases(OOP_AttrBase *bases, CONST_STRPTR *interfaces, ULONG count, struct Library *OOPBase)
{
    ULONG i;
    ULONG failed = 0;
    
    for (i = 0; i < count; i++)
    {
    	bases[i] = OOP_ObtainAttrBase(interfaces[i]);
    	if (!bases[i])
    	    failed++;
    }
    
    return failed;
}

static void ReleaseAttrBases(OOP_AttrBase *bases, CONST_STRPTR *interfaces, ULONG count, struct Library *OOPBase)
{
    ULONG i;
    
    for (i = 0; i < count; i++)
    {
    	if (bases[i])
    	    OOP_ReleaseAttrBase(interfaces[i]);
    }
}

static ULONG GetMethodBases(OOP_MethodID *bases, CONST_STRPTR *interfaces, ULONG count, struct Library *OOPBase)
{
    ULONG i;
    ULONG failed = 0;

    for (i = 0; i < count; i++)
    {
    	bases[i] = OOP_GetMethodID(interfaces[i], 0);
    	if (bases[i] == -1)
    	    failed++;
    }

    return failed;
}

static CONST_STRPTR interfaces[NUM_ATTRBASES] =
{
    IID_Hidd_BitMap,
    IID_Hidd_Gfx,
    IID_Hidd_GC,
    IID_Hidd_ColorMap,
    IID_Hidd_Overlay,
    IID_Hidd_Sync,
    IID_Hidd_PixFmt,
    IID_Hidd_PlanarBM,
    IID_Hidd_ChunkyBM,
};

static int GFX_ClassInit(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    struct Library *OOPBase = csd->cs_OOPBase;

    if (!(csd->cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY)))
    {
	ReturnInt("init_gfxhiddclass", ULONG, FALSE);
    }
 
    if (ObtainAttrBases(csd->attrBases, interfaces, NUM_ATTRBASES, OOPBase))
    {
	ReturnInt("init_gfxhiddclass", ULONG, FALSE);
    }

    if (GetMethodBases(csd->methodBases, interfaces, NUM_METHODBASES, OOPBase))
    {
	ReturnInt("init_gfxhiddclass", ULONG, FALSE);
    }

    D(bug("Creating std pixelfmts\n"));
    ReturnInt("init_gfxhiddclass", ULONG, create_std_pixfmts(csd));    
}

/****************************************************************************************/

static int GFX_ClassFree(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    struct Library *OOPBase = csd->cs_OOPBase;
    
    EnterFunc(bug("free_gfxhiddclass(csd=%p)\n", csd));

    delete_pixfmts(csd);
    ReleaseAttrBases(csd->attrBases, interfaces, NUM_ATTRBASES, OOPBase);
    CloseLibrary(csd->cs_UtilityBase);

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
    struct Library *OOPBase = csd->cs_OOPBase;

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
    struct Library *OOPBase = csd->cs_OOPBase;

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
    struct Library *OOPBase = csd->cs_OOPBase;
			
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

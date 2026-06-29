/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: Gfx Hidd driver class implementation.
*/

/****************************************************************************************/

#include "gfx_debug.h"

#include <aros/atomic.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/config.h>
#include <cybergraphx/cgxvideo.h>
#include <exec/lists.h>
#include <oop/static_mid.h>
#include <graphics/displayinfo.h>
#include <graphics/view.h>

#include "gfx_intern.h"

#include <string.h>
#include <stddef.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>
#include <exec/memory.h>

#include <utility/tagitem.h>

#include LC_LIBDEFS_FILE

#include <hidd/gfx.h>

/*****************************************************************************************

    NAME
        --background_graphics--

    LOCATION
        hidd.gfx.driver

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
        hidd.gfx.driver

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

static BOOL register_modes(OOP_Class *cl, OOP_Object *o, struct TagItem *modetags);

static BOOL alloc_mode_db(struct mode_db *mdb, ULONG numsyncs, ULONG numpfs, OOP_Class *cl);
static VOID free_mode_db(struct mode_db *mdb, OOP_Class *cl);

static struct pixfmt_data *find_pixfmt(HIDDT_PixelFormat *tofind
        , struct class_static_data *_csd);

static VOID copy_bm_and_colmap(OOP_Class *cl, OOP_Object *o,  OOP_Object *src_bm
        , OOP_Object *dst_bm, UWORD width, UWORD height);

BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf, ULONG attrcheck, struct class_static_data *_csd);

/****************************************************************************************/

#define COMPUTE_HIDD_MODEID(sync, pf)   \
    ( ((sync) << 8) | (pf) )
    
#define MODEID_TO_SYNCIDX(id) (((id) & 0X0000FF00) >> 8)
#define MODEID_TO_PFIDX(id)   ( (id) & 0x000000FF)

/****************************************************************************************/

OOP_Object *GFXHIDD__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct TagItem new_tags[] =
    {
        {aHidd_Name,            0               },
        {aHidd_HardwareName,    0               },
        {aHidd_ProducerName,    0               },
        {TAG_MORE,              (IPTR)msg->attrList   }
    };
    struct pRoot_New new_msg =
    {
        .mID      = msg->mID,
        .attrList = new_tags
    };

    D(bug("Entering gfx.hidd::New\n"));

    new_tags[0].ti_Data = GetTagData(aHidd_Name, (IPTR)"gfx.hidd", msg->attrList);
    new_tags[1].ti_Data = GetTagData(aHidd_HardwareName, (IPTR)"Software Rasterizer", msg->attrList);
    new_tags[2].ti_Data = GetTagData(aHidd_ProducerName, (IPTR)"The AROS development team", msg->attrList);

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&new_msg);

    D(bug("Got object o=%x\n", o));

    if (o)
    {
        struct HiddGfxData *data = OOP_INST_DATA(cl, o);
        struct TagItem *tstate = msg->attrList;
        struct TagItem *tag;

        data->fbmode = -1;

        while ((tag = NextTagItem(&tstate)))
        {
            ULONG idx;

            Hidd_Gfx_Switch(tag->ti_Tag, idx)
            {
            case aoHidd_Gfx_FrameBufferType:
                data->fbmode = tag->ti_Data;
                break;
            }
        }
    }

    D(bug("Leaving gfx.hidd::New o=%x\n", o));
    return o;
}

/****************************************************************************************/

VOID GFXHIDD__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, msg);
}

/*****************************************************************************************

    NAME
        aoHidd_Gfx_IsWindowed

    SYNOPSIS
        [..G], BOOL

    LOCATION
        hidd.gfx.driver

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
        hidd.gfx.driver

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
        hidd.gfx.driver

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
            { aHidd_PixFmt_RedShift     , 24                            },
            { aHidd_PixFmt_GreenShift   , 16                            },
            { aHidd_PixFmt_BlueShift    , 8                             },
            { aHidd_PixFmt_AlphaShift   , 0                             },
            { aHidd_PixFmt_RedMask      , 0x000000FF                    },
            { aHidd_PixFmt_GreenMask    , 0x0000FF00                    },
            { aHidd_PixFmt_BlueMask     , 0x00FF0000                    },
            { aHidd_PixFmt_AlphaMask    , 0x00000000                    },
            { aHidd_PixFmt_ColorModel   , vHidd_ColorModel_TrueColor    },
            { aHidd_PixFmt_Depth        , 24                            },
            { aHidd_PixFmt_BytesPerPixel, 4                             },
            { aHidd_PixFmt_BitsPerPixel , 24                            },
            { aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_Native        },
            { aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Chunky       },
            { TAG_DONE                  , 0UL                           }
        };

        // 640x480 resolution
        struct TagItem tags_800_600[] =
        {
            { aHidd_Sync_HDisp          , 640                    },
            { aHidd_Sync_VDisp          , 480                    },
            { TAG_DONE                  , 0UL                    }
        };

        // 800x600 resolution
        struct TagItem tags_800_600[] =
        {
            { aHidd_Sync_HDisp          , 800                    },
            { aHidd_Sync_VDisp          , 600                    },
            { TAG_DONE                  , 0UL                    }
        };

        // 1024x768 resolution
        struct TagItem tags_1024_768[] =
        {
            { aHidd_Sync_HDisp          , 1024                    },
            { aHidd_Sync_VDisp          , 768                     },
            { TAG_DONE                  , 0UL                     }
        };

        // Mode description taglist itself
        struct TagItem mode_tags[] =
        {
            // Our driver supports a single pixelformat
            { aHidd_Gfx_PixFmtTags  , (IPTR)pftags              },

            // Here go sync values common for all sync modes
            { aHidd_Sync_HMin       , 112                       },
            { aHidd_Sync_VMin       , 112                       },
            { aHidd_Sync_HMax       , 16384                     },
            { aHidd_Sync_VMax       , 16384                     },
            { aHidd_Sync_Description, (IPTR)"Example: %hx%v"    },

            // First resolution
            { aHidd_Gfx_SyncTags    , (IPTR)tags_800_600        },

            // Next two syncs will have HMax = 32768, as an example
            { aHidd_Sync_HMax       , 32768                     },

            // Two more resolutions
            { aHidd_Gfx_SyncTags    , (IPTR)tags_800_600        },
            { aHidd_Gfx_SyncTags    , (IPTR)tags_1024_768       },
            { TAG_DONE              , 0UL                       }
        };
    
        // This is the attribute list which is given to New method
        // of the base class
        struct TagItem mytags[] =
        {
            { aHidd_Gfx_ModeTags        , (IPTR)mode_tags       },
            { TAG_DONE                  , NULL                  }
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
        hidd.gfx.driver

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
        hidd.gfx.driver

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
        hidd.gfx.driver

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
        hidd.gfx.driver

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
        hidd.gfx.driver

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
        hidd.gfx.driver

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
        hidd.gfx.driver

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
        hidd.gfx.driver

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
        hidd.gfx.driver

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
        hidd.gfx.driver

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
    struct HiddGfxData *data = OOP_INST_DATA(cl, o);

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
        hidd.gfx.driver

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
        hidd.gfx.driver

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

VOID GFXHIDD__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    Hidd_Gfx_Switch (msg->attrID, idx)
    {
    case aoHidd_Gfx_SupportsDisplayChange:
    case aoHidd_Gfx_IsWindowed:
    case aoHidd_Gfx_SupportsHWCursor:
    case aoHidd_Gfx_NoFrameBuffer:
        *msg->storage = 0;
        return;

    case aoHidd_Gfx_DriverName:
        *msg->storage = (IPTR)OOP_OCLASS(o)->ClassNode.ln_Name;
        return;

    case aoHidd_Gfx_FrameBufferType:
        *msg->storage = get_fbmode(cl, o);
        return;

    case aoHidd_Gfx_DisplayList:
    case aoHidd_Gfx_DisplayDefault:
        *msg->storage = 0;
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}





/****************************************************************************************/

/****************************************************************************************/

/* modebm functions pfidx is x and syncidx is y coord in the bitmap */

/****************************************************************************************/













/*****************************************************************************************

    NAME
        moHidd_Gfx_CopyBox

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Gfx_CopyBox *msg);

        VOID HIDD_Gfx_CopyBox(OOP_Object *gfxHidd, OOP_Object *src, WORD srcX, WORD srcY,
                              OOP_Object *dest, WORD destX, WORD destY, UWORD width, UWORD height,
                              OOP_Object *gc);

    LOCATION
        hidd.gfx.driver

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

/*
 * Real CopyBox implementation. The public method below first de-masquerades any
 * CursorFB-wrapped operands to their real bitmaps (so the direct HBM()/buffer
 * access here is valid) and brackets the operation with the software pointer
 * hide/show.
 */
static VOID copybox_impl(OOP_Class *cl, OOP_Object *obj, struct pHidd_Gfx_CopyBox *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    WORD                            x, y;
    WORD                            srcX = msg->srcX, destX = msg->destX;
    WORD                            srcY = msg->srcY, destY = msg->destY;
    WORD                            startX, endX, deltaX, startY, endY, deltaY;
    ULONG                           memFG;
    HIDDT_PixelFormat               *srcpf, *dstpf;
    OOP_Object                      *dest, *src;
    OOP_Object                      *gc;
    APTR                            srcPixels  = NULL;
    APTR                            destPixels = NULL;

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
                 *        (optimized to either per-line or bulk memcpy()). But it can't handle
                 *        overlapping regions (which seems to be a requirement for CopyBox).
                 *        If this is fixed, we can even throw away HIDD_BM_CopyMemBoxXX at all, reducing
                 *        kickstart size.
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
                    HIDDT_Color col;
                    
                    pix = HIDD_BM_GetPixel(src, srcX + x, srcY + y);
                    col = ctab[pix];

                    /*
                     * !!!! HIDD_BM_MapColor() pokes pixval of the passed HIDDT_Color,
                     * so if the address of the entry in the colormap is passed, the colormap
                     * itself will be modified, if we pass "&ctab[pix]" to HIDD_BM_MapColor() !!!!
                     */
        
                    GC_FG(gc) = HIDD_BM_MapColor(msg->dest, &col);
                    HIDD_BM_DrawPixel(msg->dest, gc, destX + x, destY + y);
                    
                }
            }
        }
        
    } /* if (HIDD_PF_COLMODEL(srcpf) == HIDD_PF_COLMODEL(dstpf)) else ... */
    
    GC_FG(gc) = memFG;
}

VOID GFXHIDD__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *obj, struct pHidd_Gfx_CopyBox *msg)
{
    OOP_Object *realsrc = cursorfb_realbitmap(CSD(cl), msg->src);
    OOP_Object *realdst = cursorfb_realbitmap(CSD(cl), msg->dest);
    OOP_Object *display = NULL;
    struct SignalSemaphore *sem = NULL;
    BOOL bracket = FALSE;

    if (realdst)
        display = cursorfb_display(CSD(cl), msg->dest);
    else if (realsrc)
        display = cursorfb_display(CSD(cl), msg->src);

    if (display)
    {
        WORD w = msg->width, h = msg->height;

        sem = GfxDisplay_CursorSem(CSD(cl), display);
        ObtainSemaphore(sem);

        if (realdst && GfxDisplay_CursorIntersects(CSD(cl), display, realdst,
                msg->destX, msg->destY, msg->destX + w - 1, msg->destY + h - 1))
            bracket = TRUE;
        if (realsrc && GfxDisplay_CursorIntersects(CSD(cl), display, realsrc,
                msg->srcX, msg->srcY, msg->srcX + w - 1, msg->srcY + h - 1))
            bracket = TRUE;

        if (bracket)
            GfxDisplay_CursorRemove(CSD(cl), display);
    }

    /* De-masquerade the operands so the implementation sees real bitmaps */
    if (realsrc)
        msg->src = realsrc;
    if (realdst)
        msg->dest = realdst;

    copybox_impl(cl, obj, msg);

    if (display)
    {
        if (bracket)
            GfxDisplay_CursorRender(CSD(cl), display);
        ReleaseSemaphore(sem);
    }
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
        hidd.gfx.driver

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
#define NUMPIX 4096     /* Not that much room to spare */
#else
#define NUMPIX 100000
#endif

static IPTR copyboxmasked_impl(OOP_Class *cl, OOP_Object *obj, struct pHidd_Gfx_CopyBoxMasked *msg)
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
            UBYTE       *mask;
            UWORD        x, y;

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
            case vHidd_GC_DrawMode_Or:   /* (ABC|ABNC|ANBC) if copy source and blit thru mask */
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
                                 
                                /*
                                 * !!!! HIDD_BM_MapColor() pokes pixval of the passed HIDDT_Color,
                                 * so if the address of the entry in the colormap is passed, the colormap
                                 * itself will be modified, if we pass "&ctab[pix]" to HIDD_BM_MapColor() !!!!
                                 */
                                 
                                HIDDT_Color col = ctab[pix];

                                pix = HIDD_BM_MapColor(msg->dest, &col);
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

IPTR GFXHIDD__Hidd_Gfx__CopyBoxMasked(OOP_Class *cl, OOP_Object *obj, struct pHidd_Gfx_CopyBoxMasked *msg)
{
    OOP_Object *realsrc = cursorfb_realbitmap(CSD(cl), msg->src);
    OOP_Object *realdst = cursorfb_realbitmap(CSD(cl), msg->dest);
    OOP_Object *display = NULL;
    struct SignalSemaphore *sem = NULL;
    BOOL bracket = FALSE;
    IPTR ret;

    if (realdst)
        display = cursorfb_display(CSD(cl), msg->dest);
    else if (realsrc)
        display = cursorfb_display(CSD(cl), msg->src);

    if (display)
    {
        WORD w = msg->width, h = msg->height;

        sem = GfxDisplay_CursorSem(CSD(cl), display);
        ObtainSemaphore(sem);

        if (realdst && GfxDisplay_CursorIntersects(CSD(cl), display, realdst,
                msg->destX, msg->destY, msg->destX + w - 1, msg->destY + h - 1))
            bracket = TRUE;
        if (realsrc && GfxDisplay_CursorIntersects(CSD(cl), display, realsrc,
                msg->srcX, msg->srcY, msg->srcX + w - 1, msg->srcY + h - 1))
            bracket = TRUE;

        if (bracket)
            GfxDisplay_CursorRemove(CSD(cl), display);
    }

    if (realsrc)
        msg->src = realsrc;
    if (realdst)
        msg->dest = realdst;

    ret = copyboxmasked_impl(cl, obj, msg);

    if (display)
    {
        if (bracket)
            GfxDisplay_CursorRender(CSD(cl), display);
        ReleaseSemaphore(sem);
    }

    return ret;
}



/****************************************************************************************/


/****************************************************************************************/















/****************************************************************************************/

/****************************************************************************************/

#undef csd

static inline BOOL cmp_pfs(HIDDT_PixelFormat *tmppf, HIDDT_PixelFormat *dbpf)
{
    /* Just compare everything except stdpixfmt */
    /* Compare flags first (because it's a fast check) */
    if (tmppf->flags != dbpf->flags)
        return FALSE;
    /* If they match, compare the rest of things */
    return !memcmp(tmppf, dbpf, offsetof(HIDDT_PixelFormat, stdpixfmt));
}



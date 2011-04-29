/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics bitmap class implementation.
    Lang: english
*/

/****************************************************************************************/

#include <string.h>
#include <stdlib.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <oop/oop.h>
#include <oop/static_mid.h>
#include <graphics/text.h>
#include <graphics/scale.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

#define POINT_OUTSIDE_CLIP(gc, x, y)	\
	(  (x) < GC_CLIPX1(gc)		\
	|| (x) > GC_CLIPX2(gc)		\
	|| (y) < GC_CLIPY1(gc)		\
	|| (y) > GC_CLIPY2(gc) )

/*****************************************************************************************

    NAME
	--background_bitmap--

    LOCATION
	hidd.graphics.bitmap

    NOTES
	Every display driver should implement at least one bitmap class for displayable
	bitmaps.

	Normally this class doesn't need to have public ID. In order to use it the driver
	should pass class pointer as aoHidd_BitMap_ClassPtr value to the graphics base class
	in its moHidd_Gfx_NewBitMap implementation.

	BitMap base class is in C++ terminology a pure virtual
	baseclass. It will not allocate any bitmap data at all;
	that is up to the subclass to do.

	The main task of the BitMap baseclass is to store some information about the bitmap
	like its size and pixelformat. A pixelformat is an object of private class which
	stores the actual information about the format.

	There are two ways that we can find out the pixfmt in our moHidd_Gfx_NewBitMap
	implementation:

	Displayable bitmap -
	    The tags will contain a modeid.
   	    One can use this modeid to get a pointer to an
	    already registered pixfmt.

	Non-displayable bitmap -
	    The aoHidd_BitMap_StdPixFmt or aoHidd_BitMap_Friend attribute will always be
	    passed.

*****************************************************************************************/

#define GOT_BM_ATTR(code)   GOT_ATTR(code, aoHidd_BitMap, bitmap)
#define AO(x) 	    	    aoHidd_BitMap_ ## x

#define BMAF(x)     	    (1L << aoHidd_BitMap_ ## x)

#define BM_NONDISP_AF 	    ( BMAF(Width) | BMAF(Height) | BMAF(PixFmt) )


#define PIXBUFBYTES 	    (50000*4)

#define PIXBUF_DECLARE_VARS 	\
    LONG __buflines; 	    	\
    LONG __bufy = 0;	    	\
    LONG __worky = 0;	    	\
    LONG __height;
    
#define PIXBUF_ALLOC(buf,bytesperline,height)      	    	\
    __height = height;	    	    	    	    	    	\
    __buflines = PIXBUFBYTES / (bytesperline);  	    	\
    if (__buflines == 0)      	    	    	    	    	\
    { 	    	    	    	    	    	    	    	\
    	__buflines = 1; 	    	    	    	    	\
    } 	    	    	    	    	    	    	    	\
    else if (__buflines > __height)     	    	    	\
    { 	    	    	    	    	    	    	    	\
    	__buflines = __height;  	    	    	    	\
    } 	    	    	    	    	    	    	    	\
    buf = AllocVec((bytesperline) * __buflines, MEMF_PUBLIC); 	\
    if (!buf && (__buflines > 1))     	    	    	    	\
    { 	    	    	    	    	    	    	    	\
    	__buflines = 1; 	    	    	    	    	\
	buf = AllocVec((bytesperline), MEMF_PUBLIC);  	    	\
    }
    
#define PIXBUF_TIME_TO_START_PROCESS  	(__bufy == __worky)

#define PIXBUF_LINES_TO_PROCESS     	(((__height - __bufy) > __buflines) ? 	    \
    	    	    	    	    	__buflines : __height - __bufy )

#define PIXBUF_TIME_TO_END_PROCESS  	(((__worky - __bufy + 1) == __buflines) || \
    	    	    	    	     	(__worky == __height - 1))

#define PIXBUF_NEXT_LINE    	    	    	    	    	\
    __worky++;      	    	    	    	    	    	\
    if ((__worky - __bufy) == __buflines) __bufy = __worky;
    
        
#define PIXBUF_FREE(buf) \
    if (buf) FreeVec(buf);

/*****************************************************************************************

    NAME
        aoHidd_BitMap_Width

    SYNOPSIS
        [ISG], ULONG

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Specifies bitmap width in pixels.
	
	Setting this attribute does not cause actual bitmap resize, just updates the information
	about it. Use this only from within subclasses only if you know what you do. For example
	SDL hosted driver sets it when framebufer changes the resolution.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_BitMap_Height

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_Height

    SYNOPSIS
        [ISG], ULONG

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Specifies bitmap height in pixels.
	
	Setting this attribute does not cause actual bitmap resize, just updates the information
	about it. Use this only from within subclasses only if you know what you do. For example
	SDL hosted driver sets it when framebufer changes the resolution.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_BitMap_Width

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_Displayable

    SYNOPSIS
        [I.G], BOOL

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        The bitmap is displayable. A displayable bitmap is always managed by a display
	driver and must have valid display mode ID specification.

	If this attribute is not supplied during bitmap creation, its value defaults
	to FALSE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_BitMap_ModeID

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_Visible

    SYNOPSIS
        [..G], BOOL

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Check if the bitmap is currently visible on screen

    NOTES

    EXAMPLE

    BUGS
	Not all display drivers implement this attribute. No AROS components currently rely
	on its value.

    SEE ALSO

    INTERNALS
	Some drivers may choose to have this attribute internally setable. Do not rely on it
	in any way and do not attempt to set it manually from within applications, this will
	not do any nice things.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_IsLinearMem

    SYNOPSIS
        [..G], BOOL

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Check if the bitmap provides linear memory access. This means that bitmap's
	pixelbuffer is directly addressable by the CPU.

	Bitmaps with no linear memory may implement moHidd_BitMap_ObtainDirectAccess,
	but this means that this method will rely on mirrored buffer. In such a case
	the user must call moHidd_BitMap_UpdateRect after modifying bitmap's contents.

    NOTES
	Used by cybergraphics.library/GetCyberMapAttr() for providing CYBRMATTR_ISLINEARMEM
	value.

    EXAMPLE

    BUGS
	Currently no display drivers implement this attribute despite many native mode
	drivers actually provide linear memory.

    SEE ALSO
	moHidd_BitMap_ObtainDirectAccess, moHidd_BitMap_ReleaseDirectAccess,
	moHidd_BitMap_UpdateRect

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_BytesPerRow

    SYNOPSIS
        [..G], ULONG

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Query number of bytes per row in the bitmap storage buffer

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_ColorMap

    SYNOPSIS
        [..G], OOP_Object *

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Return associated colormap (palette) object.

	By default only displayable bitmaps have colormaps. However a colormap can be attached
	to any bitmap using moHidd_BitMap_SetColors or moHidd_BitMap_SetColorMap.

	Note that manual attaching of a colormap to a nondisplayable bitmap may cause undesired
	side-effects on graphics.library behavior. It's better not to do this at all. The system
	knows what it does better than you.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_BitMap_SetColorMap, moHidd_BitMap_SetColors.

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_Friend

    SYNOPSIS
        [I.G], OOP_Object *

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Specify a friend bitmap. The bitmap will be allocated so that it
        is optimized for blitting to this bitmap.

	Display drivers may query this attribute and then query friend bitmap
	for anything they want (like pixelformat, mode ID, etc).

	Note that explicit specification of mode ID and/or standard pixelformat
	should override defaults provided by friend bitmap (i.e. actually breaking
	the friendship).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_GfxHidd

    SYNOPSIS
        [I.G], OOP_Object *

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Specify display driver object this bitmap was created with.

	Normally the user doesn't have to supply this attribute. Instead you should use
	driver's moHidd_Gfx_NewBitMap method in order to create bitmaps. In this case
	aoHidd_BitMap_GfxHidd attribute will be provided by graphics driver base class
	with the correct value.

	It is illegal to manually create bitmap objects with no driver associated.
	graphics.library maintains at least a memory driver for nondisplayable
	bitmaps in system RAM without any acceleration.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CLID_Hidd_Gfx/moHidd_Gfx_NewBitMap

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_StdPixFmt

    SYNOPSIS
        [I..], HIDDT_StdPixFmt

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Specify standard pixelformat code (one of vHidd_StdPixFmt_... values) for the
	bitmap.

	Values less than num_Hidd_PseudoStdPixFmt are illegal for this attribute.

	Actually the bitmap class itself ignores this attribute. It is processed by
	CLID_Hidd_Gfx/moHidd_Gfx_NewBitMap method in order to look up a corresponding
	pixelformat object in the system's database.

    NOTES
    	Bitmaps with this attribute set should be created as RAM bitmaps with direct CPU
	access. It is not recommended to replace them with, for example, virtual surfaces on
	hosted AROS. Such bitmaps are expected to be directly addressable and breaking
	this may cause undesired side effects.

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_BitMap_PixFmt, CLID_Hidd_Gfx/moHidd_Gfx_NewBitMap

    INTERNALS
    	Currently all display drivers omit specifying own bitmap class for bitmaps with this
	attribute set, letting base class (actually memory driver) to select an appropriate
	class for it. This way it ends up in a bitmap of CLID_Hidd_ChunkyBM or CLID_Hidd_PlanarBM
	class. It is recommended to follow this rule. It's not prohibited, however, to do some
	adjustments to the bitmap (like alignment) in order to optimize blitting to/from it.
	In fact if the display driver was asked to create such a bitmap, this means that
	the standard bitmap is being created as a friend of some bitmap which was allocated
	using this driver. This way the bitmap is expected to be friendly to this driver.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_PixFmt

    SYNOPSIS
        [I.G], OOP_Object *

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Returns pixelformat descriptor object associated with the bitmap.

	Every bitmap has some associated pixelformat object. Pixelformat objects are
	shared data storages, so many bitmaps may refer to the same pixelformat objects.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_ModeID

    SYNOPSIS
        [I.G], HIDDT_ModeID

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Specify display mode ID for displayable bitmap.
	
	A displayable bitmap must have this attribute supplied with valid value. A nondisplayable
	one may miss it, however it may remember it if it was created as a friend of displayable
	one. This way you may create another displayable bitmap as a friend of nondisplayable
	one which in turn is a friend of displayable one.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_ClassPtr

    SYNOPSIS
        [I.G], OOP_Class *

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Explicitly specify bitmap's class pointer.

	This attribute is simply remembered by bitmap base class. Its main purpose is to let
	graphics driver base class to select a class on which to call OOP_NewObject() in
	its moHidd_Gfx_NewBitMap implementation.
	
	If neither this attribute nor aoHidd_BitMap_ClassID attribute is provided for
	moHidd_Gfx_NewBitMap, graphics base class will do its best in order to find out the
	correct class based on aoHidd_StdPixFmt attribute value	or aoHidd_BitMap_ClassPtr value
	of friend bitmap.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_BitMap_ClassID, CLID_Hidd_Gfx/moHidd_Gfx_NewBitMap

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_ClassID

    SYNOPSIS
        [I..]

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Explicitly specify bitmap's class ID.

	The purpose of this attribute is to let	graphics driver base class to select a class
	on which to call OOP_NewObject() in its moHidd_Gfx_NewBitMap implementation.

	If neither this attribute nor aoHidd_BitMap_ClassPtr attribute is provided for
	moHidd_Gfx_NewBitMap, graphics base class will do its best in order to find out the
	correct class based on aoHidd_StdPixFmt attribute value	or aoHidd_BitMap_ClassPtr value
	of friend bitmap.

    NOTES

    EXAMPLE

    BUGS
	The pointer to a given class will not be remembered as aoHidd_BitMap_ClassPtr value.

    SEE ALSO
	aoHidd_BitMap_ClassPtr, CLID_Hidd_Gfx/moHidd_Gfx_NewBitMap

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_PixFmtTags

    SYNOPSIS
        [I..]

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Only used by subclasses of BitMap class.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_FrameBuffer

    SYNOPSIS
        [I..], BOOL

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Specifies that the bitmap is a framebuffer bitmap.

	A detailed description of a framebuffer is given in CLID_Hidd_Gfx/moHidd_Gfx_NewBitMap
	and in CLID_Hidd_Gfx/moHidd_Gfx_Show documentation.

	Specifying this attribute to moHidd_Gfx_NewBitMap method causes also implicit setting
	of aoHidd_BitMap_Displayable to TRUE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_LeftEdge

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Controls horizontal position of a scrollable screen bitmap.

	Size of displayable bitmaps may differ from actual screen size. In this case the
	bitmap can be scrolled around the whole display area. If the bitmap is larger than
	the display, only its part can be visible.

	Setting this attribute causes changing left origin point of the bitmap. The value
	of this attribute represents an offset from the physical edge of the display to the
	logical edge of the bitmap. This means that if a large bitmap scrolls to the left in
	order to reveal its right part, the offset will be negative. If the bitmap scrolls
	to the left (possibly revealing another bitmap behind it), the offset will be positive.

	It's up to the display driver to set scroll limits. If the value of the attribute
	becomes unacceptable for any reason, the driver should adjust it and provide the real
	resulting value back.

    NOTES
	Implementing screen scrolling does not enforce to implement screen composition, despite
	the composition is really based on scrolling (in case of composition scrolling a bitmap
	off-display is expected to reveal another bitmap behing it instead of empty space).

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_BitMap_TopEdge

    INTERNALS
	Base class will always provide zero value for this attribute and ignore all attempts
	to set it. This means that by default bitmaps don't scroll and this needs explicit
	implementation in the display driver.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_TopEdge

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Controls vertical position of a scrollable screen bitmap.

	Size of displayable bitmaps may differ from actual screen size. In this case the
	bitmap can be scrolled around the whole display area. If the bitmap is larger than
	the display, only its part can be visible.

	Setting this attribute causes changing top origin point of the bitmap. The value
	of this attribute represents an offset from the physical edge of the display to the
	logical edge of the bitmap. This means that if a large bitmap scrolls upwards in
	order to reveal its bottom part, the offset will be negative. If the bitmap scrolls
	downdards (possibly revealing another bitmap behind it), the offset will be positive.

	It's up to the display driver to set scroll limits. If the value of the attribute
	becomes unacceptable for any reason, the driver should adjust it and provide the real
	resulting value back.

    NOTES
	Implementing screen scrolling does not enforce to implement screen composition, despite
	the composition is really based on scrolling (in case of composition scrolling a bitmap
	off-display is expected to reveal another bitmap behing it instead of empty space).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_BitMap_Align

    SYNOPSIS
        [I..]

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Specify number of pixels to align bitmap data width to.

	This attribute can be added in CLID_Hidd_Gfx/moHidd_Gfx_NewBitMap implementation
	in order to enforce alignment needed for example by blitting hardware.

    NOTES

    EXAMPLE

    BUGS
	This attribute does not affect aoHidd_BitMap_BytesPerRow value.

	This attribute is supported only by CLID_Hidd_PlanarBM class at the moment.

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/****************************************************************************************/

OOP_Object *BM__Root__New(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
    EnterFunc(bug("BitMap::New()\n"));

    obj  = (OOP_Object *) OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);

    if (NULL != obj)
    {
	struct TagItem      	colmap_tags[] =
	{
	    { aHidd_ColorMap_NumEntries , 16	},
	    { TAG_DONE	    	    	     	}
	};
    	struct HIDDBitMapData 	*data;
	BOOL 	    	    	ok = TRUE;

    	DECLARE_ATTRCHECK(bitmap);
    	IPTR attrs[num_Total_BitMap_Attrs] = {0};

        data = OOP_INST_DATA(cl, obj);

        /* clear all data and set some default values */
        memset(data, 0, sizeof(struct HIDDBitMapData));

        data->width         = 320;
        data->height        = 200;
        data->reqdepth	    = 8;
        data->displayable   = FALSE;
	data->pf_registered = FALSE;
	data->modeid 	    = vHidd_ModeID_Invalid;

	if (0 != OOP_ParseAttrs(msg->attrList, attrs, num_Total_BitMap_Attrs,
	    	    	    	&ATTRCHECK(bitmap), HiddBitMapAttrBase))
	{
	    D(bug("!!! ERROR PARSING ATTRS IN BitMap::New() !!!\n"));
	    D(bug("!!! NUMBER OF ATTRS IN IF: %d !!!\n", num_Total_BitMap_Attrs));
	    ok = FALSE;
	}

	if (ok)
	{
	    if (!GOT_BM_ATTR(GfxHidd))
	    {
    	    	D(bug("!!!! BM CLASS DID NOT GET GFX HIDD !!!\n"));
	    	D(bug("!!!! The reason for this is that the gfxhidd subclass NewBitmap() method\n"));
	    	D(bug("!!!! has not left it to the baseclass to actually create the object,\n"));
	    	D(bug("!!!! but rather done it itself. This MUST be corrected in the gfxhidd subclass\n"));
		D(bug("!!!! ATTRCHECK: %p !!!!\n", ATTRCHECK(bitmap)));

	    	ok = FALSE;
	    }
	    else
	    {
	    	data->gfxhidd = (OOP_Object *)attrs[AO(GfxHidd)];
	    }
	}

	/* Save pointer to friend bitmap */
	if (GOT_BM_ATTR(Friend))
	    data->friend = (OOP_Object *)attrs[AO(Friend)];

	if (ok)
	{
	    if ( attrs[AO(Displayable)] )
	    {
		/* We should allways get modeid, but we check anyway */
		if (!GOT_BM_ATTR(ModeID))
		{
		    D(bug("!!! BitMap:New() DID NOT GET MODEID FOR DISPLAYABLE BITMAP !!!\n"));
		    ok = FALSE;
		}
		else
		{
	    	    HIDDT_ModeID    modeid;
	    	    OOP_Object      *sync, *pf;

		    modeid = (HIDDT_ModeID)attrs[AO(ModeID)];

	    	    if (!HIDD_Gfx_GetMode(data->gfxhidd, modeid, &sync, &pf))
		    {
	    		D(bug("!!! BitMap::New() RECEIVED INVALID MODEID %p\n", modeid));
	    		ok = FALSE;
	    	    }
		    else
		    {
	    		IPTR width, height;

	    		/* Update the bitmap with data from the modeid */
			OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
			OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

			data->width = width;
			data->height = height;
			data->displayable = TRUE;

		        /* The PixFmt is allready registered and locked in the PixFmt database */
			data->prot.pixfmt = pf;
		    }
	    	}
	    }
	    else
	    {  /* displayable */
		if (BM_NONDISP_AF != (BM_NONDISP_AF & ATTRCHECK(bitmap)))
		{
		    if (OOP_OCLASS(obj) != CSD(cl)->planarbmclass)
		    {
	    		/* HACK. This is an ugly hack to allow the
		           late initialization of BitMap objects in AROS.

		           The PixelFormat will be set later.
			*/

    	    	    	/* FIXME: Find a better way to do this. */

	    	    	/* One could maybe fix this by implementing a separate AmigaPitMap class
	    	    	*/

			D(bug("!!! BitMap:New(): NO PIXFMT FOR NONDISPLAYABLE BITMAP !!!\n"));
			ok = FALSE;
		    }
		}
		else
		    data->prot.pixfmt = (OOP_Object *)attrs[AO(PixFmt)];
	    } /* displayable */
	    if (GOT_BM_ATTR(Width))
	        data->width = attrs[AO(Width)];
	    if (GOT_BM_ATTR(Height))
	        data->height = attrs[AO(Height)];
	    if (GOT_BM_ATTR(Depth))
	        data->reqdepth = attrs[AO(Depth)];
	} /* if (ok) */

	if (ok)
	{
	    /* initialize the direct method calling */

	    if (GOT_BM_ATTR(ModeID))
	    	data->modeid = attrs[AO(ModeID)];
	    data->classptr = (OOP_Class *)attrs[AO(ClassPtr)];

    	#if USE_FAST_PUTPIXEL
	    data->putpixel = (IPTR (*)(OOP_Class *, OOP_Object *, struct pHidd_BitMap_PutPixel *))
			     OOP_GetMethod(obj, CSD(cl)->putpixel_mid);
	    if (NULL == data->putpixel)
		ok = FALSE;
    	#endif

    	#if USE_FAST_GETPIXEL
	    data->getpixel = (IPTR (*)(OOP_Class *, OOP_Object *, struct pHidd_BitMap_GetPixel *))
			     OOP_GetMethod(obj, CSD(cl)->getpixel_mid);
	    if (NULL == data->getpixel)
		ok = FALSE;
    	#endif

    	#if USE_FAST_DRAWPIXEL
	    data->drawpixel = (IPTR (*)(OOP_Class *, OOP_Object *, struct pHidd_BitMap_DrawPixel *))
			      OOP_GetMethod(obj, CSD(cl)->drawpixel_mid);
	    if (NULL == data->drawpixel)
		ok = FALSE;
    	#endif

	    /* Try to create the colormap */

    	    /* stegerg: Only add a ColorMap for a visible bitmap (screen). This
	                is important because one can create for example a bitmap
			in PIXFMT_LUT8 without friend bitmap and then copy this
			bitmap to a 16 bit screen. During copy the screen bitmap
			CLUT must be used, which would not happen if our PIXFMT_LUT8
			also had a colormap itself because then bltbitmap would use the
			colormap of the PIXFMT_LUT8 bitmap as lookup, which in this
			case would just cause everything to become black in the
			destination (screen) bitmap, because noone ever sets up the
			colormap of the PIXFMT_LUT8 bitmap */

    	    if (data->displayable)
	    {
		data->colmap = OOP_NewObject(NULL, CLID_Hidd_ColorMap, colmap_tags);
		if (NULL == data->colmap)
		    ok = FALSE;
	    }
	}


	if (!ok)
	{
	    ULONG dispose_mid;

	    dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

	    OOP_CoerceMethod(cl, obj, (OOP_Msg)&dispose_mid);

	    obj = NULL;

    	} /* if(obj) */

    } /* if (NULL != obj) */

    ReturnPtr("BitMap::New", OOP_Object *, obj);
}

/****************************************************************************************/

void BM__Root__Dispose(OOP_Class *cl, OOP_Object *obj, OOP_Msg *msg)
{
    struct HIDDBitMapData *data = OOP_INST_DATA(cl, obj);

    EnterFunc(bug("BitMap::Dispose()\n"));

    if (NULL != data->colmap)
    	OOP_DisposeObject(data->colmap);

    D(bug("Calling super\n"));

    /* Release the previously registered pixel format */
    if (data->pf_registered)
    	HIDD_Gfx_ReleasePixFmt(data->gfxhidd, data->prot.pixfmt);

    OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);

    ReturnVoid("BitMap::Dispose");
}

/****************************************************************************************/

VOID BM__Root__Get(OOP_Class *cl, OOP_Object *obj, struct pRoot_Get *msg)
{
    struct HIDDBitMapData *data = OOP_INST_DATA(cl, obj);
    ULONG   	    	   idx;

    EnterFunc(bug("BitMap::Get() attrID: %i  storage: %p\n", msg->attrID, msg->storage));

    if(IS_BITMAP_ATTR(msg->attrID, idx))
    {
        switch(idx)
        {
            case aoHidd_BitMap_Width:
	    	 *msg->storage = data->width;
		 D(bug("  width: %i\n", data->width));
		 break;

            case aoHidd_BitMap_Height:
	    	*msg->storage = data->height;
		break;

            case aoHidd_BitMap_Depth:
	    	*msg->storage = data->reqdepth;
		break;

            case aoHidd_BitMap_Displayable:
	    	*msg->storage = (IPTR) data->displayable;
		break;

	    case aoHidd_BitMap_PixFmt:
	    	*msg->storage = (IPTR)data->prot.pixfmt;
		break;

	    case aoHidd_BitMap_Friend:
	    	*msg->storage = (IPTR)data->friend;
		break;

	    case aoHidd_BitMap_ColorMap:
	    	*msg->storage = (IPTR)data->colmap;
		break;

    	#if 0
            case aoHidd_BitMap_Depth:
	    	if (NULL != data->prot.pixfmt)
		{
	    	    *msg->storage = ((HIDDT_PixelFormat *)data->prot.pixfmt)->depth;
		}
		else
		{
		    *msg->storage = data->reqdepth;
		}
		break;

    	#endif

	    case aoHidd_BitMap_GfxHidd:
	    	*msg->storage = (IPTR)data->gfxhidd;
		break;

	    case aoHidd_BitMap_ModeID:
	    	*msg->storage = data->modeid;
		break;

	    case aoHidd_BitMap_BytesPerRow:
	    {
	    	HIDDT_PixelFormat *pf;

		pf = (HIDDT_PixelFormat *)data->prot.pixfmt;

		*msg->storage = pf->bytes_per_pixel * data->width;
		break;
	    }
	    
	    /* Generic bitmaps don't scroll. This has to be implemented
	       in the subclass */
	    case aoHidd_BitMap_LeftEdge:
	    case aoHidd_BitMap_TopEdge:
		*msg->storage = 0;
		break;

	    case aoHidd_BitMap_ClassPtr:
		*msg->storage = (IPTR)data->classptr;
		break;

            default:
	    	D(bug("UNKNOWN ATTR IN BITMAP BASECLASS: %d\n", idx));
	    	OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);
		break;
        }
    }
    else
    {
	OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);
    }

    ReturnVoid("BitMap::Get");
}

/****************************************************************************************/

#define UB(x) ((UBYTE *)x)

/*****************************************************************************************

    NAME
        moHidd_BitMap_SetColors

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_SetColors *msg);

        BOOL HIDD_BM_SetColors (OOP_Object *obj, HIDDT_Color *colors,
                                ULONG firstColor, ULONG numColors);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
	Sets values for one or more colors in the colormap object associated with the
	bitmap.

	The colormap will be created if it does not exist.

	Only ARGB values from the source array are taken into account. pixval member is
	updated with the real pixel value for every color.

    INPUTS
        obj        - A bitmap object whose colormap needs to be set
        colors     - A pointer to source data array
        firstColor - Number of the first color to set
        numColors  - Number of subsequent colors to set

    RESULT
	TRUE on success, FALSE in case of some error (like out of memory)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CLID_Hidd_ColorMap/moHidd_ColorMap_SetColors

    INTERNALS

*****************************************************************************************/

BOOL BM__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    /* Copy the colors into the internal buffer */
    struct HIDDBitMapData *data;

    data = OOP_INST_DATA(cl, o);

    /* Subclass has initialized HIDDT_Color->pixelVal field and such.
       Just copy it into the colortab.
    */

    if (NULL == data->colmap)
    {
	struct TagItem colmap_tags[] =
	{
    	    { aHidd_ColorMap_NumEntries, 0  },
	    { TAG_DONE	    	    	    }
   	};

	colmap_tags[0].ti_Data = msg->firstColor + msg->numColors;
    	data->colmap = OOP_NewObject(NULL, CLID_Hidd_ColorMap, colmap_tags);
    }

    if (NULL == data->colmap)
    {
    	return FALSE;
    }

    /* Use the colormap class to set the colors */
    return HIDD_CM_SetColors(data->colmap, msg->colors,
    	    	    	     msg->firstColor, msg->numColors,
			     data->prot.pixfmt);

}

/*****************************************************************************************

    NAME
        moHidd_BitMap_DrawPixel

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawPixel *msg);

        ULONG HIDD_BM_DrawPixel(OOP_Object *obj, OOP_Object *gc, WORD x, WORD y);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Changes the pixel at (x,y). The color of the pixel depends on the
        attributes of gc, eg. colors, drawmode, colormask etc.
        This function does not check the coordinates.

    INPUTS
        obj  - A bitmap to draw on
        gc   - A GC (graphics context) object to use for drawing
        x, y - Coordinates of the pixel to draw

    RESULT
	Undefined. Many drivers declare this method as void.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO
        - Support for shapeplane.
        - Optimize

*****************************************************************************************/

ULONG BM__Hidd_BitMap__DrawPixel(OOP_Class *cl, OOP_Object *obj,
				 struct pHidd_BitMap_DrawPixel *msg)
{
    HIDDT_Pixel     	    	    src, dest, val;
    HIDDT_DrawMode  	    	    mode;
    HIDDT_Pixel     	    	    writeMask;
    OOP_Object      	    	    *gc;
#if USE_FAST_PUTPIXEL
    struct pHidd_BitMap_PutPixel    p;
#endif

/*    EnterFunc(bug("BitMap::DrawPixel() x: %i, y: %i\n", msg->x, msg->y));
*/
    /*
        Example: Pixels which bits are set to 0 in the colMask must be
                 unchanged

          data->colMask = 001111
          dest          = 101100
                          --

          writeMask = ~data->colMask & dest
                    =   110000       & 101100
                    =   100000

          dest = data->fg && dest = 010100
                                    --

          dest      = dest   & (writeMask | data->ColMask)
                    = 010100 & (100000   | 001111)
                    = 010100 & (101111)
                    = 000100
                      --

          dest      = dest   | writeMask;
                    = 000100   100000
                    = 100100
                      --
    */

    gc = msg->gc;

    src       = GC_FG(gc);
    mode      = GC_DRMD(gc);

#if OPTIMIZE_DRAWPIXEL_FOR_COPY
    if (vHidd_GC_DrawMode_Copy == mode && GC_COLMASK(gc) == ~0)
    {
	val = src;
    }
    else
    {
#endif

    dest      = HIDD_BM_GetPixel(obj, msg->x, msg->y);
    writeMask = ~GC_COLMASK(gc) & dest;

    val = 0;

    if(mode & 1) val = ( src &  dest);
    if(mode & 2) val = ( src & ~dest) | val;
    if(mode & 4) val = (~src &  dest) | val;
    if(mode & 8) val = (~src & ~dest) | val;

    val = (val & (writeMask | GC_COLMASK(gc) )) | writeMask;

#if OPTIMIZE_DRAWPIXEL_FOR_COPY
}
#endif

#if USE_FAST_PUTPIXEL
    p.mID	= CSD(cl)->putpixel_mid;
    p.x		= msg->x;
    p.y		= msg->y;
    p.pixel	= val;
    PUTPIXEL(obj, &p);
#else

    HIDD_BM_PutPixel(obj, msg->x, msg->y, val);
#endif

/*    ReturnInt("BitMap::DrawPixel ", ULONG, 1); */ /* in quickmode return always 1 */

    return 1;
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_DrawLine

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawPixel *msg);

        VOID HIDD_BM_DrawLine(OOP_Object *obj, OOP_Object *gc, WORD x1, WORD y1,
                              WORD x2, WORD y2);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Draws a line from (x1,y1) to (x2,y2) in the specified gc.
        The function does not clip the line against the drawing area.

    INPUTS
        obj   - A bitmap to draw on
        gc    - A graphics context object to use
        x1,y1 - start point of the line in pixels
        x2,y2 - end point of the line in pixels

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Uses midpoint line ("Bresenham") algorithm([FOL90] 3.2.2)

    TODO Support for line pattern
         Optimize remove if t == 1 ...
	 Implement better clipping: Should be no reason to calculate
	 more than the part of the line that is inside the cliprect

*****************************************************************************************/

VOID BM__Hidd_BitMap__DrawLine
(
    OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_DrawLine *msg
)
{
    WORD    	dx, dy, incrE, incrNE, d, x, y, s1, s2, t, i;
    LONG 	x1, y1, x2, y2;
    UWORD   	maskLine;  /* for line pattern */
    ULONG   	fg;   /* foreground pen   */
    BOOL    	doclip;
    BOOL    	opaque;
    OOP_Object  *gc;


/* bug("BitMap::DrawLine()\n");
*/    EnterFunc(bug("BitMap::DrawLine() x1: %i, y1: %i x2: %i, y2: %i\n", msg->x1, msg->y1, msg->x2, msg->y2));

    gc = msg->gc;
    doclip = GC_DOCLIP(gc);
    opaque = (GC_COLEXP(gc) & vHidd_GC_ColExp_Opaque) ? TRUE : FALSE;
    fg = GC_FG(gc);

    maskLine = 1 << GC_LINEPATCNT(gc);
    
    if (doclip)
    {
        /* If line is not inside cliprect, then just return */
        /* Normalize coords */
        if (msg->x1 > msg->x2)
        {
            x1 = msg->x2; x2 = msg->x1;
        }
        else
        {
            x1 = msg->x1; x2 = msg->x2;
        }
    
        if (msg->y1 > msg->y2)
        {
            y1 = msg->y2; y2 = msg->y1;
        }
        else
        {
            y1 = msg->y1; y2 = msg->y2;
        }
    
        if (    x1 > GC_CLIPX2(gc)
             || x2 < GC_CLIPX1(gc)
             || y1 > GC_CLIPY2(gc)
             || y2 < GC_CLIPY1(gc) )
        {
    
             /* Line is not inside cliprect, so just return */
             return;
    
        }
    }

    x1 = msg->x1;
    y1 = msg->y1;
    x2 = msg->x2;
    y2 = msg->y2;
    
    if (y1 == y2)
    {
        /*
            Horizontal line drawing code.
        */
        y = y1; 
	
	/* Don't swap coordinates if x2 < x1! Because of linepattern! */
	
	if (x1 < x2)
	{
	    x2++;
	    dx = 1;
	}
	else
	{
	    x2--;
	    dx = -1;
	}
	
        for(i = x1; i != x2; i += dx)
        {    
            /* Pixel inside ? */

            if (!doclip || !POINT_OUTSIDE_CLIP(gc, i, y ))
            {
                if(GC_LINEPAT(gc) & maskLine)
                {
                    HIDD_BM_DrawPixel(obj, gc, i, y);
                }
                else if (opaque)
                {
                    GC_FG(gc) = GC_BG(gc);
                    HIDD_BM_DrawPixel(obj, gc, i, y);
                    GC_FG(gc) = fg;
                }
            }
	    
	    maskLine = maskLine >> 1;
	    if (!maskLine) maskLine = 1L << 15;	    
        }
    }
    else if (x1 == x2)
    {
        /*
            Vertical line drawing code.
        */
        x = x1;

	/* Don't swap coordinates if y2 < y1! Because of linepattern! */
	
	if (y1 < y2)
	{
	    y2++;
	    dy = 1;
	}
	else
	{
	    y2--;
	    dy = -1;
	}
	
        for(i = y1; i != y2; i += dy)
        {    
            /* Pixel inside ? */
            if (!doclip || !POINT_OUTSIDE_CLIP(gc, x, i ))
            {
                if(GC_LINEPAT(gc) & maskLine)
                {
                    HIDD_BM_DrawPixel(obj, gc, x, i);
                }
                else if (opaque)
                {
                    GC_FG(gc) = GC_BG(gc);
                    HIDD_BM_DrawPixel(obj, gc, x, i);
                    GC_FG(gc) = fg;
                }
            }
	    
	    maskLine = maskLine >> 1;
	    if (!maskLine) maskLine = 1L << 15;	    
	    
        }
    }
    else
    {
        /*
            Generic line drawing code.
        */
        /* Calculate slope */
        dx = abs(x2 - x1);
        dy = abs(y2 - y1);
    
        /* which direction? */
        if((x2 - x1) > 0) s1 = 1; else s1 = - 1;
        if((y2 - y1) > 0) s2 = 1; else s2 = - 1;
    
        /* change axes if dx < dy */
        if(dx < dy)
        {
            d = dx;
            dx = dy;
            dy = d;
            t = 0;
        }
        else
        {
           t = 1;
        }
    
        d  = 2 * dy - dx;        /* initial value of d */
    
        incrE  = 2 * dy;         /* Increment use for move to E  */
        incrNE = 2 * (dy - dx);  /* Increment use for move to NE */
    
        x = x1; y = y1;
        
        for(i = 0; i <= dx; i++)
        {    
            /* Pixel inside ? */
            if (!doclip || !POINT_OUTSIDE_CLIP(gc, x, y ))
            {
                if(GC_LINEPAT(gc) & maskLine)
                {
                    HIDD_BM_DrawPixel(obj, gc, x, y);
                }
                else if (opaque)
                {
                    GC_FG(gc) = GC_BG(gc);
                    HIDD_BM_DrawPixel(obj, gc, x, y);
                    GC_FG(gc) = fg;
                }
            }
    
            if(d <= 0)
            {
                if(t == 1)
                {
                    x = x + s1;
                }
                else
                {
                    y = y + s2;
                }
    
                d = d + incrE;
            }
            else
            {
                x = x + s1;
                y = y + s2;
                d = d + incrNE;
            }

            maskLine = maskLine >> 1;
	    if (!maskLine) maskLine = 1L << 15;	    

        }
    }

    ReturnVoid("BitMap::DrawLine ");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_DrawRect

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawRect *msg);

        VOID HIDD_BM_DrawRect (OOP_Object *obj, OOP_Object *gc, WORD minX, WORD minY,
                          WORD maxX, WORD maxY);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Draws a hollow rectangle. minX and minY specifies the upper
        left corner of the rectangle. minY and maxY specifies the lower
        right corner of the rectangle.
        The function does not clip the rectangle against the drawing area.

    INPUTS
        obj        - A bitmap to draw on
        gc         - A GC object to use for drawing
        minX, minY - upper left corner of the rectangle in pixels
        maxX, maxY - lower right corner of the rectangle in pixels

    RESULT
	None.

    NOTES
	This method is not used by the system and considered reserved.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/

VOID BM__Hidd_BitMap__DrawRect(OOP_Class *cl, OOP_Object *obj,
			       struct pHidd_BitMap_DrawRect *msg)
{
    OOP_Object *gc = msg->gc;
    WORD    	addX, addY;

    EnterFunc(bug("BitMap::DrawRect()"));

    if(msg->minX == msg->maxX) addX = 0; else addX = 1;
    if(msg->minY == msg->maxY) addY = 0; else addY = 1;

    HIDD_BM_DrawLine(obj, gc, msg->minX, msg->minY       , msg->maxX, msg->minY);
    HIDD_BM_DrawLine(obj, gc, msg->maxX, msg->minY + addY, msg->maxX, msg->maxY);
    HIDD_BM_DrawLine(obj, gc, msg->maxX - addX, msg->maxY, msg->minX, msg->maxY);
    HIDD_BM_DrawLine(obj, gc, msg->minX, msg->maxY - addY, msg->minX, msg->minY + addY);

    ReturnVoid("BitMap::DrawRect");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_FillRect

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawRect *msg);

        VOID HIDD_BM_FillRect (OOP_Object *obj, OOP_Object *gc, WORD minX, WORD minY,
                               WORD maxX, WORD maxY);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

        Draws a solid rectangle. minX and minY specifies the upper
        left corner of the rectangle. minY and maxY specifies the lower
        right corner of the rectangle.
        The function does not clip the rectangle against the drawing area.

    INPUTS
        obj        - A bitmap to draw on
        gc         - A GC object to use for drawing
        minX, minY - upper left corner of the rectangle in pixels
        maxX, maxY - lower right corner of the rectangle in pixels

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO
        Fill with pattern

*****************************************************************************************/

VOID BM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *obj,
			       struct pHidd_BitMap_DrawRect *msg)
{
    OOP_Object *gc = msg->gc;
    WORD    	y = msg->minY;
    UWORD   	linepat;
    
    EnterFunc(bug("BitMap::FillRect()"));

    linepat = GC_LINEPAT(gc);
    GC_LINEPAT(gc) = ~0;
    
    for(; y <= msg->maxY; y++)
    {
        HIDD_BM_DrawLine(obj, gc, msg->minX, y, msg->maxX, y);
    }

    GC_LINEPAT(gc) = linepat;
    
    ReturnVoid("BitMap::FillRect");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_DrawEllipse

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawEllipse *msg);

        VOID HIDD_BM_DrawEllipse (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y,
                                  WORD rx, WORD ry);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Draws a hollow ellipse from the center point (x,y) with the radii
        rx and ry in the specified bitmap.
        The function does not clip the ellipse against the drawing area.

    INPUTS
        obj   - A bitmap to draw on
        gc    - A GC object to use for drawing
        x,y   - Coordinates of center point in pixels
        rx,ry - ry and ry radius in pixels

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS
        Because of overflow the current code do not work with big
        values of rx and ry.

    SEE ALSO

    INTERNALS

    TODO
        Bugfix

*****************************************************************************************/

/* TODO: Try to opimize clipping here */

VOID BM__Hidd_BitMap__DrawEllipse(OOP_Class *cl, OOP_Object *obj,
				  struct pHidd_BitMap_DrawEllipse *msg)
{
    OOP_Object *gc = msg->gc;
    WORD    	x = msg->rx, y = 0;     /* ellipse points */

    /* intermediate terms to speed up loop */
    LONG    	t1 = msg->rx * msg->rx, t2 = t1 << 1, t3 = t2 << 1;
    LONG    	t4 = msg->ry * msg->ry, t5 = t4 << 1, t6 = t5 << 1;
    LONG    	t7 = msg->rx * t5, t8 = t7 << 1, t9 = 0L;
    LONG    	d1 = t2 - t7 + (t4 >> 1);    /* error terms */
    LONG    	d2 = (t1 >> 1) - t8 + t5;

    BOOL    	doclip = GC_DOCLIP(gc);


    EnterFunc(bug("BitMap::DrawEllipse()"));

    while (d2 < 0)                  /* til slope = -1 */
    {
        /* draw 4 points using symmetry */

	if  (doclip)
	{

	    if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y + y))
		HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y + y);

	    if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y - y))
		HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y - y);

	    if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y + y))
		HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y + y);

	    if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y - y))
		HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y - y);

	}
	else
	{
	    HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y + y);
            HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y - y);
            HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y + y);
            HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y - y);
        }

        y++;            /* always move up here */
        t9 = t9 + t3;
        if (d1 < 0)     /* move straight up */
        {
            d1 = d1 + t9 + t2;
            d2 = d2 + t9;
        }
        else            /* move up and left */
        {
            x--;
            t8 = t8 - t6;
            d1 = d1 + t9 + t2 - t8;
            d2 = d2 + t9 + t5 - t8;
        }
    }

    do                              /* rest of top right quadrant */
    {
        /* draw 4 points using symmetry */
    #if 1
	if  (doclip)
	{

	    if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y + y))
		HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y + y);

	    if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y - y))
		HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y - y);

	    if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y + y))
		HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y + y);

	    if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y - y))
		HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y - y);

	}
	else
	{

	    HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y + y);
            HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y - y);
            HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y + y);
            HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y - y);
        }
    #else

        HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y + y);
        HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y - y);
        HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y + y);
        HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y - y);
    #endif
        x--;            /* always move left here */
        t8 = t8 - t6;
        if (d2 < 0)     /* move up and left */
        {
            y++;
            t9 = t9 + t3;
            d2 = d2 + t9 + t5 - t8;
        }
        else            /* move straight left */
        {
            d2 = d2 + t5 - t8;
        }

    } while (x >= 0);


    ReturnVoid("BitMap::DrawEllipse");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_FillEllipse

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawEllipse *msg);

        VOID HIDD_BM_FillEllipse (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y,
                                  WORD ry, WORD rx);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Draws a solid ellipse from the center point (x,y) with the radii
        rx and ry in the specified bitmap.
        The function does not clip the ellipse against the drawing area.

    INPUTS
        obj   - A bitmap to draw on
        gc    - A GC object to use for drawing
        x,y   - Coordinates of center point in pixels
        rx,ry - ry and ry radius in pixels

    RESULT
	None.

    NOTES
	This method is not used by the system and considered reserved.

    EXAMPLE

        Because of overflow the current code do not work with big
        values of rx and ry.

    SEE ALSO

    INTERNALS

    TODO
        Bugfix

*****************************************************************************************/

VOID BM__Hidd_BitMap__FillEllipse(OOP_Class *cl, OOP_Object *obj,
				  struct pHidd_BitMap_DrawEllipse *msg)
{
    OOP_Object *gc = msg->gc;
    WORD    	x = msg->rx, y = 0;     /* ellipse points */

    /* intermediate terms to speed up loop */
    LONG    	t1 = msg->rx * msg->rx, t2 = t1 << 1, t3 = t2 << 1;
    LONG    	t4 = msg->ry * msg->ry, t5 = t4 << 1, t6 = t5 << 1;
    LONG    	t7 = msg->rx * t5, t8 = t7 << 1, t9 = 0L;
    LONG    	d1 = t2 - t7 + (t4 >> 1);    /* error terms */
    LONG    	d2 = (t1 >> 1) - t8 + t5;

    EnterFunc(bug("BitMap::FillEllipse()"));

    while (d2 < 0)                  /* til slope = -1 */
    {
        /* draw 4 points using symmetry */
        HIDD_BM_DrawLine(obj, gc, msg->x - x, msg->y + y, msg->x + x, msg->y + y);
        HIDD_BM_DrawLine(obj, gc, msg->x - x, msg->y - y, msg->x + x, msg->y - y);

        y++;            /* always move up here */
        t9 = t9 + t3;
        if (d1 < 0)     /* move straight up */
        {
            d1 = d1 + t9 + t2;
            d2 = d2 + t9;
        }
        else            /* move up and left */
        {
            x--;
            t8 = t8 - t6;
            d1 = d1 + t9 + t2 - t8;
            d2 = d2 + t9 + t5 - t8;
        }
    }

    do                              /* rest of top right quadrant */
    {
        /* draw 4 points using symmetry */
        HIDD_BM_DrawLine(obj, gc, msg->x - x, msg->y + y, msg->x + x, msg->y + y);
        HIDD_BM_DrawLine(obj, gc, msg->x - x, msg->y - y, msg->x + x, msg->y - y);

        x--;            /* always move left here */
        t8 = t8 - t6;
        if (d2 < 0)     /* move up and left */
        {
            y++;
            t9 = t9 + t3;
            d2 = d2 + t9 + t5 - t8;
        }
        else            /* move straight left */
        {
            d2 = d2 + t5 - t8;
        }

    } while (x >= 0);

    ReturnVoid("BitMap::FillEllipse");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_DrawPolygon

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawPolygon *msg);

        VOID HIDD_BM_DrawPolygon (OOP_Object *obj, OOP_Object *gc, UWORD n, WORD *coords);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Draws a hollow polygon from the list of coordinates in coords[].
        The function does not clip the polygon against the drawing area.

    INPUTS
        obj    - A bitmap to draw on
        gc     - A GC object to use for drawing
        n      - number of coordinate pairs
        coords - array of n (x, y) coordinates in pixels

    RESULT
	None.

    NOTES
    	This method is not used by the system and considered reserved.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/

VOID BM__Hidd_BitMap__DrawPolygon(OOP_Class *cl, OOP_Object *obj,
				  struct pHidd_BitMap_DrawPolygon *msg)
{
    OOP_Object *gc = msg->gc;
    WORD    	i;

    EnterFunc(bug("BitMap::DrawPolygon()"));

    for(i = 2; i < (2 * msg->n); i = i + 2)
    {
        HIDD_BM_DrawLine(obj, gc, msg->coords[i - 2], msg->coords[i - 1],
                              msg->coords[i], msg->coords[i + 1]);
    }

    ReturnVoid("BitMap::DrawPolygon");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_FillPolygon

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawPolygon *msg);

        VOID HIDD_BM_FillPolygon (OOP_Object *obj, OOP_Object *gc, UWORD n, WORD *coords);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
	This method was initially designed for drawing solid polygons, however it was never
	used and implemented. At the moment it is considered reserved, its synopsis and
	semantics may change in future.

    INPUTS
        obj    - A bitmap to draw on
        gc     - A GC object to use for drawing
        n      - number of coordinate pairs
        coords - array of n (x, y) coordinates in pixels

    RESULT
	None

    NOTES

    EXAMPLE

    BUGS
	Never used and implemented

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/

VOID BM__Hidd_BitMap__FillPolygon(OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_DrawPolygon *msg)
{

    EnterFunc(bug("BitMap::FillPolygon()"));

    D(bug("Sorry, FillPolygon() not implemented yet in bitmap baseclass\n"));

    ReturnVoid("BitMap::FillPolygon");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_DrawText

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawText *msg);

        VOID HIDD_BM_DrawText (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y,
                               STRPTR text, UWORD length);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Draws the first length characters of text at (x, y).
        The function does not clip the text against the drawing area.

    INPUTS
        obj    - A bitmap to draw on
        gc     - A GC object to use for drawing and font specification
        x, y   - Position to start drawing in pixels. The x
                 coordinate is relativ to the left side of the
                 first character.
                 The y coordinate is relative to the baseline of the font.
        text   - Pointer to a Latin 1 string
        length - Number of characters to draw

    RESULT
	None.

    NOTES
	At the moment text drawing is processed entirely by graphics.library
	using BltTemplate(), which in turn uses moHodd_BitMap_PutTemplate.
	This method is considered obsolete.

    EXAMPLE

    BUGS
	The default implementation in the base class does not process styles,
	color and alpha-blended fonts.

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/

VOID BM__Hidd_BitMap__DrawText(OOP_Class *cl, OOP_Object *obj,
			       struct pHidd_BitMap_DrawText *msg)
{
    OOP_Object      *gc = msg->gc;
    struct TextFont *font  = GC_FONT(gc);
    UBYTE   	    *charPatternPtr = font->tf_CharData;
    UWORD   	    modulo          = font->tf_Modulo;
    ULONG   	    charLog;
    UBYTE   	    ch;              /* current character to print               */
    WORD    	    fx, fx2, fy, fw; /* position and length of character in the  */
                            /* character bitmap                         */
    WORD    	    xMem = msg->x;   /* position in bitmap                       */
    WORD    	    yMem = msg->y - font->tf_Baseline;
    WORD    	    x, y, i;


    EnterFunc(bug("BitMap::DrawText()"));

    for(i = 0; i < msg->length; i++)
    {
        ch = msg->text[i];

        if((ch < font->tf_LoChar) || (ch > font->tf_HiChar))
        {
            ch = font->tf_HiChar - font->tf_LoChar + 1;
        }
        else
        {
            ch = ch - font->tf_LoChar;
        }

        if(font->tf_Flags & FPF_PROPORTIONAL)
        {
            xMem = xMem + ((UWORD *) font->tf_CharKern)[ch];
        }

        charLog = ((ULONG *) font->tf_CharLoc)[ch];
        fx2 = charLog >> 16;   /* x position of character pattern in character bitmap */
        fw  = (UWORD) charLog; /* width of character pattern in character bitmap */

        y = yMem;

        for(fy = 0; fy < font->tf_YSize; fy ++)
        {
            x = xMem;

            for(fx = fx2; fx < fw + fx2; fx++)
            {
                if(*(charPatternPtr + fx / 8 + fy * modulo) & (128 >> (fx % 8)))
                {
                    HIDD_BM_DrawPixel(obj, msg->gc, x, y);
                }
                x++;
            }

            y++;
        }

        if(font->tf_Flags & FPF_PROPORTIONAL)
        {
            xMem = xMem + ((UWORD *) font->tf_CharSpace)[ch];
        }
        else
        {
            xMem = xMem + font->tf_XSize;
        }
    }

    ReturnVoid("BitMap::DrawText");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_FillText

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawText *msg);

        VOID HIDD_BM_FillText (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y,
                               STRPTR text, UWORD length);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
	Historically this method was designed to draw a text with background.
	It was never implemented.
	
	Currently this method is considered reserved. Its synopsis and semantics
	may change in future.

    INPUTS
        obj    - A bitmap to draw on
        gc     - A GC object to use for drawing
        x, y   - Position to start drawing in pixels. The x
                 coordinate is relative to the left side of the
                 first character.
                 The y coordinate is relative to the baseline of the font.
        text   - Pointer to a Latin 1 string
        length - Number of characters to draw

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/

VOID BM__Hidd_BitMap__FillText(OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_DrawText *msg)
{

    EnterFunc(bug("BitMap::FillText()\n"));

    D(bug("Sorry, FillText() not implemented yet in bitmap baseclass\n"));

    ReturnVoid("BitMap::DrawFillText");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_FillSpan

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_DrawText *msg);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Reserved, never implemented method. The definition will change in future.

    INPUTS

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/

VOID BM__Hidd_BitMap__FillSpan(OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_DrawText *msg)
{

    EnterFunc(bug("BitMap::FillSpan()\n"));

    D(bug("Sorry, not implemented yet\n"));

    ReturnVoid("BitMap::FillSpan");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_Clear

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_Clear *msg);

        VOID HIDD_BM_Clear (OOP_Object *obj, OOP_Object *gc);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
        Sets all pixels of the drawing area to the background color.

    INPUTS
        obj - A bitmap to clear.
        gc  - A GC object, specifies background color value

    RESULT

    NOTES
	This method is not used by the system and considered reserved. However it can
	be useful for display driver's own needs.

    EXAMPLE

    BUGS
	Default implementation in the base class sets all pixels to zero color instead of
	the background color from GC

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/

VOID BM__Hidd_BitMap__Clear(OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_Clear *msg)
{
    WORD  x, y;
    IPTR width, height;

    EnterFunc(bug("BitMap::Clear()\n"));

    OOP_GetAttr(obj, aHidd_BitMap_Width, &width);
    OOP_GetAttr(obj, aHidd_BitMap_Height, &height);

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            HIDD_BM_PutPixel(obj, x, y, 0);
        }
    }

    ReturnVoid("BitMap::Clear");
}

/****************************************************************************************/

static LONG inline getpixfmtbpp(OOP_Class *cl, OOP_Object *o, HIDDT_StdPixFmt stdpf)
{
    OOP_Object *pf;
    struct HIDDBitMapData *data;
    SIPTR bpp = -1;

    data = OOP_INST_DATA(cl, o);

    switch (stdpf)
    {
    	case vHidd_StdPixFmt_Native:
	    OOP_GetAttr(data->prot.pixfmt, aHidd_PixFmt_BytesPerPixel, &bpp);
	    break;

	case vHidd_StdPixFmt_Native32:
	    bpp = sizeof (HIDDT_Pixel);
	    break;

	default:
	    pf = HIDD_Gfx_GetPixFmt(data->gfxhidd, stdpf);

	    if (NULL == pf)
	    {
		D(bug("!!! INVALID PIXFMT IN BitMap::PutImage(): %d !!!\n", stdpf));
	    }
	    else
	    {
		OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bpp);
	    }
	    break;
    }

    return bpp;
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_GetImage

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_GetImage *msg);

        VOID HIDD_BM_GetImage (OOP_Object *obj, UBYTE *pixels, ULONG modulo, WORD x, WORD y,
                               WORD width, WORD height, HIDDT_StdPixFmt pixFmt);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj    -
        pixels -
        modulo -
        x, y   -
        width  -
        height -
        pixFmt -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o,
			       struct pHidd_BitMap_GetImage *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    APTR		    ppixarray = &pixarray;
    LONG    	    	    bpp;
    struct HIDDBitMapData   *data;

    data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("BitMap::GetImage(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));


    bpp = getpixfmtbpp(cl, o, msg->pixFmt);
    if (-1 == bpp)
    {
	D(bug("!!! INVALID PIXFMT IN BitMap::PutImage(): %d !!!\n", msg->pixFmt));
	return;
    }


    switch(msg->pixFmt)
    {
    	case vHidd_StdPixFmt_Native:
	case vHidd_StdPixFmt_Native32:
	    for (y = 0; y < msg->height; y ++)
	    {
    		for (x = 0; x < msg->width; x ++)
    		{
		    register HIDDT_Pixel pix;

		    pix = HIDD_BM_GetPixel(o, x + msg->x , y + msg->y);

		    switch (bpp)
		    {
	    		case 1:
			    *pixarray++ = pix;
			    break;

			case 2:
			    *((UWORD *)pixarray) = pix;
			    pixarray += 2;
			    break;

			case 3:
			#if AROS_BIG_ENDIAN
			    pixarray[0] = (pix >> 16) & 0xFF;
			    pixarray[1] = (pix >> 8) & 0xFF;
			    pixarray[2] =  pix & 0xFF;
			#else
			    pixarray[0] =  pix & 0xFF;
			    pixarray[1] = (pix >> 8) & 0xFF;
			    pixarray[2] = (pix >> 16) & 0xFF;
			#endif
			    pixarray += 3;
			    break;

			case 4:
			    *(ULONG *)pixarray = pix;
			    pixarray += 4;
			    break;
		    }

		}

		pixarray += (msg->modulo - msg->width * bpp);
	    }

	    break;

	default:
	    {
		OOP_Object *dstpf;
	    	APTR 	    buf, srcPixels;

		dstpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);

		buf = srcPixels = AllocVec(msg->width * sizeof(HIDDT_Pixel), MEMF_PUBLIC);
		if (buf)
		{
		    for(y = 0; y < msg->height; y++)
		    {
		    	HIDD_BM_GetImage(o,
					 buf,
					 0,
					 msg->x,
					 msg->y + y,
					 msg->width,
					 1,
					 vHidd_StdPixFmt_Native);

		    	HIDD_BM_ConvertPixels(o,
			    	    	      &srcPixels,
					      (HIDDT_PixelFormat *)data->prot.pixfmt,
					      0,
			    	    	      (APTR *)ppixarray,
					      (HIDDT_PixelFormat *)dstpf,
			    	    	      msg->modulo,
					      msg->width,
					      1,
					      NULL);

		    }
		    FreeVec(buf);
		}
	    }
	    break;

    } /* switch(msg->pixFmt) */

    ReturnVoid("BitMap::GetImage");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_PutImage

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_PutImage *msg);

        VOID HIDD_BM_PutImage (OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo,
                               WORD x, WORD y, WORD width, WORD height, HIDDT_StdPixFmt pixFmt);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj    -
        gc     -
        pixels -
        modulo -
        x, y   -
        width  -
        height -
        pixFmt -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o,
			       struct pHidd_BitMap_PutImage *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    APTR		    ppixarray = &pixarray;
    ULONG   	    	    old_fg;
    LONG    	    	    bpp;
    struct HIDDBitMapData   *data;
    OOP_Object	    	    *gc = msg->gc;

    data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("BitMap::PutImage(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));

    if (msg->width <= 0 || msg->height <= 0)
	return;

    bpp = getpixfmtbpp(cl, o, msg->pixFmt);
    if (-1 == bpp)
    {
	D(bug("!!! INVALID PIXFMT IN BitMap::PutImage(): %d !!!\n", msg->pixFmt));
	return;
    }

    switch(msg->pixFmt)
    {
    	case vHidd_StdPixFmt_Native:
	case vHidd_StdPixFmt_Native32:

	    /* Preserve old fg pen */
	    old_fg = GC_FG(gc);

	    for (y = 0; y < msg->height; y ++)
	    {
    		for (x = 0; x < msg->width; x ++)
    		{
		    register HIDDT_Pixel pix = 0;

		    switch (bpp)
		    {
	    		case 1:
			    pix = *((UBYTE *)pixarray);
			    pixarray ++;
			    break;

			case 2:
			    pix = *((UWORD *)pixarray);
			    pixarray += 2;
			    break;

			case 3:
			#if AROS_BIG_ENDIAN
			    pix = ((UBYTE *)pixarray)[0] << 16;
			    pix |= ((UBYTE *)pixarray)[1] << 8;
			    pix |= ((UBYTE *)pixarray)[2];
			#else
			    pix = ((UBYTE *)pixarray)[2] << 16;
			    pix |= ((UBYTE *)pixarray)[1] << 8;
			    pix |= ((UBYTE *)pixarray)[0];
			#endif
			    pixarray += 3;
			    break;

			case 4:
			    pix = *((ULONG *)pixarray); pixarray += 4;
			    break;

		    }

		    GC_FG(gc) = pix;

		    HIDD_BM_DrawPixel(o, gc, x + msg->x , y + msg->y);
		}
		pixarray += (msg->modulo - msg->width * bpp);
	    }

	    GC_FG(gc) = old_fg;
	    break;

	default:
	    {
		OOP_Object *srcpf;
	    	APTR 	    buf, destPixels;

		srcpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);

		buf = destPixels = AllocVec(msg->width * sizeof(HIDDT_Pixel), MEMF_PUBLIC);
		if (buf)
		{
		    for(y = 0; y < msg->height; y++)
		    {
		    	HIDD_BM_ConvertPixels(o,
			    	    	      (APTR *)ppixarray,
					      (HIDDT_PixelFormat *)srcpf,
			    	    	      msg->modulo,
					      &destPixels,
					      (HIDDT_PixelFormat *)data->prot.pixfmt,
					      0,
					      msg->width,
					      1,
					      NULL);

		    	HIDD_BM_PutImage(o,
			    	    	 msg->gc,
					 buf,
					 0,
					 msg->x,
					 msg->y + y,
					 msg->width,
					 1,
					 vHidd_StdPixFmt_Native);
		    }
		    FreeVec(buf);
		}
	    }
	    break;

    } /* switch(msg->pixFmt) */

    ReturnVoid("BitMap::PutImage");
}

/****************************************************************************************/

int static inline 
__attribute__((always_inline, const)) do_alpha(int a, int v) 
{
    int tmp  = (a*v);
    return ((tmp<<8) + tmp + 32768)>>16;
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_PutAlphaImage

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_PutAlphaImage *msg);

        VOID HIDD_BM_PutAlphaImage (OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo,
                                    WORD x, WORD y, WORD width, WORD height);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj    -
        gc     -
        pixels -
        modulo -
        x, y   -
        width  -
        height -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__PutAlphaImage(OOP_Class *cl, OOP_Object *o,
				    struct pHidd_BitMap_PutAlphaImage *msg)
{
    WORD    	    	    x, y;
    ULONG   	    	    *pixarray = (ULONG *)msg->pixels;
    ULONG    	    	    *buf, *xbuf;
    struct HIDDBitMapData   *data;
    PIXBUF_DECLARE_VARS
    
    data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("BitMap::PutAlphaImage(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));

    if (msg->width <= 0 || msg->height <= 0)
	return;

    PIXBUF_ALLOC(buf, msg->width * sizeof(ULONG), msg->height);

    if (buf)
    {
    	HIDDT_DrawMode old_drmd = GC_DRMD(msg->gc);	
	GC_DRMD(msg->gc) = vHidd_GC_DrawMode_Copy;
	
	xbuf = buf;
	
    	for(y = msg->y; y < msg->y + msg->height; y++)
	{
	    if (PIXBUF_TIME_TO_START_PROCESS)
	    {
	    	WORD height = PIXBUF_LINES_TO_PROCESS;

		HIDD_BM_GetImage(o,
	    	    		 (UBYTE *)buf,
				 msg->width * sizeof(ULONG),
				 msg->x,
				 y,
				 msg->width,
				 height,
				 vHidd_StdPixFmt_ARGB32);
    	    }
	    			     
	    for(x = 0; x < msg->width; x++)
	    {
	    	ULONG	    destpix;
		ULONG	    srcpix;
		LONG 	    src_red, src_green, src_blue, src_alpha;
		LONG	    dst_red, dst_green, dst_blue;
		
    	    	srcpix = *pixarray++;
    	    #if AROS_BIG_ENDIAN		
		src_red   = (srcpix & 0x00FF0000) >> 16;
		src_green = (srcpix & 0x0000FF00) >> 8;
		src_blue  = (srcpix & 0x000000FF);
		src_alpha = (srcpix & 0xFF000000) >> 24;
	    #else
		src_red   = (srcpix & 0x0000FF00) >> 8;
		src_green = (srcpix & 0x00FF0000) >> 16;
		src_blue  = (srcpix & 0xFF000000) >> 24;
		src_alpha = (srcpix & 0x000000FF);
	    #endif
		
		destpix = xbuf[x];
    	    #if AROS_BIG_ENDIAN		
		dst_red   = (destpix & 0x00FF0000) >> 16;
		dst_green = (destpix & 0x0000FF00) >> 8;
		dst_blue  = (destpix & 0x000000FF);
	    #else
		dst_red   = (destpix & 0x0000FF00) >> 8;
		dst_green = (destpix & 0x00FF0000) >> 16;
		dst_blue  = (destpix & 0xFF000000) >> 24;
	    #endif
		
		dst_red   += do_alpha(src_alpha, src_red - dst_red);
		dst_green += do_alpha(src_alpha, src_green - dst_green);
		dst_blue  += do_alpha(src_alpha, src_blue - dst_blue);

    	    #if AROS_BIG_ENDIAN
	    	destpix &= 0xFF000000;
		destpix |= (dst_red << 16) + (dst_green << 8) + (dst_blue);
    	    #else
	    	destpix &= 0x000000FF;
		destpix |= (dst_blue << 24) + (dst_green << 16) + (dst_red << 8);
	    #endif
	    		
		xbuf[x] = destpix;
		
	    } /* for(x = 0; x < msg->width; x++) */
    	
	    if (PIXBUF_TIME_TO_END_PROCESS)
	    {
	    	LONG height = PIXBUF_LINES_TO_PROCESS;
		
		HIDD_BM_PutImage(o,
	    	    		 msg->gc,
				 (UBYTE *)buf,
				 msg->width * sizeof(ULONG),
				 msg->x,
				 y - height + 1,
				 msg->width,
				 height,
				 vHidd_StdPixFmt_ARGB32);
	    	xbuf = buf;
	    }
	    else
	    {
	    	xbuf += msg->width;
	    }
	    
	    pixarray = (ULONG *)((UBYTE *)pixarray + msg->modulo - msg->width * 4);
	    
	    PIXBUF_NEXT_LINE;
	    

	} /* for(y = msg->y; y < msg->y + msg->height; y++) */

	GC_DRMD(msg->gc) = old_drmd;
	
    	PIXBUF_FREE(buf);
		
    } /* if (buf) */
    else
    {
    	for(y = msg->y; y < msg->y + msg->height; y++)
	{
	    for(x = msg->x; x < msg->x + msg->width; x++)
	    {
	    	HIDDT_Pixel destpix;
		HIDDT_Color col;
		ULONG	    srcpix;
		LONG 	    src_red, src_green, src_blue, src_alpha;
		LONG	    dst_red, dst_green, dst_blue;
		
	    	destpix = HIDD_BM_GetPixel(o, x, y);
		HIDD_BM_UnmapPixel(o, destpix, &col);
		
    	    	srcpix = *pixarray++;
    	    #if AROS_BIG_ENDIAN		
		src_red   = (srcpix & 0x00FF0000) >> 16;
		src_green = (srcpix & 0x0000FF00) >> 8;
		src_blue  = (srcpix & 0x000000FF);
		src_alpha = (srcpix & 0xFF000000) >> 24;
	    #else
		src_red   = (srcpix & 0x0000FF00) >> 8;
		src_green = (srcpix & 0x00FF0000) >> 16;
		src_blue  = (srcpix & 0xFF000000) >> 24;
		src_alpha = (srcpix & 0x000000FF);
	    #endif
		
		dst_red   = col.red   >> 8;
		dst_green = col.green >> 8;
		dst_blue  = col.blue  >> 8;
		
		dst_red   += do_alpha(src_alpha, src_red - dst_red);
		dst_green += do_alpha(src_alpha, src_green - dst_green);
		dst_blue  += do_alpha(src_alpha, src_blue - dst_blue);
		
		col.red   = dst_red << 8;
		col.green = dst_green << 8;
		col.blue  = dst_blue << 8;
		
		HIDD_BM_PutPixel(o, x, y, HIDD_BM_MapColor(o, &col));
		
	    } /* for(x = msg->x; x < msg->x + msg->width; x++) */
	    
	    pixarray = (ULONG *)((UBYTE *)pixarray + msg->modulo - msg->width * 4);
	    
	} /* for(y = msg->y; y < msg->y + msg->height; y++) */
	
    }

    ReturnVoid("BitMap::PutAlphaImage");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_PutTemplate

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_PutTemplate *msg);

        VOID HIDD_BM_PutTemplate (OOP_Object *obj, OOP_Object *gc, UBYTE *template, ULONG modulo,
                                  WORD srcx, WORD x, WORD y, WORD width, WORD height, BOOL inverttemplate);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj            -
        gc             -
        template       -
        modulo         -
        srcx           -
        x, y           -
        width          -
        height         -
        inverttemplate -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o,
				  struct pHidd_BitMap_PutTemplate *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *bitarray;
    HIDDT_Pixel	    	    *buf, *xbuf, bitmask;
    OOP_Object	    	    *gc = msg->gc;
    struct HIDDBitMapData   *data;
    WORD    	    	     type = 0;
    PIXBUF_DECLARE_VARS
    
    data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("BitMap::PutTemplate(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));

    if (msg->width <= 0 || msg->height <= 0)
	return;

    if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
    {
    	type = 0;
    }
    else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
    {
    	type = 2;
    }
    else
    {
    	type = 4;
    }
    
    if (msg->inverttemplate) type++;
    
    bitarray = msg->template + ((msg->srcx / 16) * 2);
    bitmask = 0x8000 >> (msg->srcx & 0xF);
    
    PIXBUF_ALLOC(buf, msg->width * sizeof(HIDDT_Pixel), msg->height);
    
    if (buf)
    {
    	HIDDT_DrawMode   old_drmd = GC_DRMD(msg->gc);
    	HIDDT_Pixel	     	 fg = GC_FG(msg->gc);
	HIDDT_Pixel	    	 bg = GC_BG(msg->gc);
	
	GC_DRMD(msg->gc) = vHidd_GC_DrawMode_Copy;

    	xbuf = buf;
	
    	for(y = msg->y; y < msg->y + msg->height; y++)
	{
	    ULONG  mask = bitmask;
	    UWORD *array = (UWORD *)bitarray;
	    UWORD  bitword = AROS_BE2WORD(*array);
	    	    
	    if ((type < 4) && PIXBUF_TIME_TO_START_PROCESS)
	    {
	    	WORD height = PIXBUF_LINES_TO_PROCESS;
		
		HIDD_BM_GetImage(o,
	    	    		 (UBYTE *)buf,
				 msg->width * sizeof(HIDDT_Pixel),
				 msg->x,
				 y,
				 msg->width,
				 height,
				 vHidd_StdPixFmt_Native32);
	    }
	    
	    
	    switch(type)
	    {
	    	case 0:	/* JAM1 */	    
		    for(x = 0; x < msg->width; x++)
		    {
    	    	    	if (bitword & mask) xbuf[x] = fg;
    	    	    	
			mask >>= 1;
			if (!mask)
			{
			    mask = 0x8000;
			    array++;
			    bitword = AROS_BE2WORD(*array);
			}
			
		    } /* for(x = 0; x < msg->width; x++) */
		    break;

	    	case 1:	/* JAM1 | INVERSVID */	    
		    for(x = 0; x < msg->width; x++)
		    {
    	    	    	if (!(bitword & mask)) xbuf[x] = fg;
    	    	    	
			mask >>= 1;
			if (!mask)
			{
			    mask = 0x8000;
			    array++;
			    bitword = AROS_BE2WORD(*array);
			}

		    } /* for(x = 0; x < msg->width; x++) */
		    break;

    	    	case 2: /* COMPLEMENT */
		    for(x = 0; x < msg->width; x++)
		    {
    	    	    	if (bitword & mask) xbuf[x] = ~xbuf[x];
    	    	    	
			mask >>= 1;
			if (!mask)
			{
			    mask = 0x8000;
			    array++;
			    bitword = AROS_BE2WORD(*array);
			}
		    } /* for(x = 0; x < msg->width; x++) */
		    break;

		case 3: /* COMPLEMENT | INVERSVID*/
		    for(x = 0; x < msg->width; x++)
		    {
    	    	    	if (!(bitword & mask)) xbuf[x] = ~xbuf[x];
    	    	    	
			mask >>= 1;
			if (!mask)
			{
			    mask = 0x8000;
			    array++;
			    bitword = AROS_BE2WORD(*array);
			}
		    		    } /* for(x = 0; x < msg->width; x++) */
		    break;
		    
	    	case 4:	/* JAM2 */	    
		    for(x = 0; x < msg->width; x++)
		    {
     	    	    	xbuf[x] = (bitword & mask) ? fg : bg;
			
			mask >>= 1;
			if (!mask)
			{
			    mask = 0x8000;
			    array++;
			    bitword = AROS_BE2WORD(*array);
			}
			
		    } /* for(x = 0; x < msg->width; x++) */
		    break;

	    	case 5:	/* JAM2 | INVERSVID */	    
		    for(x = 0; x < msg->width; x++)
		    {
     	    	    	xbuf[x] = (bitword & mask) ? bg : fg;
			
			mask >>= 1;
			if (!mask)
			{
			    mask = 0x8000;
			    array++;
			    bitword = AROS_BE2WORD(*array);
			}
		    } /* for(x = 0; x < msg->width; x++) */
		    break;
    	
	    } /* switch(type) */
	    
	    if (PIXBUF_TIME_TO_END_PROCESS)
	    {
	    	LONG height = PIXBUF_LINES_TO_PROCESS;

    	    	//kprintf("  PutImage at %d  height %d  __height %d  __bufy %d  __worky %d\n",
    	    	//  	  y - height + 1 - msg->y, height, __height, __bufy, __worky);

		HIDD_BM_PutImage(o,
	    	    		 msg->gc,
				 (UBYTE *)buf,
				 msg->width * sizeof(HIDDT_Pixel),
				 msg->x,
				 y - height + 1,
				 msg->width,
				 height,
				 vHidd_StdPixFmt_Native32);
		xbuf = buf;		
	    }
	    else
	    {
	    	xbuf += msg->width;
	    }
			     
	    bitarray += msg->modulo;

    	    PIXBUF_NEXT_LINE;
	    
	} /* for(y = msg->y; y < msg->y + msg->height; y++) */
	
	GC_DRMD(msg->gc) = old_drmd;
	
    	PIXBUF_FREE(buf);
	
    } /* if (buf) */
    else
    {

    }

    ReturnVoid("BitMap::PutTemplate");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_PutAlphaTemplate

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_PutAlphaTemplate *msg);

        VOID HIDD_BM_PutAlphaTemplate (OOP_Object *obj, OOP_Object *gc, UBYTE *alpha, ULONG modulo,
                                       WORD x, WORD y, WORD width, WORD height, BOOL invertalpha);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj         -
        gc          -
        alpha       -
        modulo      -
        x, y        -
        width       -
        height      -
        invertalpha -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__PutAlphaTemplate(OOP_Class *cl, OOP_Object *o,
				       struct pHidd_BitMap_PutAlphaTemplate *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *pixarray = msg->alpha;
    ULONG    	    	    *buf, *xbuf;
    OOP_Object	    	    *gc = msg->gc;
    struct HIDDBitMapData   *data;
    HIDDT_Color     	     color;
    LONG 	    	     a_red, a_green, a_blue;
    LONG	    	     b_red = 0;
    LONG		     b_green = 0;
    LONG		     b_blue = 0;
    WORD    	    	     type = 0;
    PIXBUF_DECLARE_VARS
    
    data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("BitMap::PutAlphaTemplate(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));

    if (msg->width <= 0 || msg->height <= 0)
	return;

    HIDD_BM_UnmapPixel(o, GC_FG(gc), &color);

    a_red   = color.red >> 8;
    a_green = color.green >> 8;
    a_blue  = color.blue >> 8;

    if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
    {
    	type = 0;
    }
    else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
    {
    	type = 2;
    }
    else
    {
    	type = 4;

    	HIDD_BM_UnmapPixel(o, GC_BG(gc), &color);
    	b_red   = color.red >> 8;
    	b_green = color.green >> 8;
    	b_blue  = color.blue >> 8;
    }
    
    if (msg->invertalpha) type++;
    
    PIXBUF_ALLOC(buf, msg->width * sizeof(ULONG), msg->height);
    
    if (buf)
    {
    	HIDDT_DrawMode old_drmd = GC_DRMD(msg->gc);	
	GC_DRMD(msg->gc) = vHidd_GC_DrawMode_Copy;

    	xbuf = buf;
	
    	for(y = msg->y; y < msg->y + msg->height; y++)
	{
	    if ((type < 4) && PIXBUF_TIME_TO_START_PROCESS)
	    {
	    	WORD height = PIXBUF_LINES_TO_PROCESS;
		
		HIDD_BM_GetImage(o,
	    	    		 (UBYTE *)buf,
				 msg->width * sizeof(ULONG),
				 msg->x,
				 y,
				 msg->width,
				 height,
				 vHidd_StdPixFmt_ARGB32);
	    }
	    
	    switch(type)
	    {
	    	case 0:	/* JAM1 */	    
		    for(x = 0; x < msg->width; x++)
		    {
	    		ULONG	    destpix;
			LONG	    dst_red, dst_green, dst_blue, alpha;

    	    		alpha = *pixarray++;

    	    	    	destpix = xbuf[x];
			
    		    #if AROS_BIG_ENDIAN		
			dst_red   = (destpix & 0x00FF0000) >> 16;
			dst_green = (destpix & 0x0000FF00) >> 8;
			dst_blue  = (destpix & 0x000000FF);
		    #else
			dst_red   = (destpix & 0x0000FF00) >> 8;
			dst_green = (destpix & 0x00FF0000) >> 16;
			dst_blue  = (destpix & 0xFF000000) >> 24;
		    #endif
			
			dst_red   += do_alpha(alpha, a_red - dst_red);
			dst_green += do_alpha(alpha, a_green - dst_green);
			dst_blue  += do_alpha(alpha, a_blue - dst_blue);

    		    #if AROS_BIG_ENDIAN
			destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
    		    #else
			destpix = (dst_blue << 24) + (dst_green << 16) + (dst_red << 8);
		    #endif

			xbuf[x] = destpix;

		    } /* for(x = 0; x < msg->width; x++) */
		    break;

	    	case 1:	/* JAM1 | INVERSVID */	    
		    for(x = 0; x < msg->width; x++)
		    {
	    		ULONG	    destpix;
			LONG	    dst_red, dst_green, dst_blue, alpha;

    	    		alpha = (*pixarray++) ^ 255;

    	    	    	destpix = xbuf[x];
			
    		    #if AROS_BIG_ENDIAN		
			dst_red   = (destpix & 0x00FF0000) >> 16;
			dst_green = (destpix & 0x0000FF00) >> 8;
			dst_blue  = (destpix & 0x000000FF);
		    #else
			dst_red   = (destpix & 0x0000FF00) >> 8;
			dst_green = (destpix & 0x00FF0000) >> 16;
			dst_blue  = (destpix & 0xFF000000) >> 24;
		    #endif

			dst_red   += do_alpha(alpha, a_red - dst_red);
			dst_green += do_alpha(alpha, a_green - dst_green);
			dst_blue  += do_alpha(alpha, a_blue - dst_blue);

    		    #if AROS_BIG_ENDIAN
			destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
    		    #else
			destpix = (dst_blue << 24) + (dst_green << 16) + (dst_red << 8);
		    #endif

			xbuf[x] = destpix;

		    } /* for(x = 0; x < msg->width; x++) */
		    break;

    	    	case 2: /* COMPLEMENT */
		    for(x = 0; x < msg->width; x++)
		    {
	    		ULONG	    destpix;
    	    	    	UBYTE	    alpha;

    	    		alpha = *pixarray++;

    	    	    	destpix = xbuf[x];
    	    	    	if (alpha >= 0x80) destpix = ~destpix;
			xbuf[x] = destpix;

		    } /* for(x = 0; x < msg->width; x++) */
		    break;

		case 3: /* COMPLEMENT | INVERSVID*/
		    for(x = 0; x < msg->width; x++)
		    {
	    		ULONG	    destpix;
    	    	    	UBYTE	    alpha;

    	    		alpha = *pixarray++;

    	    	    	destpix = xbuf[x];
    	    	    	if (alpha < 0x80) destpix = ~destpix;
			xbuf[x] = destpix;

		    } /* for(x = 0; x < msg->width; x++) */
		    break;
		    
	    	case 4:	/* JAM2 */	    
		    for(x = 0; x < msg->width; x++)
		    {
	    		ULONG	    destpix;
			LONG	    dst_red, dst_green, dst_blue, alpha;

    	    		alpha = *pixarray++;

			dst_red   = b_red   + ((a_red   - b_red)   * alpha) / 256;
			dst_green = b_green + ((a_green - b_green) * alpha) / 256;
			dst_blue  = b_blue  + ((a_blue  - b_blue)  * alpha) / 256;

    		    #if AROS_BIG_ENDIAN
			destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
    		    #else
			destpix = (dst_blue << 24) + (dst_green << 16) + (dst_red << 8);
		    #endif

			xbuf[x] = destpix;

		    } /* for(x = 0; x < msg->width; x++) */
		    break;

	    	case 5:	/* JAM2 | INVERSVID */	    
		    for(x = 0; x < msg->width; x++)
		    {
	    		ULONG	    destpix;
			LONG	    dst_red, dst_green, dst_blue, alpha;

    	    		alpha = (*pixarray++) ^ 255;

			dst_red   = b_red   + ((a_red   - b_red)   * alpha) / 256;
			dst_green = b_green + ((a_green - b_green) * alpha) / 256;
			dst_blue  = b_blue  + ((a_blue  - b_blue)  * alpha) / 256;

    		    #if AROS_BIG_ENDIAN
			destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
    		    #else
			destpix = (dst_blue << 24) + (dst_green << 16) + (dst_red << 8);
		    #endif

			xbuf[x] = destpix;

		    } /* for(x = 0; x < msg->width; x++) */
		    break;
    	
	    } /* switch(type) */
	    
	    if (PIXBUF_TIME_TO_END_PROCESS)
	    {
	    	LONG height = PIXBUF_LINES_TO_PROCESS;
		
		HIDD_BM_PutImage(o,
	    	    		 msg->gc,
				 (UBYTE *)buf,
				 msg->width * sizeof(ULONG),
				 msg->x,
				 y - height + 1,
				 msg->width,
				 height,
				 vHidd_StdPixFmt_ARGB32);
				 
		xbuf = buf;
	    }
	    else
	    {
	    	xbuf += msg->width;
	    }
	    
	    pixarray += msg->modulo - msg->width;

    	    PIXBUF_NEXT_LINE

	} /* for(y = msg->y; y < msg->y + msg->height; y++) */
	
	GC_DRMD(msg->gc) = old_drmd;
	
    	PIXBUF_FREE(buf);
	
    } /* if (buf) */
    else
    {

    }

    ReturnVoid("BitMap::PutAlphaTemplate");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_PutPattern

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *o, struct pHidd_BitMap_PutPattern *msg);

        VOID HIDD_BM_PutPattern(OOP_Object *obj, OOP_Object *gc, UBYTE *pattern,
                                WORD patternsrcx, WORD patternsrcy, WORD patternheight, WORD patterndepth,
                                HIDDT_PixelLUT *patternlut, BOOL invertpattern, UBYTE *mask,
                                ULONG maskmodulo, WORD masksrcx, WORD x, WORD y,
                                WORD width, WORD height);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj           - A bitmap to draw on
        gc            - A GC object to use for drawing
        pattern       -
        patternsrcx   -
        patternsrcy   -
        patternheight -
        patterndepth  -
        patternlut    -
        invertpattern -
        mask          -
        maskmodulo    -
        masksrcx      -
        x, y          -
        width         -
        height        -

    RESULT
	None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__PutPattern(OOP_Class *cl, OOP_Object *o,
				 struct pHidd_BitMap_PutPattern *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *patarray;
    UBYTE   	    	    *maskarray = 0;
    ULONG		     maskmask = 0;
    ULONG    	    	    *buf, *xbuf, patmask;
    OOP_Object	    	    *gc = msg->gc;
    struct HIDDBitMapData   *data;
    WORD    	    	     type = 0;
    PIXBUF_DECLARE_VARS
    
    data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("BitMap::PutPattern(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));

    if (msg->width <= 0 || msg->height <= 0)
	return;

    if (msg->patterndepth > 1)
    {
    	type = 6;
    }
    else
    {
	if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
	{
    	    type = 0;
	}
	else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
	{
    	    type = 2;
	}
	else
	{
    	    type = 4;
	}

	if (msg->invertpattern) type++;
    }
    
    patarray = msg->pattern;
    patmask = 0x8000 >> (msg->patternsrcx & 0xF);
    
    if ((maskarray = msg->mask))
    {
    	maskarray += (msg->masksrcx / 16) * 2;
	maskmask = 0x8000 >> (msg->masksrcx & 0xF);
    }
    
    PIXBUF_ALLOC(buf, msg->width * sizeof(ULONG), msg->height);
    
    if (buf)
    {
    	HIDDT_DrawMode   old_drmd = GC_DRMD(msg->gc);
    	ULONG	     	 fg = GC_FG(msg->gc);
	ULONG	    	 bg = GC_BG(msg->gc);
	
	GC_DRMD(msg->gc) = vHidd_GC_DrawMode_Copy;

    	xbuf = buf;
	
    	for(y = msg->y; y < msg->y + msg->height; y++)
	{
	    ULONG  pmask = patmask;
	    ULONG  mmask = maskmask;
	    UWORD *parray = ((UWORD *)patarray) + ((y + msg->patternsrcy - msg->y) % msg->patternheight);
	    UWORD  patword = AROS_BE2WORD(*parray);
	    UWORD *marray = NULL;
	    UWORD  maskword = 0;
	    
	    if (maskarray)
	    {
	    	marray = (UWORD *)maskarray;
		maskword = AROS_BE2WORD(*marray);
	    }
	    
	    if (((type < 4) || maskarray) && PIXBUF_TIME_TO_START_PROCESS)
	    {
	    	WORD height = PIXBUF_LINES_TO_PROCESS;
		
    	    	//kprintf("  GetImage at %d  height %d\n", y - msg->y, height);
		
		HIDD_BM_GetImage(o,
	    	    		 (UBYTE *)buf,
				 msg->width * sizeof(ULONG),
				 msg->x,
				 y,
				 msg->width,
				 height,
				 vHidd_StdPixFmt_Native32);
	    }
	    
	    
	    switch(type)
	    {
	    	case 0:	/* JAM1 */	    
		    for(x = 0; x < msg->width; x++)
		    {
		    	if (!maskarray || (maskword & mmask))
			{
    	    	    	    if (patword & pmask) xbuf[x] = fg;
			}
    	    	    	
			if (maskarray)
			{
			    mmask >>= 1;
			    if (!mmask)
			    {
			    	mmask = 0x8000;
			    	marray++;
			    	maskword = AROS_BE2WORD(*marray);
			    }
			}
			
			pmask >>= 1;
			if (!pmask) pmask = 0x8000;
			
		    } /* for(x = 0; x < msg->width; x++) */
		    break;

	    	case 1:	/* JAM1 | INVERSVID */	    
		    for(x = 0; x < msg->width; x++)
		    {
		    	if (!maskarray || (maskword & mmask))
			{
    	    	    	    if (!(patword & pmask)) xbuf[x] = fg;
			}
    	    	    	
			if (maskarray)
			{
			    mmask >>= 1;
			    if (!mmask)
			    {
			    	mmask = 0x8000;
			    	marray++;
			    	maskword = AROS_BE2WORD(*marray);
			    }
			}
			
			pmask >>= 1;
			if (!pmask) pmask = 0x8000;
			
		    } /* for(x = 0; x < msg->width; x++) */
		    break;

    	    	case 2: /* COMPLEMENT */
		    for(x = 0; x < msg->width; x++)
		    {
		    	if (!maskarray || (maskword & mmask))
			{
    	    	    	    if (patword & pmask) xbuf[x] = ~xbuf[x];
			}
    	    	    	
			if (maskarray)
			{
			    mmask >>= 1;
			    if (!mmask)
			    {
			    	mmask = 0x8000;
			    	marray++;
			    	maskword = AROS_BE2WORD(*marray);
			    }
			}
			
			pmask >>= 1;
			if (!pmask) pmask = 0x8000;
			
		    } /* for(x = 0; x < msg->width; x++) */
		    break;

		case 3: /* COMPLEMENT | INVERSVID*/
		    for(x = 0; x < msg->width; x++)
		    {
		    	if (!maskarray || (maskword & mmask))
			{
    	    	    	    if (!(patword & pmask)) xbuf[x] = ~xbuf[x];
			}
    	    	    	
			if (maskarray)
			{
			    mmask >>= 1;
			    if (!mmask)
			    {
			    	mmask = 0x8000;
			    	marray++;
			    	maskword = AROS_BE2WORD(*marray);
			    }
			}
			
			pmask >>= 1;
			if (!pmask) pmask = 0x8000;
			
		    } /* for(x = 0; x < msg->width; x++) */
		    break;
		    
	    	case 4:	/* JAM2 */	    
		    for(x = 0; x < msg->width; x++)
		    {
		    	if (!maskarray || (maskword & mmask))
			{
    	    	    	    xbuf[x] = (patword & pmask) ? fg : bg;
			}
    	    	    	
			if (maskarray)
			{
			    mmask >>= 1;
			    if (!mmask)
			    {
			    	mmask = 0x8000;
			    	marray++;
			    	maskword = AROS_BE2WORD(*marray);
			    }
			}
			
			pmask >>= 1;
			if (!pmask) pmask = 0x8000;
			
		    } /* for(x = 0; x < msg->width; x++) */
		    break;

	    	case 5:	/* JAM2 | INVERSVID */	    
		    for(x = 0; x < msg->width; x++)
		    {
		    	if (!maskarray || (maskword & mmask))
			{
    	    	    	    xbuf[x] = (patword & pmask) ? bg : fg;
			}
    	    	    	
			if (maskarray)
			{
			    mmask >>= 1;
			    if (!mmask)
			    {
			    	mmask = 0x8000;
			    	marray++;
			    	maskword = AROS_BE2WORD(*marray);
			    }
			}
			
			pmask >>= 1;
			if (!pmask) pmask = 0x8000;
			
		    } /* for(x = 0; x < msg->width; x++) */
		    break;
    	
	    	case 6: /* multi color pattern */
		    for(x = 0; x < msg->width; x++)
		    {
		    	if (!maskarray || (maskword & mmask))
			{
			    WORD plane;
			    ULONG pixel = (patword & pmask) ? 1 : 0;
			    
			    for(plane = 1; plane < msg->patterndepth; plane++)
			    {
			    	UWORD *_parray = parray + plane * msg->patternheight;
				UWORD _patword = AROS_BE2WORD(*_parray);
				
				if (_patword & pmask) pixel |= 1L << plane;				
			    }
			    
			    if (msg->patternlut) pixel = msg->patternlut->pixels[pixel];
			    
    	    	    	    xbuf[x] = pixel;
			}
    	    	    	
			if (maskarray)
			{
			    mmask >>= 1;
			    if (!mmask)
			    {
			    	mmask = 0x8000;
			    	marray++;
			    	maskword = AROS_BE2WORD(*marray);
			    }
			}
			
			pmask >>= 1;
			if (!pmask) pmask = 0x8000;
			
		    } /* for(x = 0; x < msg->width; x++) */
		    break;
		    		    
	    } /* switch(type) */
	    
	    if (PIXBUF_TIME_TO_END_PROCESS)
	    {
	    	LONG height = PIXBUF_LINES_TO_PROCESS;
		
    	    	//kprintf("  PutImage at %d  height %d  __height %d  __bufy %d  __worky %d\n",
    	    	//  	  y - height + 1 - msg->y, height, __height, __bufy, __worky);

		HIDD_BM_PutImage(o,
	    	    		 msg->gc,
				 (UBYTE *)buf,
				 msg->width * sizeof(ULONG),
				 msg->x,
				 y - height + 1,
				 msg->width,
				 height,
				 vHidd_StdPixFmt_Native32);
		xbuf = buf;		
	    }
	    else
	    {
	    	xbuf += msg->width;
	    }
			     
	    if (maskarray) maskarray += msg->maskmodulo;

    	    PIXBUF_NEXT_LINE;
	    
	} /* for(y = msg->y; y < msg->y + msg->height; y++) */
	
	GC_DRMD(msg->gc) = old_drmd;
	
    	PIXBUF_FREE(buf);
	
    } /* if (buf) */
    else
    {

    }

    ReturnVoid("BitMap::PutPattern");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_PutImageLUT

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg);

        VOID HIDD_BM_PutImageLUT (OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo,
                                  WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj    -
        gc     -
        pixels -
        modulo -
        x, y   -
        width  -
        height -
        pixlut -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o,
				  struct pHidd_BitMap_PutImageLUT *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    HIDDT_PixelLUT  	    *pixlut = msg->pixlut;
    HIDDT_Pixel     	    *lut = pixlut ? pixlut->pixels : NULL;
    HIDDT_Pixel     	    *linebuf;
    struct HIDDBitMapData   *data;
    OOP_Object      	    *gc = msg->gc;

    data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("BitMap::PutImageLUT(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));

    if (msg->width <= 0 || msg->height <= 0)
	return;

    linebuf = AllocVec(msg->width * sizeof(HIDDT_Pixel), MEMF_PUBLIC);

    for(y = 0; y < msg->height; y++)
    {
    	if (linebuf)
	{
    	    if (lut)
	    {
    		for(x = 0; x < msg->width; x++)
		{
    	    	    linebuf[x] = lut[pixarray[x]];
		}
	    }
	    else
	    {
    		for(x = 0; x < msg->width; x++)
		{
    	    	    linebuf[x] = pixarray[x];
		}
	    }
	    pixarray += msg->modulo;

	    HIDD_BM_PutImage(o,
	    	    	     msg->gc,
			     (UBYTE *)linebuf,
			     0,
			     msg->x,
			     msg->y + y,
			     msg->width,
			     1,
			     vHidd_StdPixFmt_Native32);

	} /* if (linebuf) */
	else
	{
	    ULONG old_fg;

	    /* Preserve old fg pen */
	    old_fg = GC_FG(gc);

    	    if (lut)
	    {
    		for(x = 0; x < msg->width; x++)
		{
		    GC_FG(gc) = lut[pixarray[x]];
    	    	    HIDD_BM_DrawPixel(o, gc, msg->x + x, msg->y + y);
		}
	    }
	    else
	    {
    		for(x = 0; x < msg->width; x++)
		{
		    GC_FG(gc) = pixarray[x];
    	    	    HIDD_BM_DrawPixel(o, gc, msg->x + x, msg->y + y);
		}
	    }
	    GC_FG(gc) = old_fg;

	    pixarray += msg->modulo;

	} /* if (linebuf) else ... */

    } /* for(y = 0; y < msg->height; y++) */

    if (linebuf) FreeVec(linebuf);

    ReturnVoid("BitMap::PutImageLUT");
}
/*****************************************************************************************

    NAME
        moHidd_BitMap_PutTranspImageLUT

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_PutTranspImageLUT *msg);

        VOID HIDD_BM_PutTranspImageLUT (OOP_Object *obj, OOP_Object *gc, UBYTE *pixels,
                                        ULONG modulo, WORD x, WORD y, WORD width, WORD height,
                                        HIDDT_PixelLUT *pixlut, UBYTE transparent);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
	Copy an array of 8-bit LUT pixels to the bitmap at the specified position making
	one of colors transparent.

	Pixels are converted to bitmap's native format using either user-supplied LUT (if
	given) or bitmap's own colormap.

	Draw mode of the supplied GC is ignored, the operation is always bulk copy.

    INPUTS
        obj         - A bitmap to draw image on
        gc          - A GC used for drawing
        pixels      - A pointer to source pixel array
        modulo      - Total number of bytes per line in the source array
        x, y        - Top-left corner of the destination rectangle
        width       - Width of the image to draw
        height      - Height of the image to draw
        pixlut      - An optional pointer to a LUT to use. NULL means using bitmap's
		      own colormap (if available)
        transparent - Value of pixels in the source array which will be made
		      transparent

    RESULT
	None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__PutTranspImageLUT(OOP_Class *cl, OOP_Object *o,
					struct pHidd_BitMap_PutTranspImageLUT *msg)
{
    WORD    	    	     x, y;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    UBYTE   	    	     transparent = msg->transparent;
    HIDDT_PixelLUT  	    *pixlut = msg->pixlut;
    HIDDT_Pixel     	    *lut = pixlut ? pixlut->pixels : NULL;
    HIDDT_Pixel	    	    *buf, *xbuf;
    struct HIDDBitMapData   *data;
    PIXBUF_DECLARE_VARS
    
    data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("BitMap::PutTranspImageLUT(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));

    if (msg->width <= 0 || msg->height <= 0)
	return;

    PIXBUF_ALLOC(buf, msg->width * sizeof(HIDDT_Pixel), msg->height);

    if (buf)
    {
    	HIDDT_DrawMode old_drmd = GC_DRMD(msg->gc);	

	GC_DRMD(msg->gc) = vHidd_GC_DrawMode_Copy;
	xbuf = buf;

    	for(y = msg->y; y < msg->y + msg->height; y++)
	{
	    if (PIXBUF_TIME_TO_START_PROCESS)
	    {
	    	WORD height = PIXBUF_LINES_TO_PROCESS;

		HIDD_BM_GetImage(o,
	    	    		 (UBYTE *)buf,
				 msg->width * sizeof(HIDDT_Pixel),
				 msg->x,
				 y,
				 msg->width,
				 height,
				 vHidd_StdPixFmt_Native32);
    	    }

	    if (lut)
	    {
	    	for(x = 0; x < msg->width; x++)
	    	{
		    UBYTE pix = *pixarray++;
		    
		    if (pix != transparent)
		    {
		    	xbuf[x] = lut[pix];
		    }
		    
    		} /* for(x = 0; x < msg->width; x++) */
	    }
	    else
	    {
	    	for(x = 0; x < msg->width; x++)
	    	{
		    UBYTE pix = *pixarray++;
		    
		    if (pix != transparent)
		    {
			if (data->colmap)
			    pix = HIDD_CM_GetPixel(data->colmap, pix);
		    	xbuf[x] = pix;
		    }

    		} /* for(x = 0; x < msg->width; x++) */
	    }
		
    	
	    if (PIXBUF_TIME_TO_END_PROCESS)
	    {
	    	LONG height = PIXBUF_LINES_TO_PROCESS;
		
		HIDD_BM_PutImage(o,
	    	    		 msg->gc,
				 (UBYTE *)buf,
				 msg->width * sizeof(HIDDT_Pixel),
				 msg->x,
				 y - height + 1,
				 msg->width,
				 height,
				 vHidd_StdPixFmt_Native32);
	    	xbuf = buf;
	    }
	    else
	    {
	    	xbuf += msg->width;
	    }
	    
	    pixarray = ((UBYTE *)pixarray + msg->modulo - msg->width);
	    
	    PIXBUF_NEXT_LINE;
	    

	} /* for(y = msg->y; y < msg->y + msg->height; y++) */

	GC_DRMD(msg->gc) = old_drmd;
	
    	PIXBUF_FREE(buf);
		
    } /* if (buf) */

    ReturnVoid("BitMap::PutTranspImageLUT");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_GetImageLUT

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_GetImageLUT *msg);

        VOID HIDD_BM_GetImageLUT (OOP_Object *obj, UBYTE *pixels, ULONG modulo, WORD x, WORD y,
                                  WORD width, WORD height, HIDDT_PixelLUT *pixlut);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj    -
        pixels -
        modulo -
        x, y   -
        width  -
        height -
        pixlut -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__GetImageLUT(OOP_Class *cl, OOP_Object *o,
				  struct pHidd_BitMap_GetImageLUT *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    HIDDT_PixelLUT  	    *pixlut = msg->pixlut;
    HIDDT_Pixel     	    *lut = pixlut ? pixlut->pixels : NULL;
    HIDDT_Pixel     	    *linebuf;
    struct HIDDBitMapData   *data;

    data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("BitMap::GetImageLUT(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));

    linebuf = AllocVec(msg->width * sizeof(HIDDT_Pixel), MEMF_PUBLIC);

    for(y = 0; y < msg->height; y++)
    {
    	if (linebuf)
	{
	    HIDD_BM_GetImage(o,
	    	    	    (UBYTE *)linebuf,
			    0,
			    msg->x,
			    msg->y + y,
			    msg->width,
			    1,
			    vHidd_StdPixFmt_Native32);
    	    if (lut)
	    {
	    	/* FIXME: This is wrong, but HIDD_BM_GetImageLUT on hi/truecolor screens does not really make sense anyway */
    		for(x = 0; x < msg->width; x++)
		{
    	    	    pixarray[x] = (UBYTE)linebuf[x];
		}
	    }
	    else
	    {
    		for(x = 0; x < msg->width; x++)
		{
    	    	    pixarray[x] = (UBYTE)linebuf[x];
		}
	    }
	    pixarray += msg->modulo;

	} /* if (linebuf) */
	else
	{
    	    if (lut)
	    {
	    	/* FIXME: This is wrong, but HIDD_BM_GetImageLUT on hi/truecolor screens does not really make sense anyway */
    		for(x = 0; x < msg->width; x++)
		{
		    pixarray[x] = (UBYTE)HIDD_BM_GetPixel(o, msg->x + x, msg->y + y);
		}
	    }
	    else
	    {
    		for(x = 0; x < msg->width; x++)
		{
		    pixarray[x] = (UBYTE)HIDD_BM_GetPixel(o, msg->x + x, msg->y + y);
		}
	    }

	    pixarray += msg->modulo;

	} /* if (linebuf) else ... */

    } /* for(y = 0; y < msg->height; y++) */

    if (linebuf) FreeVec(linebuf);

    ReturnVoid("BitMap::GetImageLUT");
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_BlitColorExpansion

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_BlitColorExpansion *msg);

        VOID HIDD_BM_BlitColorExpansion (OOP_Object *obj, OOP_Object *gc, OOP_Object *srcBitMap,
                                         WORD srcX, WORD srcY, WORD destX, WORD destY,
                                         UWORD width, UWORD height);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
	Perform a color expansion of the mask in srcBitMap according to foreground and background
	colors and expansion mode specified by the supplied GC. Pixels which are set to zero in
	the mask bitmap will be either painted by background (in opaque mode) or left without
	change (in transparent mode). Pixels which are set to nonzero in the mask will be painted
	by foreground color.
	
	The result of expansion is blitted onto the destination bitmap accorging to GC's draw mode.

    INPUTS
        obj           - A bitmap to draw on
        gc            - A GC object to use for drawing
        srcBitMap     - A bitmap object containing mask image.
        srcX, srcY    - A top-left coordinate of the used rectangle in the source bitmap
        destX, destY  - A top-left coordinate of the destination rectangle to draw in
        width, height - A size of the rectangle to blit

    RESULT
	None.

    NOTES
	This method was previously used by graphics.library/Text() to draw fonts with no
	styles specified. Currently graphics.library always uses BltTemplate() and this
	method is considered obsolete.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__BlitColorExpansion(OOP_Class *cl, OOP_Object *o,
					 struct pHidd_BitMap_BlitColorExpansion *msg)
{
    ULONG   cemd;
    ULONG   fg, bg;
    LONG    x, y;

    OOP_Object *gc = msg->gc;

    EnterFunc(bug("BitMap::BlitColorExpansion(srcBM=%p, srcX=%d, srcY=%d, destX=%d, destY=%d, width=%d, height=%d)\n",
    		msg->srcBitMap, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));

    cemd = GC_COLEXP(gc);
    fg	 = GC_FG(gc);
    bg	 = GC_BG(gc);

/* bug("------------- Blit_ColExp: (%d, %d, %d, %d, %d, %d) cemd=%d, fg=%p, bg=%p -------------\n"
    , msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height
    , cemd, fg, bg);
*/
    for (y = 0; y < msg->height; y ++)
    {
    	for (x = 0; x < msg->width; x ++)
	{
	    ULONG is_set;

	    /* Pixel value is either 0 or 1 for BM of depth 1 */
	    is_set = HIDD_BM_GetPixel(msg->srcBitMap, x + msg->srcX, y + msg->srcY);

/*
if (is_set)
    bug("#");
else
    bug(" ");
*/
	    if (is_set)
	    {
		HIDD_BM_DrawPixel(o, gc, x + msg->destX, y + msg->destY);
	    }
	    else
	    {
		if (cemd & vHidd_GC_ColExp_Opaque)
		{
		    /* Write bixel with BG pen */
		    GC_FG(gc) = bg;
		    HIDD_BM_DrawPixel(o, gc, x + msg->destX, y + msg->destY);
		    /* Reset to FG pen */
		    GC_FG(gc) = fg;
		}

	    } /* if () */

	} /* for (each x) */
/*
    bug("\n");
*/
    } /* for ( each y ) */

    ReturnVoid("BitMap::BlitColorExpansion");

}

/*****************************************************************************************

    NAME
        moHidd_BitMap_BytesPerLine

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_BytesPerLine *msg);

        ULONG HIDD_BM_BytesPerLine(OOP_Object *obj, HIDDT_StdPixFmt pixFmt, ULONG width);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj    -
        pixFmt -
        width  -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

ULONG BM__Hidd_BitMap__BytesPerLine(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BytesPerLine *msg)
{
     ULONG bpl;

     switch (msg->pixFmt)
     {
         case vHidd_StdPixFmt_Native32:
	     bpl = sizeof (HIDDT_Pixel) * msg->width;
	     break;

	 case vHidd_StdPixFmt_Native:
	 {
	     struct HIDDBitMapData *data;

	     data = OOP_INST_DATA(cl, o);

	     bpl = ((HIDDT_PixelFormat *)data->prot.pixfmt)->bytes_per_pixel * msg->width;
	     break;
	}

	default:
	{
	    OOP_Object      	    *pf;
	    struct HIDDBitMapData   *data;

	    data = OOP_INST_DATA(cl, o);

	    pf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);

	    if (NULL == pf)
	    {
		D(bug("!!! COULD NOT GET STD PIXFMT IN BitMap::BytesPerLine() !!!\n"));
		return 0;
	    }

	    bpl = ((HIDDT_PixelFormat *)pf)->bytes_per_pixel * msg->width;
	    break;
	}
     }

     return bpl;

}

/****************************************************************************************/

/*
   This makes it easier to create a subclass of the graphics hidd.
   It is only allowed to use this method in the p_RootNew method of a
   bitmap subclass.
*/

/****************************************************************************************/

VOID BM__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct HIDDBitMapData   *data = OOP_INST_DATA(cl, obj);
    struct TagItem  	    *tag;
    const struct TagItem    *tstate;
    ULONG   	    	    idx;

/*    EnterFunc(bug("BitMap::Set()\n"));
*/
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_BitMap_Width:
		    data->width = tag->ti_Data;
		    break;

                case aoHidd_BitMap_Height:
		    data->height = tag->ti_Data;
		    break;

                case aoHidd_BitMap_Depth:
		    data->reqdepth = tag->ti_Data;
		    break;

    	    #if 0
                case aoHidd_BitMap_ColorTab:
		    data->colorTab = (APTR)tag->ti_Data;
		    break;
    	    #endif

		default:
		    D(bug("!!! TRYING TO SET NONSETTABLE BITMAP ATTR %d !!!\n", idx));
		    break;

            }
        }
    }

    return;
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_SetColorMap

    SYNOPSIS
        OOP_Object * OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_SetColorMap *msg);

        OOP_Object * HIDD_BM_SetColorMap(OOP_Object *obj, OOP_Object *colorMap);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj      -
        colorMap -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

OOP_Object *BM__Hidd_BitMap__SetColorMap(OOP_Class *cl, OOP_Object *o,
					 struct pHidd_BitMap_SetColorMap *msg)
{
    struct HIDDBitMapData   *data;
    OOP_Object	    	    *old;

    data = OOP_INST_DATA(cl, o);

    old = data->colmap;
    data->colmap = msg->colorMap;

    return old;
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_MapColor

    SYNOPSIS
        HIDDT_Pixel OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_MapColor *msg);

        HIDDT_Pixel HIDD_BM_MapColor(OOP_Object *obj, HIDDT_Color *color);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj   -
        color -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

HIDDT_Pixel BM__Hidd_BitMap__MapColor(OOP_Class *cl, OOP_Object *o,
				      struct pHidd_BitMap_MapColor *msg)
{
    HIDDT_PixelFormat *pf = BM_PIXFMT(o);

    HIDDT_Pixel red	= msg->color->red;
    HIDDT_Pixel green	= msg->color->green;
    HIDDT_Pixel blue	= msg->color->blue;
    HIDDT_Pixel alpha   = msg->color->alpha;
    
    /* This code assumes that sizeof (HIDDT_Pixel is a multimple of sizeof(col->#?)
       which should be true for most (all ?) systems. (I have never heard
       of any system with for example 3 byte types.
    */

    if (IS_TRUECOLOR(pf))
    {
    	if (HIDD_PF_SWAPPIXELBYTES(pf))
	{
	    /* FIXME: BM__Hidd_BitMap__MapColor assuming that SwapPixelBytes flag only set for 2-byte/16-bit pixel formats */

	    HIDDT_Pixel pixel = MAP_RGBA(red, green, blue, alpha, pf);

	    msg->color->pixval = SWAPBYTES_WORD(pixel);
	}
	else
	{
    	    msg->color->pixval = MAP_RGBA(red, green, blue, alpha, pf);
	}
    }
    else
    {
    	struct HIDDBitMapData 	*data = OOP_INST_DATA(cl, o);
	HIDDT_Color 	    	*ctab;

	ctab = ((HIDDT_ColorLUT *)data->colmap)->colors;
	/* Search for the best match in the color table */
	/* FIXME: Implement this */

    }

    return msg->color->pixval;
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_UnmapPixel

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_UnmapPixel *msg);

        VOID HIDD_BM_UnmapPixel(OOP_Object *obj, HIDDT_Pixel pixel, HIDDT_Color *color);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj   -
        pixel -
        color -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__UnmapPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UnmapPixel *msg)
{

    HIDDT_PixelFormat *pf = BM_PIXFMT(o);

    if (IS_TRUECOLOR(pf))
    {
    	HIDDT_Pixel pixel = msg->pixel;

	if (HIDD_PF_SWAPPIXELBYTES(pf))
	{
	    /* FIXME: bitmap_unmappixel assuming that SwapPixelBytes flag only set for 2-byte/16-bit pixel formats */
	    pixel = SWAPBYTES_WORD(pixel);
	}

    	msg->color->red		= RED_COMP	(pixel, pf);
    	msg->color->green	= GREEN_COMP	(pixel, pf);
    	msg->color->blue	= BLUE_COMP	(pixel, pf);
    	msg->color->alpha	= ALPHA_COMP	(pixel, pf);
    }
    else
    {
    	struct HIDDBitMapData 	*data = OOP_INST_DATA(cl, o);
	HIDDT_ColorLUT      	*clut;

	clut = (HIDDT_ColorLUT *)data->colmap;



	/* FIXME: Use CLUT shift and CLUT mask here */
	if (msg->pixel < 0 || msg->pixel >= clut->entries)
	    return;

	*msg->color = clut->colors[msg->pixel];

    }

    /* Unnecesart, but ... */
    msg->color->pixval	= msg->pixel;
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_ObtainDirectAccess

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_ObtainDirectAccess *msg);

        BOOL HIDD_BM_ObtainDirectAccess(OOP_Object *obj, UBYTE **addressReturn,
                                        ULONG *widthReturn, ULONG *heightReturn,
                                        ULONG *bankSizeReturn, ULONG *memSizeReturn);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj            -
        addressReturn  -
        widthReturn    -
        heightReturn   -
        bankSizeReturn -
        memSizeReturn  -

    RESULT
        BOOL

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

BOOL BM__Hidd_BitMap__ObtainDirectAccess(OOP_Class *cl, OOP_Object *o,
					 struct pHidd_BitMap_ObtainDirectAccess *msg)
{
    /* Default implementation of direct access funcs. Just return FALSE */
    return FALSE;
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_ReleaseDirectAccess

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_ReleaseDirectAccess *msg);

        VOID HIDD_BM_ReleaseDirectAccess(OOP_Object *obj);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__ReleaseDirectAccess(OOP_Class *cl, OOP_Object *o,
					  struct pHidd_BitMap_ReleaseDirectAccess *msg)
{
     D(bug("!!! BitMap BaseClasse ReleaseDirectAccess() called !!!\n"));
     D(bug("!!! This should never happen and is probably due to a buggy implementation in the subclass !!!\n"));

     return;
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_BitMapScale

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_BitMapScale * msg);

        VOID HIDD_BM_BitMapScale(OOP_Object *obj, OOP_Object *src, OOP_Object *dest,
                                 struct BitScaleArgs * bsa, OOP_Object *gc);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS
        obj  -
        src  -
        dest -
        bsa  -
        gc   -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__BitMapScale(OOP_Class * cl, OOP_Object *o,
				  struct pHidd_BitMap_BitMapScale * msg)
{
    struct BitScaleArgs *bsa = msg->bsa;
    ULONG *srcbuf, *dstbuf;
    LONG srcline = -1;
    UWORD *linepattern;
    ULONG count;
    UWORD ys = bsa->bsa_SrcY;
    ULONG xs = bsa->bsa_SrcX;
    ULONG dyd = bsa->bsa_DestHeight;
    ULONG dxd = bsa->bsa_DestWidth;
    LONG accuys = dyd;
    LONG accuxs = dxd;
    ULONG dxs = bsa->bsa_SrcWidth;
    ULONG dys = bsa->bsa_SrcHeight;
    LONG accuyd = - (dys >> 1);
    LONG accuxd = - (dxs >> 1);
    ULONG x;

    if ((srcbuf = AllocVec(bsa->bsa_SrcWidth * sizeof(ULONG), 0)) == NULL)
        return;

    if ((dstbuf = AllocVec(bsa->bsa_DestWidth * sizeof(ULONG), 0)) == NULL) {
        FreeVec(srcbuf);
        return;
    }

    if ((linepattern = (UWORD *) AllocVec(bsa->bsa_DestWidth * sizeof(UWORD), 0)) == NULL) {
        FreeVec(dstbuf);
        FreeVec(srcbuf);
        return;
    }

    count = 0;
    while (count < bsa->bsa_DestWidth) {
        accuxd += dxs;
        while (accuxd > accuxs) {
            xs++;
            accuxs += dxd;
        }

        linepattern[count] = xs;

        count++;
    }

    count = bsa->bsa_DestY;
    while (count < bsa->bsa_DestHeight + bsa->bsa_DestY) {
        accuyd += dys;
        while (accuyd > accuys) {
            ys++;
            accuys += dyd;
        }

        if (srcline != ys) {
            HIDD_BM_GetImage(msg->src, (UBYTE *) srcbuf, bsa->bsa_SrcWidth * sizeof(ULONG), bsa->bsa_SrcX, bsa->bsa_SrcY + ys, bsa->bsa_SrcWidth, 1, vHidd_StdPixFmt_Native32);
            srcline = ys;

            for (x = 0; x < bsa->bsa_DestWidth; x++)
                dstbuf[x] = srcbuf[linepattern[x]];
        }

        HIDD_BM_PutImage(msg->dst, msg->gc, (UBYTE *) dstbuf, bsa->bsa_DestWidth * sizeof(ULONG), bsa->bsa_DestX, bsa->bsa_DestY + count, bsa->bsa_DestWidth, 1, vHidd_StdPixFmt_Native32);

        count++;
    }

    FreeVec(linepattern);

    FreeVec(dstbuf);
    FreeVec(srcbuf);
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_SetRGBConversionFunction

    SYNOPSIS
        HIDDT_RGBConversionFunction
        OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_SetRGBConversionFunction *msg);

        HIDDT_RGBConversionFunction
        HIDD_BM_SetRGBConversionFunction(OOP_Object *obj, HIDDT_StdPixFmt srcPixFmt,
                                         HIDDT_StdPixFmt dstPixFmt, 
                                         HIDDT_RGBConversionFunction function);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

HIDDT_RGBConversionFunction BM__Hidd_BitMap__SetRGBConversionFunction(OOP_Class * cl, OOP_Object *o,
				    	            	    	      struct pHidd_BitMap_SetRGBConversionFunction * msg)
{
    HIDDT_RGBConversionFunction old;
    
    if ((msg->srcPixFmt < FIRST_RGB_STDPIXFMT) ||
    	(msg->dstPixFmt < FIRST_RGB_STDPIXFMT) ||
	(msg->srcPixFmt > LAST_RGB_STDPIXFMT) ||
	(msg->dstPixFmt > LAST_RGB_STDPIXFMT))
    {
    	return (HIDDT_RGBConversionFunction)-1;
    }
    else
    {
    	ObtainSemaphore(&CSD(cl)->rgbconvertfuncs_sem);
    	old = CSD(cl)->rgbconvertfuncs[msg->srcPixFmt - FIRST_RGB_STDPIXFMT][msg->dstPixFmt - FIRST_RGB_STDPIXFMT];
	CSD(cl)->rgbconvertfuncs[msg->srcPixFmt - FIRST_RGB_STDPIXFMT][msg->dstPixFmt - FIRST_RGB_STDPIXFMT] = msg->function;
    	ReleaseSemaphore(&CSD(cl)->rgbconvertfuncs_sem);
	
	return old;
    }
}

/*****************************************************************************************

    NAME
        moHidd_BitMap_UpdateRect

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_BitMap_UpdateRect *msg);

        VOID HIDD_BM_UpdateRect(OOP_Object *obj, WORD x, WORD y, WORD width, WORD height);

    LOCATION
        hidd.graphics.bitmap

    FUNCTION
	Update displayed image of the given rectangle.

	Some drivers (like VGA and VESA) may work not with VRAM directly, but with a mirrored
	copy of it. Usually it is done in case if VRAM reading is slow. This method is called
	by the system after it completes any drawing operation, in order to make sure that
	changes made are visible on the actual screen. If your driver uses mirroring, this method
	should copy the given rectangle (at least) from the mirror buffer to the actual VRAM.

	This method is also called after changing currently visible bitmap (after moHidd_Gfx_Show
	method call) in order to allow the mirroring driver to refresh the screen after current bitmap
	changed. Note that moHidd_Gfx_ShowViewPorts is very different and moHidd_BitMap_UpdateRect
	will not be called if it succeeded!

    INPUTS
        obj    - an object whose image to refresh
        x, y   - A top-left edge of the rectangle to refresh
        width  - Width of the rectangle to refresh
        height - Height of the rectangle to refresh

    RESULT
    	None.

    NOTES
    	This method is called also on offscreen bitmaps. You should track visible state of your bitmap
    	and ignore these calls if it's not currently visible on the screen.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID BM__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg) {
    /* baseclass version does nothing */
    return;
}

/****************************************************************************************/

/* private ! */

/****************************************************************************************/

BOOL BM__Hidd_BitMap__SetBitMapTags(OOP_Class *cl, OOP_Object *o,
				    struct pHidd_BitMap_SetBitMapTags *msg)
{
    struct HIDDBitMapData   *data;
    OOP_Object      	    *pf;
    IPTR    	    	    attrs[num_Hidd_BitMap_Attrs];
    DECLARE_ATTRCHECK(bitmap);

    data = OOP_INST_DATA(cl, o);

    if (0 != OOP_ParseAttrs(msg->bitMapTags
    		, attrs, num_Hidd_BitMap_Attrs
		, &ATTRCHECK(bitmap), HiddBitMapAttrBase))
    {
	D(bug("!!! FAILED PARSING IN BitMap::SetBitMapTags !!!\n"));
	return FALSE;
    }

    if (GOT_BM_ATTR(PixFmtTags))
    {
    	/* Allready a pixfmt registered ? */

	pf = HIDD_Gfx_RegisterPixFmt(data->gfxhidd, (struct TagItem *)attrs[AO(PixFmtTags)]);
	if (NULL == pf)
	    return FALSE;

    	if (data->pf_registered)
	     HIDD_Gfx_ReleasePixFmt(data->gfxhidd, data->prot.pixfmt);

	data->prot.pixfmt = pf;
    }


    if (GOT_BM_ATTR(Width))
    	data->width = attrs[AO(Width)];

    if (GOT_BM_ATTR(Height))
    	data->height = attrs[AO(Height)];

    return TRUE;
}

/****************************************************************************************/

BOOL HIDD_BitMap_SetBitMapTags(OOP_Object *o, struct TagItem *bitMapTags)
{
    STATIC_MID;
    struct pHidd_BitMap_SetBitMapTags 	p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_SetBitMapTags);

    p.mID = static_mid;

    p.bitMapTags = bitMapTags;

    return (BOOL)OOP_DoMethod(o, (OOP_Msg)msg);
}

/****************************************************************************************/

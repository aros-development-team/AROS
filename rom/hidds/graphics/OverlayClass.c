/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Overlay hidd class description
    Lang: english
*/

/*
 * This file contains only documentation. Currently no overlay base class is developed.
 * So far there's no need to compile this file.
 */

/*****************************************************************************************

    NAME
	--background--

    LOCATION
	hidd.graphics.overlay

    NOTES
	Objects of overlay class represent hardware video overlays.
	
	Current hardware supports only one video overlay per screen, however in future
	the situation may change.
	
	hidd.graphics.overlay is an interface name. There's no such public ID since
	there's actually no base class for the overlay. The whole implementation is
	hardware-dependant and needs to be done separately for every driver.

	Overlay classes do not need to be public. It's up to display drivers to manage
	them. A moHidd_Gfx_NewOverlay method of graphics driver class is used to create
	overlay objects.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Overlay_SrcWidth

    SYNOPSIS
        [I..], ULONG

    LOCATION
        hidd.graphics.overlay

    FUNCTION
        Specifies source data width in pixels.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Overlay_SrcHeight

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Overlay_SrcHeight

    SYNOPSIS
        [I..], ULONG

    LOCATION
        hidd.graphics.overlay

    FUNCTION
        Specifies source data height in pixels.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Overlay_SrcWidth

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Overlay_SrcFormat

    SYNOPSIS
        [I..], ULONG

    LOCATION
        hidd.graphics.overlay

    FUNCTION
        Specifies source data pixel format. The value should be one of SRCFMT_... constants
	defined in cybergraphx/cgxvideo.h:
	
	  SRCFMT_YUV16   - 16-but YUV
	  SRCFMT_YCbCr16 - 16-bit YCbCr
	  SRCFMT_RGB15PC - R5G5B5, little-endian
	  SRCFMT_RGB16PC - R5G6B5, little-endian

    NOTES
	Not all formats can be supported by all drivers. Use aoHidd_Overlay_Error attribute
	in order to get an explanation why overlay creation fails.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Overlay_Error

    SYNOPSIS
        [I..], ULONG *

    LOCATION
        hidd.graphics.overlay

    FUNCTION
        Specifies a pointer to ULONG location where error code will be written.

	This attribute can be used for overlay creation in order to be able to get an
	information about the actual failure reason.

	Resulting error code can be one of VOERR_... values defined in
	cybergraphx/cgxvideo.h:
	
	  VOERR_OK          - there was no error
	  VOERR_INVSCRMODE  - no (more) hardware overlays are supported on this card
	  VOERR_NOOVLMEMORY - there is not enough VRAM to hold overlay data
	  VOERR_INVSRCFMT   - given source pixel format is not supported by the card
	  VOERR_NOMEMORY    - there is not enough system RAM for internal driver needs

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

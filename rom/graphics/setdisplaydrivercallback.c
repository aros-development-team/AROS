/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set display driver notification callback
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/driver.h>
#include <oop/oop.h>
#include <proto/utility.h>

#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(void, SetDisplayDriverCallback,

/*  SYNOPSIS */
	AROS_LHA(APTR, callback, A0),
	AROS_LHA(APTR, userdata, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 186, Graphics)

/*  FUNCTION
	Specify a display driver notification callback.
	
	The callback function is called using "C" calling convention and its
	declaration should have a form:

	APTR DriverNotify(APTR object, BOOL add, APTR userdata);

	The function will be called upon display driver insertion and removal.
	Upon insertion the parameters will be the following:
	  object   - A pointer to a struct MonitorHandle for the new driver
	  add      - TRUE, indicates driver insertion
	  userdata - User data originally passed to SetDisplayDriverCallback()
	The function should return a pointer to opaque data object which will
	be stored in the display driver handle structure.

	Upon driver removal the parameters mean:
	  object   - A pointer to opaque object returned by the callback when
		     the driver was added.
	  add	   - FALSE, indicates driver removal.
	  userdata - User data originally passed to SetDisplayDriverCallback()
	Callback return value is ignored in removal mode.

    INPUTS
	callback - A pointer to a function to call.
	userdata - User-defined data, will be passed to the callback function

    RESULT
	None.

    NOTES
	This function is private to AROS. Do not use it in any end-user software,
	the specification may change at any moment.

    EXAMPLE

    BUGS

    SEE ALSO
	AddDisplayDriverA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    CDD(GfxBase)->DriverNotify = callback;
    CDD(GfxBase)->notify_data  = userdata;

    AROS_LIBFUNC_EXIT
}

/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, StrtoStamp,

/*  SYNOPSIS */ 
      AROS_LHA(struct DateTime, datetime, A0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 95, Arp)

/*  NAME
 	  StrtoStamp - convert ASCII portion of	DateTime structure to
		  a DateStamp.
 
    FUNCTION
	  Converts a human readable ASCII string into an AmigaDOS
	  DateStamp.

    INPUTS
	  DateTime - a pointer to an initialized DateTime structure.

	  The DateTime structure should	be initialized as follows:

	  dat_Format - a format	byte which specifies the format	of the
		  dat_StrDat.  This can	be any of the following	(note:
		  If value used	is something other than	those below,
		  the default of FORMAT_DOS is used):

		  FORMAT_DOS:	  AmigaDOS format (dd-mmm-yy).

		  FORMAT_INT:	  International	format (yy-mmm-dd).

		  FORMAT_USA:	  American format (mm-dd-yy).

		  FORMAT_CDN:	  Canadian format (dd-mm-yy).

	  dat_Flags - a	flags byte.  The only flag which affects this
		  function is:

		  DTB_FUTURE:	  If set, indicates that strings such
				  as (stored in	dat_StrDate) "Monday"
				  refer	to "next" monday. Otherwise,
				  if clear, strings like "Monday"
				  refer	to "last" monday.

	  dat_StrDate -	pointer	to valid string	representing the date.
		  This can be a	"DTB_SUBST" style string such as
		  "Today" "Tomorrow" "Monday", or it may be a string
		  as specified by the dat_Format byte.	This will be
		  converted to the ds_Days portion of the DateStamp.
		  If this pointer is NULL, DateStamp->ds_Days will not
		  be affected.

	  dat_StrTime -	Pointer	to a buffer which contains the time in
		  the ASCII format hh:mm:ss.  This will	be converted
		  to the ds_Minutes and	ds_Ticks portions of the
		  DateStamp.  If this pointer is NULL, ds_Minutes and
		  ds_Ticks will	be unchanged.

    RESULT
	  error	- a non-zero return indicates that a conversion	could
		  not be performed. A Zero return indicates that the
		  DateTime.dat_Stamp variable contains the converted
		  values.
    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

    return StrToDate(&datetime) ? 0 : -1;

    AROS_LIBFUNC_EXIT
} 

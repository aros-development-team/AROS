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

      AROS_LH1(ULONG, StamptoStr,

/*  SYNOPSIS */ 
      AROS_LHA(struct DateTime, datetime, A0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 94, Arp)

/*  NAME
    StamptoStr - convert DateStamp portion of DateTime structure
		  to string.
  
    FUNCTION
	  StamptoStr converts an AmigaDOS DateStamp to a human
	  readable ASCII string	as requested by	your settings in the
	  DateTime structure.

    INPUTS
	  DateTime - a pointer to an initialized DateTime structure.

	  The DateTime structure should	be initialized as follows:

	  dat_Stamp - a	copy of	the datestamp you wish to convert to
		  ascii.

	  dat_Format - a format	byte which specifies the format	of the
		  dat_StrDate.	This can be any	of the following
		  (note: If value used is something other than those
		  below, the default of	FORMAT_DOS is used):

		  FORMAT_DOS:	  AmigaDOS format (dd-mmm-yy).

		  FORMAT_INT:	  International	format (yy-mmm-dd).

		  FORMAT_USA:	  American format (mm-dd-yy).

		  FORMAT_CDN:	  Canadian format (dd-mm-yy).

	  dat_Flags - a	flags byte.  The only flag which affects this
		  function is:

		  DTB_SUBST:	  If set, a string such	as Today,
				  Monday, etc.,	will be	used instead
				  of the dat_Format specification if
				  possible.

	  dat_StrDay - pointer to a buffer to receive the day of the
		  week string.	(Monday, Tuesday, etc.). If null, this
		  string will not be generated.

	  dat_StrDate -	pointer	to a buffer to receive the date
		  string, in the format	requested by dat_Format,
		  subject to possible modifications by DTB_SUBST.  If
		  null,	this string will not be	generated.

	  dat_StrTime -	pointer	to a buffer to receive the time	of day
		  string. If NULL, this	will not be generated.

    RESULT
	  error	- a non-zero return indicates that the DateStamp was
		  invalid, and could not be converted.	Zero indicates
		  that everything went according to plan.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

    return DateToStr(&datetime) ? 0 : -1;

    AROS_LIBFUNC_EXIT
}

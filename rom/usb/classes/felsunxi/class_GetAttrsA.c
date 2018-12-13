/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

#include "felsunxi_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH3(ULONG, GetAttrsA,

/*  SYNOPSIS */
    	AROS_LHA(ULONG,            type,      D0),
    	AROS_LHA(APTR,             usbstruct, A0),
    	AROS_LHA(struct TagItem *, taglist,   A1),
	
/*  LOCATION */
	LIBBASETYPEPTR, FELSunxiBase, 5, FELSunxi)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag;
    ULONG count = 0;

    switch(type) {
        case UGA_CLASS: {
            while((tag = LibNextTagItem(&taglist)) != NULL) {
                switch (tag->ti_Tag) {
                    case UCCA_Description:
                        *((STRPTR *) tag->ti_Data) = "Support for Allwinner FEL mode";
                        count++;
                    break;
                }
            }
            break;
        }

        case UGA_BINDING: {
            while((tag = LibNextTagItem(&taglist)) != NULL) {
                switch (tag->ti_Tag) {
                    case UCBA_UsingDefaultCfg:
                        //*((IPTR *) tag->ti_Data) = ((struct NepClassHid *) usbstruct)->nch_UsingDefaultCfg;
                        //*((IPTR *) tag->ti_Data) = 0;
                        //count++;
                    break;
                }
            }
            break;
        }

    }

    return count;
    AROS_LIBFUNC_EXIT

} /* GetAttrsA */

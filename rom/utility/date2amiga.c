/*
    $Id$
    $Log$
    Revision 1.4  1997/01/27 00:32:30  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:13  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:35  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/08/31 12:58:12  aros
    Merged in/modified for FreeBSD.

    Desc:
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
#include <proto/utility.h>

        AROS_LH1(ULONG, Date2Amiga,

/*  SYNOPSIS */
        AROS_LHA(struct ClockData *, date, A0),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 21, Utility)

/*  FUNCTION
        Converts the information given in the struct ClockData *date, into
        the number of seconds that have past since the 1st of January 1978.

    INPUTS
        date    -   Contains the information about the time.

    RESULT
        The number of seconds since 1.1.1978

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        utility/Amiga2Date(), utility/CheckData()

    INTERNALS
        Bit of a hack in the leap year handling.

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* This array contains the number of days that have been in the year
       up to the start of the month. Does not take into account leap years.
    */
    static const UWORD dayspermonth[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

    ULONG time;
    UWORD year;

    time = date->sec + (date->min * 60) + (date->hour * 3600);
    time += (date->mday - 1)* 86400;
    time += dayspermonth[date->month-1] * 86400;
    time += (date->year - 1978) * 86400 * 365;

    /* Now comes the hard bit, how do we work out the extra day for a leap
        year. I do it by subtracting lots of four to start with, then
        by considering the remaining few years.

        For every group of four years which we have to subtract, we can
        add one leap year. eg 1982 - 1978= 4, so we add one leap year (1980)

        1989 - 1978 = 11 (two lots of four subtracted), (1980, 1984, 1988).
        However in this case, the 1988 is handled by the code after the
        while loop.

        Is there an easier way perhaps?
    */
    year = date->year - 1978;
    while( year > 3 )
    {
        if(year > 4 )   time += 86400;
        year -= 4;
    }


    if( (year > 2) || ((year == 2) && (date->month > 2)))
        time += 86400;

    return time;

    AROS_LIBFUNC_EXIT
} /* Date2Amiga */

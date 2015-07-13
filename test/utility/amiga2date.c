/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <dos/dos.h>
#include <utility/date.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

struct DateTime dt;
struct ClockData cd;

char s[100];

int main(void)
{
    int retval = RETURN_OK;

    dt.dat_StrDate = "31-dec-2000";
    dt.dat_Format = FORMAT_DOS;
    
    if (StrToDate(&dt))
    {
        dt.dat_StrDate = s;
        if (DateToStr(&dt))
        {
            if (strcmp(s, "31-Dec-00")
                || dt.dat_Stamp.ds_Days != 8400
                || dt.dat_Stamp.ds_Minute != 0
                || dt.dat_Stamp.ds_Tick != 0)
            {
                retval = RETURN_ERROR;
                bug("DateToStr returned: \"%s\" days = %ld min = %ld tick = %ld\n",
                    s,
                    (long)dt.dat_Stamp.ds_Days,
                    (long)dt.dat_Stamp.ds_Minute,
                    (long)dt.dat_Stamp.ds_Tick);
            }

            Amiga2Date(dt.dat_Stamp.ds_Days * 60 * 60 * 24 + 
                       dt.dat_Stamp.ds_Minute * 60 +
                       dt.dat_Stamp.ds_Tick / 50, &cd);

            if (cd.sec != 0
                || cd.min != 0
                || cd.hour != 0
                || cd.mday != 31
                || cd.month != 12
                || cd.year != 2000
                || cd.wday != 0)
            {
                retval = RETURN_ERROR;
                bug("\nAmiga2Date says:\n\n");
                bug("sec   = %d\n", cd.sec);
                bug("min   = %d\n", cd.min);
                bug("hour  = %d\n", cd.hour);
                bug("mday  = %d\n", cd.mday);
                bug("month = %d\n", cd.month);
                bug("year  = %d\n", cd.year);
                bug("wday  = %d\n", cd.wday);
            }

            Amiga2Date((dt.dat_Stamp.ds_Days + 1) * 60 * 60 * 24 + 
                       dt.dat_Stamp.ds_Minute * 60 +
                       dt.dat_Stamp.ds_Tick / 50, &cd);

            if (cd.sec != 0
                || cd.min != 0
                || cd.hour != 0
                || cd.mday != 1
                || cd.month != 1
                || cd.year != 2001
                || cd.wday != 1)
            {
                retval = RETURN_ERROR;
                bug("\nAmiga2Date says (one day later:\n\n");
                bug("sec   = %d\n", cd.sec);
                bug("min   = %d\n", cd.min);
                bug("hour  = %d\n", cd.hour);
                bug("mday  = %d\n", cd.mday);
                bug("month = %d\n", cd.month);
                bug("year  = %d\n", cd.year);
                bug("wday  = %d\n", cd.wday);
            }
        }
        else
        {
            retval = RETURN_ERROR;
            bug("Calling DateToStr failed!\n");
        }
    }
    else
    {
        retval = RETURN_ERROR;
        bug("Calling StrToDate failed!\n");
    }

    return retval;
}

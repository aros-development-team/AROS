/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/datetime.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>

struct DateTime dt;
struct DateStamp ds;
char s[100];

LONG days = 0;

int main(void)
{
    do
    {
    	dt.dat_Stamp.ds_Days = days;
	dt.dat_Format = FORMAT_DOS;
	dt.dat_StrDate = s;
	
	DateToStr(&dt);
	
	dt.dat_Stamp.ds_Days = -1;
	StrToDate(&dt);
	
	//printf("date \"%s\" day = %ld\n", s, days);
	
	if (dt.dat_Stamp.ds_Days != days)
	{
	    printf("Bad results for date \"%s\" (day #%ld). "
	    	   "StrToDate thought it was day #%ld\n", s, (long)days, (long)dt.dat_Stamp.ds_Days);
	}
	
	days++;
	
    } while (!CheckSignal(SIGBREAKF_CTRL_C)
        && (days < 36525)); /* 2078-01-01: same as 1978 in FORMAT_DOS */

    return 0;
}

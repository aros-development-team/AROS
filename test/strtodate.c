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
	    	   "StrToDate thought it was day #%ld\n", s, days, dt.dat_Stamp.ds_Days);
	}
	
	days++;
	
    } while (!CheckSignal(SIGBREAKF_CTRL_C) && (days < 365 * 300)); /* around 300 years */

    return 0;
}

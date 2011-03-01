#include <dos/datetime.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <utility/date.h>
#include <string.h>
#include <stdio.h>

struct UtilityBase *UtilityBase;
struct DateTime dt;
struct DateStamp ds;
struct ClockData cd;
 
char s[100];

LONG days = 0;
ULONG seconds = 0;
ULONG secresult;

int main(void)
{
    if (!(UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 36)))
    {
    	printf("Can't open utility.library!\n");
    }
    else
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
		printf("StrToDate showed bad results for date \"%s\" (day #%ld). "
	    	       "StrToDate thought it was day #%ld\n", s, (long)days, (long)dt.dat_Stamp.ds_Days);
	    }
	    else
	    {
	    	Amiga2Date(seconds, &cd);
		if ((secresult = Date2Amiga(&cd)) != seconds)
		{
		    printf("Date2Amiga gave wrong values for date \"%s\" (day #%ld)"
		           " (secs %ld) -> wrong secs is %ld"
			   " --> clockdate: year = %d month = %d day = %d\n"
			   , s, (long)days, (long)seconds, (long)secresult, (int)cd.year, (int)cd.month, (int)cd.mday);
		}
		
	    }

	    days++;
    	    seconds += 86400;
	    
	} while (days < 365 * 300); /* around 300 years */
	
	CloseLibrary((struct Library *)UtilityBase);
    }

    return 0;
}

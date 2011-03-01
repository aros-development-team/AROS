#include <dos/dos.h>
#include <utility/date.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <stdio.h>

struct UtilityBase *UtilityBase;
struct DateTime dt;
struct ClockData cd;

char s[100];

int main(void)
{
    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 0);
    if (UtilityBase)
    {
	dt.dat_StrDate = "31-dec-2000";
	dt.dat_Format = FORMAT_DOS;
	
	if (StrToDate(&dt))
	{
	    dt.dat_StrDate = s;
	    if (DateToStr(&dt))
	    {
	    	printf("Verified date: \"%s\" days = %ld min = %ld tick = %ld\n",
		    	s,
		    	(long)dt.dat_Stamp.ds_Days,
			(long)dt.dat_Stamp.ds_Minute,
			(long)dt.dat_Stamp.ds_Tick);
	    	
		Amiga2Date(dt.dat_Stamp.ds_Days * 60 * 60 * 24 + 
		    	   dt.dat_Stamp.ds_Minute * 60 +
			   dt.dat_Stamp.ds_Tick / 50, &cd);
			   
		printf("\nAmiga2Date says:\n\n");
		printf("sec   = %d\n", cd.sec);
		printf("min   = %d\n", cd.min);
		printf("hour  = %d\n", cd.hour);
		printf("mday  = %d\n", cd.mday);
		printf("month = %d\n", cd.month);
		printf("year  = %d\n", cd.year);
		printf("wday  = %d\n", cd.wday);

    	    	printf("\n-------- One day later -----------\n\n");
		
		Amiga2Date((dt.dat_Stamp.ds_Days + 1) * 60 * 60 * 24 + 
		    	   dt.dat_Stamp.ds_Minute * 60 +
			   dt.dat_Stamp.ds_Tick / 50, &cd);
			   
		printf("\nAmiga2Date says:\n\n");
		printf("sec   = %d\n", cd.sec);
		printf("min   = %d\n", cd.min);
		printf("hour  = %d\n", cd.hour);
		printf("mday  = %d\n", cd.mday);
		printf("month = %d\n", cd.month);
		printf("year  = %d\n", cd.year);
		printf("wday  = %d\n", cd.wday);

	    }
	    else puts("DateToStr failed!");
	}
	else puts("StrToDate failed!");
	
    	CloseLibrary((struct Library *)UtilityBase);
    }

    return 0;
}

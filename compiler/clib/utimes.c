#include <dos/dos.h>
#include <proto/dos.h>

#include <sys/time.h>

int utimes(const char *file, struct timeval tvp[2])
{
    struct DateStamp ds;
    ULONG t = (ULONG)tvp[1].tv_secs;

    ds.ds_Days   = t / (60*60*24);
    ds.ds_Minute = (t / 60) % (60*24);
    ds.ds_Tick   = (t % 60) * TICKS_PER_SECOND;

    if (SetFileDate(file, &ds))
	return(0);

    return(-1);
}

#include <errno.h>
#include <sys/time.h>

#include <utime.h>

int utime(const char *filename, struct utimbuf *buf)
{
    struct timeval ts[2];
    
    if( buf == NULL )
    {
        time_t tt = time( NULL );

        ts[0].tv_sec = tt;
        ts[1].tv_sec = tt;
    }
    else
    {
        ts[0].tv_sec = buf->actime;
        ts[1].tv_sec = buf->modtime;
    }

    ts[0].tv_usec = 0;
    ts[1].tv_usec = 0;
    
    return utimes(filename, ts);
}

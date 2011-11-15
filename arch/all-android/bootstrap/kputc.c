#include <android/log.h>
#include <stdarg.h>
#include <stdio.h>

#include "bootstrap.h"
#include "hostlib.h"

static int p = 0;

/*
 * On Android additionally to stderr we output the log to Android's
 * own buffer. This is better than nothing.
 * Since Android's debug output is line-oriented, we have to accumulate
 * the text in the buffer.
 */

static void flushLog(void)
{
    buf[p] = 0;
    __android_log_write(ANDROID_LOG_DEBUG, "AROS", buf);
    p = 0;
}    
 
int KPutC(int chr)
{
    int ret;

    ret = fputc(chr, stderr);

    if (chr == '\n')
    {
        fflush(stderr);
        flushLog();
    }
    else
    {
        buf[p++] = chr;

    	if (p == BUFFER_SIZE - 1)
    	    flushLog();
    }

    return ret;
}

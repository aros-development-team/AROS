/*
 * These are often used by timer.device itself, and inlining
 * them saves us from function call overhead.
 */

#define ADDTIME(dest, src)			\
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						\
	(dest)->tv_secs++;			\
	(dest)->tv_micro -= 1000000;		\
    }

static inline LONG CMPTIME(struct timeval *dest, struct timeval *src)
{
    LONG diff;

    if (dest->tv_secs == src->tv_secs)
	diff = src->tv_micro - dest->tv_micro;
    else
	diff = src->tv_secs - dest->tv_secs;

    if (diff < 0)
	return -1;
    else if (diff > 0)
	return 1;
    else
	return 0;
}

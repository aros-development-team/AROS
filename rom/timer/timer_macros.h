/*
 * These are often used by timer.device itself, and inlining
 * them saves us from function call overhead.
 */

/* Add, then normalize */
#define ADDTIME(dest, src)			\
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs  += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						\
	(dest)->tv_secs++;			\
	(dest)->tv_micro -= 1000000;		\
    }

/*
 * Subtraction algorithm:
 * 1. Normalize values
 * 2. Check if wrap around will happen, when subtracting src->tv_micro
 *    from dest->tv_micro. If yes, then normalize, by adding 1 sec to
 *    micros and subtracting 1 sec from secs. Note: this check must be
 *    before subtracting src timeval from dest timeval!
 */
#define SUBTIME(dest, src)			\
    while ((src)->tv_micro > 999999)		\
    {						\
	(src)->tv_secs++;			\
	(src)->tv_micro -= 1000000;		\
    }						\
    while ((dest)->tv_micro > 999999)		\
    {						\
	(dest)->tv_secs++;			\
	(dest)->tv_micro -= 1000000;		\
    }						\
    if ((dest)->tv_micro < (src)->tv_micro)	\
    {						\
	(dest)->tv_micro += 1000000;		\
	(dest)->tv_secs--;			\
    }						\
    (dest)->tv_micro -= (src)->tv_micro;	\
    (dest)->tv_secs  -= (src)->tv_secs;

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

/*
 * Add 'diff' EClock ticks to timeval in 'time'.
 * Fraction of second value is stored in in 'frac'.
 * This macro relies on (CPU-specific) tick2usec() implementation
 */
#define INCTIME(time, frac, diff)		\
    (frac) += diff;				\
    if ((frac) >= TimerBase->tb_eclock_rate)	\
    {						\
        (frac) -= TimerBase->tb_eclock_rate;	\
        (time).tv_secs++;			\
    }						\
    (time).tv_micro = tick2usec(frac);

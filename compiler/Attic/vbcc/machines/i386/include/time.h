#ifndef __TIME_H
#define __TIME_H 1

#ifndef __TIME_T
#define __TIME_T 1
typedef long time_t;
#endif

#ifndef __CLOCK_T
#define __CLOCK_T 1
typedef long clock_t;
#endif

#ifndef __SIZE_T
#define __SIZE_T 1
typedef unsigned long size_t;
#endif

struct tm {
        int     tm_sec;
        int     tm_min;
        int     tm_hour;
        int     tm_mday;
        int     tm_mon;
        int     tm_year;
        int     tm_wday;
        int     tm_yday;
        int     tm_isdst;
/*        long    tm_gmtoff;
        char    *tm_zone;*/
};

time_t time(time_t *);
double difftime(time_t,time_t);
char *ctime(const time_t *);
char *asctime(const struct tm *);
clock_t clock(void);
struct tm *gmtime(const time_t *);
struct tm *localtime(const time_t *);
time_t mktime(struct tm *);
size_t strftime(char *,size_t,const char *,const struct tm *);

#define CLOCKS_PER_SEC 50   /* z.Z. wohl nutzlos    */

#define difftime(a,b) ((double)((a)-(b)))

#endif


#ifndef _SYS_AROSC_H
#define _SYS_AROSC_H

#include <sys/cdefs.h>

struct arosc_userdata
{
    /* stdio.h */
    void *acud_stdin;
    void *acud_stdout;
    void *acud_stderr;

    /* errno.h */
    int acud_errno;

    const unsigned short int *acud_ctype_b;
    const int                *acud_ctype_toupper;
    const int                *acud_ctype_tolower;

    /* Used by time.h functions */
    int        acud_daylight;
    long int   acud_timezone;
    char     **acud_tzname;

    /* Used by multi-byte functions */
    int acud_mb_cur_max;
};

__BEGIN_DECLS

struct arosc_userdata *__get_arosc_userdata(void);

__END_DECLS

#endif /* !_SYS_AROSC_H */

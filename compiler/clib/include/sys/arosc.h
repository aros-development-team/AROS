#ifndef _SYS_AROSC_H
#define _SYS_AROSC_H

#include <sys/cdefs.h>

struct __sFILE;

struct arosc_userdata
{
    /* stdio.h */
    struct __sFILE *acud_stdin;
    struct __sFILE *acud_stdout;
    struct __sFILE *acud_stderr;

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

struct arosc_userdata *__get_arosc_userdata(void) __pure;
int __arosc_nixmain(int (*main)(int argc, char *argv[]), int argc, char *argv[]);

__END_DECLS

enum
{ 
    #undef  SYSTEM_CALL
    #define SYSTEM_CALL(x, y...) __arosc_enum_version_ ## x,
    #include <sys/syscall.def>
    AROSC_VERSION
    #undef SYSTEM_CALL
};

#endif /* !_SYS_AROSC_H */

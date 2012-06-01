#ifndef _SYS_AROSC_H
#define _SYS_AROSC_H

#include <aros/system.h>
#include <setjmp.h>

struct __sFILE;

struct arosc_ctype {
    const unsigned short int *b;
    const unsigned char      *toupper;
    const unsigned char      *tolower;
};

/* Defined (privately) in random.c */
struct random_state;

struct arosc_userdata
{
    /* stdio.h */
    struct __sFILE *acud_stdin;
    struct __sFILE *acud_stdout;
    struct __sFILE *acud_stderr;

    /* errno.h */
    int acud_errno;

    struct arosc_ctype acud_ctype;
 
    /* Used by time.h functions */
    int        acud_daylight;
    long int   acud_timezone;
    char     **acud_tzname;

    /* Used by multi-byte functions */
    int acud_mb_cur_max;

    /* Used for arosc startup code */
    int acud_startup_error;
    jmp_buf acud_startup_jmp_buf;

    /* Used for random()/srandom() */
    struct random_state *acud_random;
};

__BEGIN_DECLS

struct arosc_userdata *__get_arosc_userdata(void) __pure;
const struct arosc_ctype *__get_arosc_ctype(void) __pure;
int __arosc_nixmain(int (*main)(int argc, char *argv[]), int argc, char *argv[]);
int __get_default_file(int file_descriptor, long* file_handle);
void __arosc_program_startup(void);
void __arosc_program_end(void);

__END_DECLS

#define __arosc_startup_jmp_buf  (__get_arosc_userdata()->acud_startup_jmp_buf)
#define __arosc_startup_error    (__get_arosc_userdata()->acud_startup_error)
#define __arosc_random           (__get_arosc_userdata()->acud_random)

#endif /* !_SYS_AROSC_H */

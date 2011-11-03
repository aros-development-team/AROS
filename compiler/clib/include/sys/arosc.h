#ifndef _SYS_AROSC_H
#define _SYS_AROSC_H

#include <aros/system.h>
#include <setjmp.h>

struct __sFILE;

struct arosc_ctype {
    const unsigned short int *b;
    const int                *toupper;
    const int                *tolower;
};

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
    
    /* environ variable value */
    char **acud_environ;

    /* Used for arosc startup code */
    int acud_startup_error;
    jmp_buf acud_startup_jmp_buf;
};

__BEGIN_DECLS

struct arosc_userdata *__get_arosc_userdata(void) __pure;
const struct arosc_ctype *__get_arosc_ctype(void) __pure;
int __arosc_nixmain(int (*main)(int argc, char *argv[]), int argc, char *argv[]);
int __get_default_file(int file_descriptor, long* file_handle);
int __env_get_environ(char **environ, int size);
void __arosc_program_startup(void);
void __arosc_program_end(void);

__END_DECLS

#define __arosc_startup_jmp_buf  (__get_arosc_userdata()->acud_startup_jmp_buf)
#define __arosc_startup_error    (__get_arosc_userdata()->acud_startup_error)

#endif /* !_SYS_AROSC_H */

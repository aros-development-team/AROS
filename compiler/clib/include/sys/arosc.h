#ifndef _SYS_AROSC_H
#define _SYS_AROSC_H

#include <aros/system.h>

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
    
    /* environ variable value */
    char **acud_environ;
};

__BEGIN_DECLS

struct arosc_userdata *__get_arosc_userdata(void) __pure;
int __arosc_nixmain(int (*main)(int argc, char *argv[]), int argc, char *argv[]);
int __get_default_file(int file_descriptor, long* file_handle);
int __env_get_environ(char **environ, int size);
void __arosc_program_startup(void);
void __arosc_program_end(void);

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

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

struct arosc_userdata
{
    /* stdio.h */
    struct __sFILE *acud_stdin;
    struct __sFILE *acud_stdout;
    struct __sFILE *acud_stderr;

    /* errno.h */
    int acud_errno;

    struct arosc_ctype acud_ctype;

    /* Used by multi-byte functions */
    int acud_mb_cur_max;
};

__BEGIN_DECLS

struct arosc_userdata *__get_arosc_userdata(void) __pure;
int __arosc_nixmain(int (*main)(int argc, char *argv[]), int argc, char *argv[]);
int __get_default_file(int file_descriptor, long* file_handle);

__END_DECLS

#endif /* !_SYS_AROSC_H */

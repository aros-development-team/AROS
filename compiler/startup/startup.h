#ifndef AUTOINIT_H
#define AUTOINIT_H

#include <setjmp.h>
#include <proto/exec.h>

struct aros_startup
{
    jmp_buf as_startup_jmp_buf;
    LONG    as_startup_error;
};

#endif /* !AUTOINIT_H */

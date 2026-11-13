/*
    Copyright (C) 2015-2025, The AROS Development Team. All rights reserved.
*/

#include <unistd.h>
#if HAVE_SYSCALL_H
#  include <syscall.h>
#elif HAVE_SYS_SYSCALL_H
#  include <sys/syscall.h>
#else
#  error "No syscall.h found"
#endif

pid_t arostid;

void Host_PreBoot()
{
    arostid = syscall(SYS_gettid);
}

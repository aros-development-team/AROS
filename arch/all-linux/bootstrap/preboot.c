/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.
*/

#include <unistd.h>
#include <syscall.h>

pid_t arostid;

void Host_PreBoot()
{
    arostid = syscall(SYS_gettid);
}

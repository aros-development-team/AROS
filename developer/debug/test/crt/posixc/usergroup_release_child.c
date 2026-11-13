/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Child of usergroup_release: a posixc program whose getpwuid() call makes
    posixc open usergroup.library. Whether a user is found does not matter.
*/

#include <pwd.h>
#include <unistd.h>

int main(void)
{
    (void)getpwuid(getuid());
    return 0;
}

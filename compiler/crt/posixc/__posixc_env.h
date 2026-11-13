/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: AROS specific function for environ emulation handling
*/

#include "__env.h"

char ***__posixc_get_environptr (void);
int __posixc_set_envlistptr (char ***envlistptr);
char ***__posixc_get_envlistptr (void);

__env_item *__posixc_env_getvar(const char *varname, int valuesize);

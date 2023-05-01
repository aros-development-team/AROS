#ifndef ___USERGRP_H
#define ___USERGRP_H

/*
    Copyright © 2016-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <pwd.h>

void __fill_passwd(struct passwd *, uid_t);

#endif /* ___USERGRP_H */

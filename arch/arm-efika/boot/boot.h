/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: boot.h
    Lang: english
*/

#ifndef BOOT_H_
#define BOOT_H_

#include <inttypes.h>

#define __startup __attribute__((section(".aros.startup")))
#define __used __attribute__((used))

#endif /* BOOT_H_ */

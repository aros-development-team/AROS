/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The few VideoCore OS abstraction definitions the MMAL headers use.
*/

#ifndef INTERFACE_VCOS_VCOS_H
#define INTERFACE_VCOS_VCOS_H

#include <stdint.h>

typedef unsigned int VCOS_UNSIGNED;

#define vcos_static_assert(cond) _Static_assert((cond), #cond)

#endif /* INTERFACE_VCOS_VCOS_H */

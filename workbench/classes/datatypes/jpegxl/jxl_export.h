/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Static-build replacement for libjxl's cmake-generated jxl_export.h
*/

#ifndef JXL_EXPORT_H
#define JXL_EXPORT_H

#define JXL_EXPORT
#define JXL_NO_EXPORT

#ifndef JXL_DEPRECATED
#  define JXL_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef JXL_DEPRECATED_EXPORT
#  define JXL_DEPRECATED_EXPORT JXL_EXPORT JXL_DEPRECATED
#endif

#ifndef JXL_DEPRECATED_NO_EXPORT
#  define JXL_DEPRECATED_NO_EXPORT JXL_NO_EXPORT JXL_DEPRECATED
#endif

#endif /* JXL_EXPORT_H */

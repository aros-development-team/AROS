/*
 * Copyright (C) 2012-2017, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef LIBRARIES_ACPICA_H
#define LIBRARIES_ACPICA_H

#include <proto/exec.h>

#if (__WORDSIZE==64)
# define ACPI_MACHINE_WIDTH	64
#else
# define ACPI_MACHINE_WIDTH	32
#endif

#define COMPILER_DEPENDENT_INT64  QUAD
#define COMPILER_DEPENDENT_UINT64 UQUAD

/*
 * Calling conventions:
 *
 * ACPI_SYSTEM_XFACE        - Interfaces to host OS (handlers, threads)
 * ACPI_EXTERNAL_XFACE      - External ACPI interfaces
 * ACPI_INTERNAL_XFACE      - Internal ACPI interfaces
 * ACPI_INTERNAL_VAR_XFACE  - Internal variable-parameter list interfaces
 */
#define ACPI_SYSTEM_XFACE           
#define ACPI_EXTERNAL_XFACE         
#define ACPI_INTERNAL_XFACE         
#define ACPI_INTERNAL_VAR_XFACE     

#define ACPI_ALLOCATE(size)             AllocVec(size, MEMF_PUBLIC)
#define ACPI_ALLOCATE_ZEROED(size)      AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR)
#define ACPI_FREE(ptr)                  FreeVec(ptr)

#include <acpica/actypes.h>
#include <acpica/actbl.h>
#include <acpica/acbuffer.h>
#include <acpica/acrestyp.h>
#include <acpica/acpixf.h>
#include <acpica/acexcep.h>
#include <acpica/aclocal.h>

#endif /* LIBRARIES_ACPICA_H */

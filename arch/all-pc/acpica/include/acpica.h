/*
 * Copyright (C) 2012-2018, The AROS Development Team
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

#ifndef __ACAROS_H__
#define ACPI_EXTERNAL_RETURN_STATUS(Prototype)
#define ACPI_EXTERNAL_RETURN_OK(Prototype)
#define ACPI_EXTERNAL_RETURN_VOID(Prototype)
#define ACPI_EXTERNAL_RETURN_UINT32(Prototype)
#define ACPI_EXTERNAL_RETURN_PTR(Prototype)

#define ACPI_INLINE             __inline__
#define ACPI_PRINTF_LIKE(c) __attribute__ ((__format__ (__printf__, c, c+1)))

#include <acpica/actypes.h>
#include <acpica/actbl.h>
#include <acpica/acbuffer.h>
#include <acpica/acrestyp.h>
#include <acpica/acpixf.h>
#include <acpica/acexcep.h>
#include <acpica/aclocal.h>
#endif

#endif /* LIBRARIES_ACPICA_H */

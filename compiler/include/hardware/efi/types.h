#ifndef HARDWARE_EFI_TYPES_H
#define HARDWARE_EFI_TYPES_H

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: EFI type definitions
    Lang: english
*/

#include <exec/types.h>
#include <libraries/uuid.h>

/* Basic UEFI Types */

typedef UWORD 			CHAR16;

typedef QUAD            EFI_STATUS;
typedef UQUAD           EFI_TPL;
typedef UQUAD           EFI_HANDLE;
typedef void            *EFI_EVENT;
typedef UQUAD           EFI_ALLOCATE_TYPE;
typedef UQUAD           EFI_MEMORY_TYPE;
typedef UQUAD           EFI_INTERFACE_TYPE;

typedef uuid_t			EFI_GUID;

/* UEFI physical address and counts */
typedef UQUAD           EFI_PHYSICAL_ADDRESS;

/* Calling convention for UEFI ABI */
#ifdef __x86_64__
/* On x86-64 EFI uses Microsoft calling convention */
#define __eficall __attribute__((ms_abi))
#else
#define __eficall
#endif

/* Memory descriptor structure */
typedef struct {
    ULONG  Type;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_PHYSICAL_ADDRESS VirtualStart;
    UQUAD  NumberOfPages;
    UQUAD  Attribute;
} EFI_MEMORY_DESCRIPTOR;

/* Device path protocol */
typedef struct {
    UBYTE Type;
    UBYTE SubType;
    UWORD Length;
    // Followed by variable-length path data
} EFI_DEVICE_PATH_PROTOCOL;

/* Event callback */
typedef void (__eficall *EFI_EVENT_NOTIFY)(
    EFI_EVENT Event,
    void *Context
);

/* Protocol open info */
typedef struct {
    EFI_HANDLE AgentHandle;
    EFI_HANDLE ControllerHandle;
    ULONG      Attributes;
    ULONG      OpenCount;
} EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;

/* Search and timer enums */
typedef enum {
    AllHandles,
    ByRegisterNotify,
    ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

typedef enum {
    TimerCancel,
    TimerPeriodic,
    TimerRelative
} EFI_TIMER_DELAY;

/* Common UEFI status values */
#define EFI_SUCCESS               0
#define EFI_LOAD_ERROR            ((EFI_STATUS)0x8000000000000001ULL)
#define EFI_INVALID_PARAMETER     ((EFI_STATUS)0x8000000000000002ULL)
#define EFI_UNSUPPORTED           ((EFI_STATUS)0x8000000000000003ULL)
#define EFI_BAD_BUFFER_SIZE       ((EFI_STATUS)0x8000000000000004ULL)
#define EFI_BUFFER_TOO_SMALL      ((EFI_STATUS)0x8000000000000005ULL)
#define EFI_NOT_FOUND             ((EFI_STATUS)0x8000000000000014ULL)

#endif /* HARDWARE_EFI_TYPES_H */

#ifndef HARDWARE_EFI_BOOT_H
#define HARDWARE_EFI_BOOT_H

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: EFI firmware boot services
    Lang: english
*/

#include <hardware/efi/tables.h>

/* Boot Services Table */
struct EFI_BootServices {
    struct EFI_TableHeader Hdr;

    // Task Priority Services
    __eficall EFI_TPL (*RaiseTPL)(EFI_TPL NewTpl);
    __eficall void (*RestoreTPL)(EFI_TPL OldTpl);

    // Memory Services
    __eficall EFI_STATUS (*AllocatePages)(
        EFI_ALLOCATE_TYPE Type,
        EFI_MEMORY_TYPE MemoryType,
        UQUAD Pages,
        EFI_PHYSICAL_ADDRESS *Memory
    );
    __eficall EFI_STATUS (*FreePages)(
        EFI_PHYSICAL_ADDRESS Memory,
        UQUAD Pages
    );
    __eficall EFI_STATUS (*GetMemoryMap)(
        UQUAD *MemoryMapSize,
        EFI_MEMORY_DESCRIPTOR *MemoryMap,
        UQUAD *MapKey,
        UQUAD *DescriptorSize,
        ULONG *DescriptorVersion
    );
    __eficall EFI_STATUS (*AllocatePool)(
        EFI_MEMORY_TYPE PoolType,
        UQUAD Size,
        void **Buffer
    );
    __eficall EFI_STATUS (*FreePool)(
        void *Buffer
    );

    // Event & Timer Services
    __eficall EFI_STATUS (*CreateEvent)(
        ULONG Type,
        EFI_TPL NotifyTpl,
        EFI_EVENT_NOTIFY NotifyFunction,
        void *NotifyContext,
        EFI_EVENT *Event
    );
    __eficall EFI_STATUS (*SetTimer)(
        EFI_EVENT Event,
        ULONG Type,
        UQUAD TriggerTime
    );
    __eficall EFI_STATUS (*WaitForEvent)(
        UQUAD NumberOfEvents,
        EFI_EVENT *Event,
        UQUAD *Index
    );
    __eficall EFI_STATUS (*SignalEvent)(EFI_EVENT Event);
    __eficall EFI_STATUS (*CloseEvent)(EFI_EVENT Event);
    __eficall EFI_STATUS (*CheckEvent)(EFI_EVENT Event);

    // Protocol Handler Services
    __eficall EFI_STATUS (*InstallProtocolInterface)(
        EFI_HANDLE *Handle,
        const EFI_GUID *Protocol,
        EFI_INTERFACE_TYPE InterfaceType,
        void *Interface
    );
    __eficall EFI_STATUS (*ReinstallProtocolInterface)(
        EFI_HANDLE Handle,
        const EFI_GUID *Protocol,
        void *OldInterface,
        void *NewInterface
    );
    __eficall EFI_STATUS (*UninstallProtocolInterface)(
        EFI_HANDLE Handle,
        const EFI_GUID *Protocol,
        void *Interface
    );
    __eficall EFI_STATUS (*HandleProtocol)(
        EFI_HANDLE Handle,
        const EFI_GUID *Protocol,
        void **Interface
    );
    void *Reserved;
    __eficall EFI_STATUS (*RegisterProtocolNotify)(
        const EFI_GUID *Protocol,
        EFI_EVENT Event,
        void **Registration
    );
    __eficall EFI_STATUS (*LocateHandle)(
        EFI_LOCATE_SEARCH_TYPE SearchType,
        const EFI_GUID *Protocol,
        void *SearchKey,
        UQUAD *BufferSize,
        EFI_HANDLE *Buffer
    );
    __eficall EFI_STATUS (*LocateDevicePath)(
        const EFI_GUID *Protocol,
        EFI_DEVICE_PATH_PROTOCOL **DevicePath,
        EFI_HANDLE *Device
    );
    __eficall EFI_STATUS (*InstallConfigurationTable)(
        const EFI_GUID *Guid,
        void *Table
    );

    // Image Services
    __eficall EFI_STATUS (*LoadImage)(
        UBYTE BootPolicy,
        EFI_HANDLE ParentImageHandle,
        EFI_DEVICE_PATH_PROTOCOL *DevicePath,
        void *SourceBuffer,
        UQUAD SourceSize,
        EFI_HANDLE *ImageHandle
    );
    __eficall EFI_STATUS (*StartImage)(
        EFI_HANDLE ImageHandle,
        UQUAD *ExitDataSize,
        const CHAR16 **ExitData
    );
    __eficall EFI_STATUS (*Exit)(
        EFI_HANDLE ImageHandle,
        EFI_STATUS ExitStatus,
        UQUAD ExitDataSize,
        const CHAR16 *ExitData
    );
    __eficall EFI_STATUS (*UnloadImage)(
        EFI_HANDLE ImageHandle
    );
    __eficall EFI_STATUS (*ExitBootServices)(
        EFI_HANDLE ImageHandle,
        UQUAD MapKey
    );

    // Misc Services
    __eficall EFI_STATUS (*GetNextMonotonicCount)(UQUAD *Count);
    __eficall EFI_STATUS (*Stall)(UQUAD Microseconds);
    __eficall EFI_STATUS (*SetWatchdogTimer)(
        UQUAD Timeout,
        UQUAD WatchdogCode,
        UQUAD DataSize,
        const CHAR16 *WatchdogData
    );

    // Driver Support Services
    __eficall EFI_STATUS (*ConnectController)(
        EFI_HANDLE ControllerHandle,
        EFI_HANDLE *DriverImageHandle,
        EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath,
        UBYTE Recursive
    );
    __eficall EFI_STATUS (*DisconnectController)(
        EFI_HANDLE ControllerHandle,
        EFI_HANDLE DriverImageHandle,
        EFI_HANDLE ChildHandle
    );

    // Protocol Access Services
    __eficall EFI_STATUS (*OpenProtocol)(
        EFI_HANDLE Handle,
        const EFI_GUID *Protocol,
        void **Interface,
        EFI_HANDLE AgentHandle,
        EFI_HANDLE ControllerHandle,
        ULONG Attributes
    );
    __eficall EFI_STATUS (*CloseProtocol)(
        EFI_HANDLE Handle,
        const EFI_GUID *Protocol,
        EFI_HANDLE AgentHandle,
        EFI_HANDLE ControllerHandle
    );
    __eficall EFI_STATUS (*OpenProtocolInformation)(
        EFI_HANDLE Handle,
        const EFI_GUID *Protocol,
        EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
        UQUAD *EntryCount
    );

    // Library Services
    __eficall EFI_STATUS (*ProtocolsPerHandle)(
        EFI_HANDLE Handle,
        const EFI_GUID ***ProtocolBuffer,
        UQUAD *ProtocolBufferCount
    );
    __eficall EFI_STATUS (*LocateHandleBuffer)(
        EFI_LOCATE_SEARCH_TYPE SearchType,
        const EFI_GUID *Protocol,
        void *SearchKey,
        UQUAD *NoHandles,
        EFI_HANDLE **Buffer
    );
    __eficall EFI_STATUS (*LocateProtocol)(
        const EFI_GUID *Protocol,
        void *Registration,
        void **Interface
    );
    __eficall EFI_STATUS (*InstallMultipleProtocolInterfaces)(
        EFI_HANDLE *Handle,
        ...
    );
    __eficall EFI_STATUS (*UninstallMultipleProtocolInterfaces)(
        EFI_HANDLE Handle,
        ...
    );

    // CRC32 and memory operations
    __eficall EFI_STATUS (*CalculateCrc32)(
        const void *Data,
        UQUAD DataSize,
        ULONG *Crc32
    );
    __eficall void (*CopyMem)(void *Destination, const void *Source, UQUAD Length);
    __eficall void (*SetMem)(void *Buffer, UQUAD Size, UBYTE Value);
};

#define EFI_BOOT_SERVICES_SIGNATURE 0x56524553544f4f42

#endif

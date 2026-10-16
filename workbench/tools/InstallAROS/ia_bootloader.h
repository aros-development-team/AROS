#ifndef IA_BOOTLOADER_H
#define IA_BOOTLOADER_H
/*
    Copyright © 2018-2023, The AROS Development Team. All rights reserved.
    $Id$
*/

#if defined(__i386__) || defined(__x86_64__)
#define INSTALL_BL_GRUB
#define INSTALL_BL_GRUB2
#define GRUBARCHDIR "i386-pc"
#endif
/*
// Not yet supported/used
#if defined(__arm__) || defined(__aarch64__)
#define INSTALL_BL_GRUB2
#define GRUBARCHDIR "arm-xx"
#endif
*/

#if defined(__riscv) && (__riscv_xlen == 64)
/* The firmware reads the stub, and the stub reads the package and the
   command line, from the ESP - none of it can live on the AROS filesystem */
#define INSTALL_BL_EFI
#define EFI_STUB_FILE   "BOOTRISCV64.EFI"
#endif

struct InstallStage_DATA;

enum BootLoaderTypes
{
    BOOTLOADER_NONE = -1,
#if defined(INSTALL_BL_GRUB)
    BOOTLOADER_GRUB1,
#endif
#if defined(INSTALL_BL_GRUB2)
    BOOTLOADER_GRUB2,
#endif
#if defined(INSTALL_BL_EFI)
    BOOTLOADER_EFI,
#endif
    BOOTLOADER_cnt
};

struct BootLoaderInfo
{
    CONST_STRPTR path;
    CONST_STRPTR dest;
    CONST_STRPTR match;
    CONST_STRPTR match2;
    CONST_STRPTR match3;
};

extern struct BootLoaderInfo BootLoaderData[];
extern LONG BootLoaderType;

extern void BOOTLOADER_InitSupport(void);
extern BOOL BOOTLOADER_PartFixUp(struct InstallStage_DATA *data, IPTR systype);
extern void BOOTLOADER_DoInstall(Class * CLASS, Object * self);
extern void BOOTLOADER_AddCoreSkipPaths(struct List *SkipList);
#if defined(INSTALL_BL_EFI)
extern BOOL BOOTLOADER_ESPInfo(STRPTR buf, ULONG bufLen, BOOL *formatted);
#endif

#endif /* IA_BOOTLOADER_H */

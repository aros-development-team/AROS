#ifndef IA_BOOTLOADER_H
#define IA_BOOTLOADER_H
/*
    Copyright © 2018-2020, The AROS Development Team. All rights reserved.
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

struct Install_DATA;

enum BootLoaderTypes
{
    BOOTLOADER_NONE = -1,
#if defined(INSTALL_BL_GRUB)
    BOOTLOADER_GRUB1,
#endif
#if defined(INSTALL_BL_GRUB2)
    BOOTLOADER_GRUB2,
#endif
    BOOTLOADER_cnt
};

struct BootLoaderInfo
{
    CONST_STRPTR path;
    CONST_STRPTR match;
    CONST_STRPTR match2;
    CONST_STRPTR match3;
};

extern struct BootLoaderInfo BootLoaderData[];
extern LONG BootLoaderType;

extern void BOOTLOADER_InitSupport(void);
extern void BOOTLOADER_PartFixUp(struct Install_DATA *data, IPTR systype);
extern void BOOTLOADER_DoInstall(Class * CLASS, Object * self);
extern void BOOTLOADER_AddCoreSkipPaths(struct List *SkipList);

#endif /* IA_BOOTLOADER_H */

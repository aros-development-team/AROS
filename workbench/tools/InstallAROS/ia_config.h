#ifndef IA_CONFIG_H
#define IA_CONFIG_H

struct Install_Options
{
    Object	                *opt_license;
    Object	                *opt_lic_box;
    Object	                *opt_lic_mgrp;

    Object	                *opt_partmethod;
    Object	                *opt_sysdevname;
    Object	                *opt_workdevname;

    Object	                *opt_format;
    
    Object	                *opt_locale;
    Object	                *opt_copycore;
    Object	                *opt_copyextra;
    Object	                *opt_developer;
    Object	                *opt_bootloader;

    Object	                *opt_reboot;

    BOOL	                partitioned;
    BOOL	                bootloaded;
};

struct Grub_Options
{
    Object                      *gopt_drive;
    Object                      *gopt_grub;
    Object                      *gopt_grub2mode;
    BOOL                        bootinfo;
};

struct optionstmp
{
    IPTR	                opt_format;
    IPTR	                opt_locale;
    IPTR	                opt_copycore;
    IPTR	                opt_copyextra;
    IPTR	                opt_developer;
    IPTR	                opt_bootloader;
    IPTR	                opt_reboot;
};

#endif /* IA_CONFIG_H */


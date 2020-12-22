#ifndef INSTALL_INTERN_H
#define INSTALL_INTERN_H

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

struct Install_DATA
{
    /* Source and Destination Paths used in the Install stage */
    char                        *install_Source;
    char                        *install_SysTarget;
    char                        *install_WorkTarget;
    /* Bootloader Target used in the Install stage */
    char                        *bl_TargetDevice;
    ULONG                       bl_TargetUnit;
    /* File copy pattern mask */
    char                        *install_Pattern;

    /* general objects */
    Object                      *partition;
    Object                      *page;
    Object                      *gauge1;
    Object                      *gauge2;
    Object                      *label;
    /**/
    Object                      *installer;

    Object                      *welcomeMsg;
    Object                      *doneMsg;
    
    /**/
    Object                      *window;
    Object                      *contents;

    Object                      *pagetitle;
    Object                      *pageheader;
    Object                      *actioncurrent;

    Object                      *back;
    Object                      *proceed;
    Object                      *cancel;

    /**/
    ULONG                       inst_success;

    ULONG                       disable_back,
                                status_back,
                                status_proceed,
                                status_cancel;

    ULONG                       cur_width,
                                cur_height;

    char                        *instc_lic_file;
    APTR			instc_lic_buffer;

    struct List     		instc_packages;
    struct List     		instc_undorecord;

    /** Option Related **/

    struct Install_Options      *instc_options_main;
    struct Grub_Options		*instc_options_grub;
    struct optionstmp		*instc_options_backup;
    enum EStage                 instc_stage_prev;
    enum EStage                 instc_stage_next;

    enum IO_OVERWRITE_FLAGS     IO_Always_overwrite;

    BOOL                        instc_default_usb;
    BOOL                        instc_cflag_driveset;
    BOOL			instc_copt_undoenabled;
    BOOL			instc_copt_licensemandatory;
};

BOOPSI_DISPATCHER_PROTO(IPTR, Install__Dispatcher, CLASS, self, message);

#endif /* INSTALL_INTERN_H */

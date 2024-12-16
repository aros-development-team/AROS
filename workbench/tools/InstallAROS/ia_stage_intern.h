#ifndef _IA_STAGE_INTERN_H
#define _IA_STAGE_INTERN_H

#include "ia_config.h"

struct InstallStage_DATA
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

    struct InstallIO_Data
    {
        ULONG                   iio_BuffSize;
        APTR                    iio_Buffer; 
        enum IO_OVERWRITE_FLAGS iio_AlwaysOverwrite;
    }                           instc_IOd;

    BOOL			instc_copt_undoenabled;
    BOOL			instc_copt_licensemandatory;
};

BOOPSI_DISPATCHER_PROTO(IPTR, InstallStage__Dispatcher, CLASS, self, message);

#endif /* _IA_STAGE_INTERN_H */

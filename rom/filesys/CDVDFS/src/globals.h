#ifndef GLOBALS_H
#define GLOBALS_H

#include <intuition/intuition.h>
#include <libraries/codesets.h>
#include <workbench/workbench.h>

#include "cdrom.h"
#include "debug.h"
#include "device.h"
#include "generic.h"
#include "volumes.h"

/* Per-CD base */
struct CDVDBase
{
    struct MinNode MinNode;

    /* Library bases */
    struct DosLibrary *g_DOSBase;
    struct UtilityBase *g_UtilityBase;/* Utility library for miscellaneous tasks */
    struct IntuitionBase *g_IntuitionBase;
    struct IconBase *g_IconBase;
    struct WorkbenchBase *g_WorkbenchBase;
    struct Library *g_CodesetsBase;

    /* these two ones are needed to create new volume nodes */
    struct ACDRBase *acdrbase;      /* Base of the AROS handler */
    struct ACDRDeviceInfo *device;  /* device info for the handler */
    CDROM	*g_cd;
    VOLUME	*g_volume;
    CDROM_OBJ	*g_top_level_obj;

    char	*g_vol_name;
    char	*g_iconname;
    PROC	*DosProc;        /* Our Process */
    DEVNODE         *DosNode;        /* Our DOS node.. created by DOS for us */
    ULONG	g_dos_sigbit;
    struct DosPacket *g_death_packet; /* ACTION_DIE packet is held here until just before exit */

    struct DeviceList	*DevList;/* Device List structure for our volume node */
    struct MsgPort	*g_timer_mp; /*  timer message port	*/
    struct timerequest	*g_timer_io; /*  timer i/o request	*/
    ULONG   g_timer_sigbit;
    int   g_changeint_signumber;
    ULONG   g_changeint_sigbit;
    int	g_scan_interval; /* Time between successive diskchange checks */
    int	g_scan_time;     /* Countdown for diskchange	          */
    int	g_time;          /* Current time	          */

    char    g_device[80];    /* SCSI device name	          */
    short   g_unit;          /* SCSI unit		      */

    int	g_inhibited;     /* Number of active INHIBIT(TRUE) packets    */

    ULONG   g_memory_type;
    WORD    g_retry_mode;
    WORD    g_use_rock_ridge;         /* Use Rock Ridge flag 'R'	    */
    WORD    g_use_joliet;	  /* Use Joliet flag 'J'	    */
    WORD    g_maybe_map_to_lowercase; /* Conditional map to lower case flag 'ML'    */
    WORD    g_map_to_lowercase;       /* Map to lower case flag 'L'	    */
    int	g_std_buffers;            /* Number of buffers for standard SCSI access */
    int	g_file_buffers;           /* Number of buffers for contiguous reads */
    t_bool  g_show_version_numbers;   /* Show version numbers		*/
    t_bool  g_disk_inserted;          /* Is a disk inserted?		*/
    char    g_play_cdda_command[80];  /* Command invoked if appicon is activated    */
    UBYTE   playing;                  /* CD-DA play status		*/

    int	g_cdrom_errno;
    int	g_ignore_blocklength;     /* should be FALSE */

    /* HFS */
    char    g_data_fork_extension[17];    /* should be initialized with zeros */
    char    g_resource_fork_extension[17];/* dito		  */
    t_bool  g_convert_hfs_filenames;      /* should be FALSE	      */
    t_bool  g_convert_hfs_spaces;         /* dito		  */
    /* iso9660 */
    int	iso_errno;
    /* intui */
    struct  MsgPort *g_app_port;
    struct  AppIcon *g_app_icon;
    ULONG   g_app_sigbit;
    int	g_retry_show_cdda_icon;         /* should be FALSE */
    struct  DiskObject *g_user_disk_object; /* should be NULL */
    /* generic */
    t_bool  g_hfs_first;                    /* should be FALSE */
    /* intui */
    struct  Image g_disk_object_image;
    UBYTE   *g_tool_types[1];	    /* should be initialized with NULL */
    struct  DiskObject g_disk_object;
    LONG    g_xpos;		/* NO_ICON_POSITION */
    LONG    g_ypos;		/* NO_ICON_POSITION */
    t_fh_node *g_fh_list;
    t_lock_node *g_lock_list;
    t_vol_reg_node *g_volume_list;
    struct Task *PrefsProc;

    /* codepage translation */
    struct codeset *uniCodeset;

    PORT *Dback;
    MSG DummyMsg;
#if !defined(NDEBUG) || defined(DEBUG_SECTORS)
	        /*  DEBUGGING		*/
    PORT *Dbport;       /*  owned by the debug process	*/
    WORD DBDisable;
#endif

};

#define DOSBase       (global->g_DOSBase)
#define UtilityBase   (global->g_UtilityBase)
#define IntuitionBase (global->g_IntuitionBase)
#define IconBase      (global->g_IconBase)
#define WorkbenchBase (global->g_WorkbenchBase)
#define CodesetsBase  (global->g_CodesetsBase)

#endif /* GLOBALS_H */


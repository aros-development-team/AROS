/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id: tap_intern.h 23803 2005-12-11 11:58:09Z verhaegs $
*/

#ifndef TAP_HIDD_INTERN_H
#define TAP_HIDD_INTERN_H

/* Include files */

#include <sys/termios.h>

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_TAP_H
#   include <hidd/tap.h>
#endif
#include <dos/dos.h>

#define PAR_MAX_UNITS	3

struct HIDDTapData
{
    OOP_Class 		*TapHIDDClass;

    OOP_Object		*TapUnits[PAR_MAX_UNITS];
    UBYTE		usedunits;
};


#define PAR_UNIT_0_F	1
#define PAR_UNIT_1_F	2
#define PAR_UNIT_2_F	4


struct class_static_data
{
    struct ExecBase      * sysbase;

    OOP_Class		 *taphiddclass;
    OOP_Class		 *tapunitclass;
};

struct HIDDTapUnitData
{
    VOID (*DataWriteCallBack)	(ULONG unitnum, APTR userdata);
    VOID (*DataReceivedCallBack)(UBYTE *buffer, ULONG len, ULONG unitnum, APTR userdata);
    VOID		*DataWriteUserData;
    VOID		*DataReceivedUserData;
    
    ULONG 		unitnum;
    int			filedescriptor;

    BOOL                stopped;
    
    struct MsgPort	*replyport_read;
    struct Interrupt 	*softint_read;
    HIDD		unixio_read;
    
    struct MsgPort	*replyport_write;
    struct Interrupt 	*softint_write;
    HIDD		unixio_write;
    
};


/* Library base */

struct IntHIDDTapBase
{
    struct Library            hdg_LibNode;
    BPTR                      hdg_SegList;
    struct ExecBase          *hdg_SysBase;

    struct class_static_data  hdg_csd;
};


#define CSD(cl) (&((struct IntHIDDTapBase *)cl)->hdg_csd)

/* pre declarations */

OOP_Class *init_taphiddclass(struct class_static_data *csd);
void   free_taphiddclass(struct class_static_data *csd);

OOP_Class *init_tapunitclass(struct class_static_data *csd);
void   free_tapunitclass(struct class_static_data *csd);


#endif /* TAP_HIDD_INTERN_H */

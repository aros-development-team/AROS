#ifndef LIBRARIES_LOWLEVEL_EXT_H
#define LIBRARIES_LOWLEVEL_EXT_H

/*
**	$VER: lowlevel_ext.h 40.6 (30.7.1993)
**	Includes Release 50.x
**
**	extended lowlevel.library interface structures and definitions.
**
**	(C) Copyright 2004 Chris Hodges
**	All Rights Reserved
*/

/*****************************************************************************/


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef LIBRARIES_LOWLEVEL_H
#include <libraries/lowlevel.h>
#endif

/*****************************************************************************/

/* New Tags for SetJoyPortAttrs() */
#define SJA_RumbleSetSlowMotor (SJA_Dummy+100) /* set rumble pack slow motor speed (0x00-0xff) */
#define SJA_RumbleSetFastMotor (SJA_Dummy+101) /* set rumble pack fast motor speed (0x00-0xff) */
#define SJA_RumbleOff          (SJA_Dummy+102) /* turn rumble effect off */

/* New Controller types for SJA_Type tag */
#define SJA_TYPE_ANALOGUE  14

/*****************************************************************************/

/* New ReadJoyPort() return value definitions */

/* Port types */
#define JP_TYPE_ANALOGUE (14<<28)   /* port has analogue joystick  */

/* Analogue joystick position reports, valid for JP_TYPE_ANALOGUE */
#define JP_XAXIS_MASK	(255<<0)	/* horizontal position */
#define JP_YAXIS_MASK	(255<<8)	/* vertical position */
#define JP_XYAXIS_MASK	(JP_XAXIS_MASK|JP_YAXIS_MASK)

/*****************************************************************************/

#define JP_ANALOGUE_PORT_MAGIC (1<<16) /* port offset to force analogue readout */

#endif /* LIBRARIES_LOWLEVEL_EXT_H */

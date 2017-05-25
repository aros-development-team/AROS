#ifndef LIBRARIES_LOWLEVEL_H
#define LIBRARIES_LOWLEVEL_H

/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for lowlevel.library
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif


/********************* Keyboard Section *********************/

/* KeyQuery structure for use with QueryKeys() */
struct KeyQuery
{
    UWORD kq_KeyCode;
    BOOL  kq_Pressed;
};

/* Bits in the return value of GetKey() */
#define LLKB_LSHIFT	16
#define LLKB_RSHIFT	17
#define LLKB_CAPSLOCK	18
#define LLKB_CONTROL	19
#define LLKB_LALT	20
#define LLKB_RALT	21
#define LLKB_LAMIGA	22
#define LLKB_RAMIGA	23

#define LLKF_LSHIFT	(1<<LLKB_LSHIFT)
#define LLKF_RSHIFT	(1<<LLKB_RSHIFT)
#define LLKF_CAPSLOCK	(1<<LLKB_CAPSLOCK)
#define LLKF_CONTROL	(1<<LLKB_CONTROL)
#define LLKF_LALT	(1<<LLKB_LALT)
#define LLKF_RALT	(1<<LLKB_RALT)
#define LLKF_LAMIGA	(1<<LLKB_LAMIGA)
#define LLKF_RAMIGA	(1<<LLKB_RAMIGA)


/********************* JoyPort Section *********************/

/* Tags for SetJoyPortAttrs() */
#define SJA_Dummy		(TAG_USER+0xC00100)
#define SJA_Type		(SJA_Dummy+1) /* Force type to Mouse, Joy, Game Controler */
#define SJA_Reinitialize	(SJA_Dummy+2) /* Free potgo bits and reset to autosense	*/

/* Controller types for SJA_Type Tag */
#define SJA_TYPE_AUTOSENSE	0
#define SJA_TYPE_GAMECTLR	1
#define SJA_TYPE_MOUSE		2
#define SJA_TYPE_JOYSTK		3

/* Return value definitions for ReadJoyPort() */

/* Port types */
#define JP_TYPE_NOTAVAIL	(00<<28) /* Port data unavailable	*/
#define JP_TYPE_GAMECTLR	(01<<28) /* Port has Game Controller	*/
#define JP_TYPE_MOUSE		(02<<28) /* Port has Mouse		*/
#define JP_TYPE_JOYSTK		(03<<28) /* Port has Joystick		*/
#define JP_TYPE_UNKNOWN		(04<<28) /* Port has Unknown Device	*/
#define JP_TYPE_MASK		(15<<28) /* Controller Type		*/

/* Button types, valid for all types except JP_TYPE_NOTAVAIL */
#define JPB_BUTTON_PLAY		17 /* Grey - Play/Pause; Middle Mouse	*/
#define JPB_BUTTON_REVERSE	18 /* Charcoal - Reverse		*/
#define JPB_BUTTON_FORWARD	19 /* Charcoal - Forward		*/
#define JPB_BUTTON_GREEN	20 /* Green - Shuffle			*/
#define JPB_BUTTON_YELLOW	21 /* Yellow - Repeat			*/
#define JPB_BUTTON_RED		22 /* Red - Select; Left Mouse; Joystick Fire */
#define JPB_BUTTON_BLUE		23 /* Blue - Stop; Right Mouse		*/

#define JPF_BUTTON_PLAY		(1<<JPB_BUTTON_PLAY)
#define JPF_BUTTON_REVERSE	(1<<JPB_BUTTON_REVERSE)
#define JPF_BUTTON_FORWARD	(1<<JPB_BUTTON_FORWARD)
#define JPF_BUTTON_GREEN	(1<<JPB_BUTTON_GREEN)
#define JPF_BUTTON_YELLOW	(1<<JPB_BUTTON_YELLOW)
#define JPF_BUTTON_RED		(1<<JPB_BUTTON_RED)
#define JPF_BUTTON_BLUE		(1<<JPB_BUTTON_BLUE)

#define JP_BUTTON_MASK	(JPF_BUTTON_PLAY|JPF_BUTTON_REVERSE|JPF_BUTTON_FORWARD|JPF_BUTTON_GREEN|JPF_BUTTON_YELLOW|JPF_BUTTON_RED|JPF_BUTTON_BLUE)

/* Direction types, valid for JP_TYPE_GAMECTLR and JP_TYPE_JOYSTK */
#define JPB_JOY_RIGHT	0
#define JPB_JOY_LEFT	1
#define JPB_JOY_DOWN	2
#define JPB_JOY_UP	3

#define JPF_JOY_RIGHT	(1<<JPB_JOY_RIGHT)
#define JPF_JOY_LEFT	(1<<JPB_JOY_LEFT)
#define JPF_JOY_DOWN	(1<<JPB_JOY_DOWN)
#define JPF_JOY_UP	(1<<JPB_JOY_UP)

#define JP_DIRECTION_MASK	(JPF_JOY_RIGHT|JPF_JOY_LEFT|JPF_JOY_DOWN|JPF_JOY_UP)

/* Mouse position reports, valid for JP_TYPE_MOUSE */
#define JP_MHORZ_MASK	(255<<0)	/* Horizontal position	*/
#define JP_MVERT_MASK	(255<<8)	/* Vertical position	*/
#define JP_MOUSE_MASK	(JP_MHORZ_MASK|JP_MVERT_MASK)


/********************* SystemControl Section *********************/

/* Tags for SystemControl() */
#define SCON_Dummy		(TAG_USER+0x00C00000)
#define SCON_TakeOverSys	(SCON_Dummy+0)
#define SCON_KillReq		(SCON_Dummy+1)
#define SCON_CDReboot		(SCON_Dummy+2)
#define SCON_StopInput		(SCON_Dummy+3)
#define SCON_AddCreateKeys	(SCON_Dummy+4)
#define SCON_RemCreateKeys	(SCON_Dummy+5)

/* Reboot control values for use with SCON_CDReboot Tag */
#define CDReboot_Off		0
#define CDReboot_On		1
#define CDReboot_Default	2


/********************* Rawkey Section *********************/

/* Rawkey codes returned when using SCON_AddCreateKeys with SystemControl() */

#define RAWKEY_PORT0_BUTTON_BLUE	0x072
#define RAWKEY_PORT0_BUTTON_PLAY	0x073
#define RAWKEY_PORT0_BUTTON_REVERSE	0x074
#define RAWKEY_PORT0_BUTTON_FORWARD	0x075
#define RAWKEY_PORT0_BUTTON_GREEN	0x076
#define RAWKEY_PORT0_BUTTON_YELLOW	0x077
#define RAWKEY_PORT0_BUTTON_RED		0x078
#define RAWKEY_PORT0_JOY_UP		0x079
#define RAWKEY_PORT0_JOY_DOWN		0x07A
#define RAWKEY_PORT0_JOY_RIGHT		0x07B
#define RAWKEY_PORT0_JOY_LEFT		0x07C

#define RAWKEY_PORT1_BUTTON_BLUE	0x172
#define RAWKEY_PORT1_BUTTON_PLAY	0x173
#define RAWKEY_PORT1_BUTTON_REVERSE	0x174
#define RAWKEY_PORT1_BUTTON_FORWARD	0x175
#define RAWKEY_PORT1_BUTTON_GREEN	0x176
#define RAWKEY_PORT1_BUTTON_YELLOW	0x177
#define RAWKEY_PORT1_BUTTON_RED		0x178
#define RAWKEY_PORT1_JOY_UP		0x179
#define RAWKEY_PORT1_JOY_DOWN		0x17A
#define RAWKEY_PORT1_JOY_RIGHT		0x17B
#define RAWKEY_PORT1_JOY_LEFT		0x17C

#define RAWKEY_PORT2_BUTTON_BLUE	0x272
#define RAWKEY_PORT2_BUTTON_PLAY	0x273
#define RAWKEY_PORT2_BUTTON_REVERSE	0x274
#define RAWKEY_PORT2_BUTTON_FORWARD	0x275
#define RAWKEY_PORT2_BUTTON_GREEN	0x276
#define RAWKEY_PORT2_BUTTON_YELLOW	0x277
#define RAWKEY_PORT2_BUTTON_RED		0x278
#define RAWKEY_PORT2_JOY_UP		0x279
#define RAWKEY_PORT2_JOY_DOWN		0x27A
#define RAWKEY_PORT2_JOY_RIGHT		0x27B
#define RAWKEY_PORT2_JOY_LEFT		0x27C

#define RAWKEY_PORT3_BUTTON_BLUE	0x372
#define RAWKEY_PORT3_BUTTON_PLAY	0x373
#define RAWKEY_PORT3_BUTTON_REVERSE	0x374
#define RAWKEY_PORT3_BUTTON_FORWARD	0x375
#define RAWKEY_PORT3_BUTTON_GREEN	0x376
#define RAWKEY_PORT3_BUTTON_YELLOW	0x377
#define RAWKEY_PORT3_BUTTON_RED		0x378
#define RAWKEY_PORT3_JOY_UP		0x379
#define RAWKEY_PORT3_JOY_DOWN		0x37A
#define RAWKEY_PORT3_JOY_RIGHT		0x37B
#define RAWKEY_PORT3_JOY_LEFT		0x37C


/********************* Language Section *********************/

/* Return values for GetLanguageSelection() */

#define LANG_UNKNOWN	 0
#define LANG_AMERICAN	 1	/* American English	*/
#define LANG_ENGLISH	 2	/* British English	*/
#define LANG_GERMAN	 3
#define LANG_FRENCH	 4
#define LANG_SPANISH	 5
#define LANG_ITALIAN	 6
#define LANG_PORTUGUESE	 7
#define LANG_DANISH	 8
#define LANG_DUTCH	 9
#define LANG_NORWEGIAN	10
#define LANG_FINNISH	11
#define LANG_SWEDISH	12
#define LANG_JAPANESE	13
#define LANG_CHINESE	14
#define LANG_ARABIC	15
#define LANG_GREEK	16
#define LANG_HEBREW	17
#define LANG_KOREAN	18


#endif /* LIBRARIES_LOWLEVEL_H */

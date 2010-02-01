#ifndef _STRINGIFY_H
#define _STRINGIFY_H

/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Stringify_CLASS;

/*** Macros *****************************************************************/
#define StringifyObject BOOPSIOBJMACRO_START(Stringify_CLASS->mcc_Class)

/*** Attributes *************************************************************/
#define MUIA_MyStringifyType        0x8001

/*** Values *****************************************************************/
#define STRINGIFY_DoubleClickDelay  0x00
#define STRINGIFY_RepeatDelay       0x01
#define STRINGIFY_RepeatRate        0x02

#endif

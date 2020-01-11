/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header file for lowlevel library
    Lang: english
*/
#ifndef __LOWLEVEL_INTERN_H__
#define __LOWLEVEL_INTERN_H__

#include <proto/exec.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>

#include <aros/debug.h>

struct llCIATimer
{
    struct Library              *llciat_Base;
    struct Interrupt            llciat_Int;
    WORD                        llciat_iCRBit;
};

struct llKBInterrupt
{
    struct Interrupt        	*llkbi_Interrupt;
    ULONG                       llkbi_KeyData;
    APTR                        llkbi_Data;
    VOID                        (* llkbi_Code)();
};

/*
    This is the LowLevelBase structure. It is documented here because 
    it is completely private. Applications should treat it as a struct 
    Library, and use the workbench.library functions to get information.
*/
struct LowLevelBase
{
    struct Library              ll_Lib;

    struct SignalSemaphore      ll_Lock;
    struct Interrupt            ll_VBlank;
    BPTR                        ll_SegList;

    struct Library              *ll_InputBase;
    struct Interrupt        	*ll_InputHandler;
    struct MsgPort          	*ll_InputMP;
    struct IOStdReq         	*ll_InputIO;

    ULONG                       ll_LastKey;
    struct List                 ll_KBInterrupts;

#if (1)
    /*
     * Variables used by amiga-m68k
     * TODO: these should be handled in an arch specific manner ..
     * see kernelbase.
     */
    ULONG                       ll_PortType[2];
    struct Library              *ll_PotgoBase;
    struct llCIATimer           ll_CIA;
    WORD                        ll_EClockMult;
#endif
};

/*
 * Defintion of internal structures.
 */

#endif /* __LOWLEVEL_INTERN_H__  */


/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for expansion.library
    Lang: english
*/

#ifndef _SHELLCOMMANDS_INTERN_H
#define _SHELLCOMMANDS_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>

#include <aros/shcommands.h>

struct ShellCommandsBase {
    struct Library sc_Lib;

    int		sc_Commands;              /* Number of commands */

    /* This is both a segment, and the data for the segment.
     * We will feed in to DOS/AddSegment() the BPTR to 
     * &sc_Command[i].scs_Next as the 'seglist' to add.
     */
    struct ShellCommandSeg {
    	ULONG              scs_Size;      /* Length of segment in # of ULONGs */
    	ULONG              scs_Next;      /* Next segment (always 0 for this) */
    	struct FullJumpVec scs_Code;      /* Code to jump to shell command */
    	CONST_STRPTR __attribute__((aligned(4)))  scs_Name;      /* Name of the segment */
    } *sc_Command;

    /* Bookkeeping */
    BPTR	sc_SegList;

    APTR	sc_DOSBase;
};

extern struct ExecBase *SysBase;
#define DOSBase	(ShellCommandsBase->sc_DOSBase)


#endif /* _SHELLCOMMANDS_INTERN_H */

/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Support functions for console.device
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1
#include <proto/exec.h>
#include <exec/lists.h>
#include <exec/io.h>
#include <exec/memory.h>

#include <proto/boopsi.h>

#include <devices/conunit.h>
#include <string.h>
#include "console_gcc.h"

#include "consoleif.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

static BOOL getparamcommand(BYTE *cmd_ptr, UBYTE **writestr_ptr, UBYTE *str_end, UBYTE *p_tab);
static BOOL string2command(BYTE *cmd_ptr, UBYTE **writestr_ptr, UBYTE *str_end, UBYTE *p_tab, struct ConsoleBase *ConsoleDevice);

#define ESC 0x1B
#define CSI 0x9B

#define BELL	 	0x07
#define BACKSPACE 	0x08
#define HTAB		0x09
#define LINEFEED	0x0A
#define VTAB		0x0B
#define FORMFEED	0x0C
#define CARRIAGE_RETURN	0x0D
#define SHIFT_OUT	0x0E
#define SHIFT_IN	0x0F
#define INDEX		0x84
#define NEXT_LINE	0x85
#define H_TAB_SET	0x88
#define REVERSE_INDEX	0x8D


#define FIRST_CSI_CMD 0x40

/***********************
**  writeToConsole()  **
***********************/

#define PARAM_BUF_SIZE 10

ULONG writeToConsole(struct IOStdReq *ioreq, struct ConsoleBase *ConsoleDevice)
{
    UBYTE param_tab[PARAM_BUF_SIZE];
    BYTE command;
    UBYTE *write_str_end;
    struct ConUnit *unit = (struct ConUnit *)ioreq->io_Unit;
    UBYTE *write_str = (UBYTE *)ioreq->io_Data;
    ULONG towrite = ioreq->io_Length;
    
    EnterFunc(bug("WriteToConsole(ioreq=%p)\n"));
    
    if (towrite == -1L)
    	towrite = strlen(write_str);
    
    D(bug("Number of chars to write %d\n", towrite));
    
    write_str_end = write_str + towrite - 1;
    
    /* Interpret string into a command and execute command */
    
    while (write_str <= write_str_end)
    {
    	if (!string2command(&command, &write_str, write_str_end, param_tab, ConsoleDevice))
    	    break;
    	
	Console_DoCommand((Object *)unit, command, param_tab);
    	
    } /* while (characters left to interpret) */
    
    ReturnInt("WriteToConsole", LONG, towrite);
}


/**********************
** string2command()  **
**********************/

#define STR_SLM	"\0x32\0x30\0x68" /* Set linefeed mode    */
#define STR_RNM "\0x32\0x30\0x6C" /* Set reset mode 	  */
#define STR_DSR "\0x36\0x6E"	  /* device status report */

static const struct special_cmd_descr
{
    BYTE	Command;
    STRPTR	CommandStr;
    BYTE	Length; 
} scd_tab[] = {

    {C_SET_LF_MODE, 		STR_SLM, sizeof (STR_SLM) },
    {C_RESET_NEWLINE_MODE,	STR_RNM, sizeof (STR_RNM) },
    {C_DEVICE_STATUS_REPORT, 	STR_DSR, sizeof (STR_DSR) }

};


static BOOL string2command( BYTE 	*cmd_ptr
		, UBYTE 		**writestr_ptr
		, UBYTE 		*str_end
		, UBYTE 		*p_tab
		, struct ConsoleBase 	*ConsoleDevice)
{
    UBYTE *write_str = *writestr_ptr,
    	*csi_str = NULL;
    
    BOOL found = FALSE,
    	 csi   = FALSE;
    	 
    EnterFunc(bug("StringToCommand(cmd_ptr=%p, writestr_ptr=%p, str_end=%p, p_tab=%p)\n",
    		cmd_ptr, writestr_ptr, str_end));
    		
    D(bug("write_str: %p, %s \n", write_str, write_str));
    
    /* Look for <CSI> */
    if (*write_str == CSI)
    {

	csi_str ++;
	csi = TRUE;
    }
    else if (str_end > write_str)
    {
    	if ( (write_str[0] == ESC) && (write_str[1] == '[') )
    	{
	    csi_str += 2;
	    csi = TRUE;
    	}
    }
    
    if (csi)
    {
	/* A parameter command ? (Ie. one of the commands that takes parameters) */
	found = getparamcommand(cmd_ptr, &write_str, str_end, p_tab);
	    
	if (!found)
	{
	    BYTE i;
	    /* Look for some special commands */
	    for (i = sizeof (scd_tab) - 1; ((i >= 0) && (!found)) ; i -- )
	    {
	    	/* Check whether command sequence is longer than input */
	    	if (str_end < csi_str + scd_tab[i].Length)
	    	    continue; /* if so, check next command sequence */
	    	    
		/* Coomand match ? */    
	    	if (0 == strncmp(csi_str, scd_tab[i].CommandStr, scd_tab[i].Length))
		{
	    	    csi_str += scd_tab[i].Length;
	    	    *cmd_ptr = scd_tab[i].Command;
	    	
	    	    found = TRUE;
	    	}
	    	
	    } /* for (each special command) */
	    
	}
	
    } /* if (CSI was found) */
    
    if (found)
    	write_str = csi_str;
    else
    {
    	/* Look for standalone codes */
    	switch (*write_str)
    	{
    	case BELL:
    	    *cmd_ptr = C_BELL;
    	    found = TRUE;
    	    break;
    	    
    	case BACKSPACE:
    	    *cmd_ptr = C_BACKSPACE;
    	    found = TRUE;
    	    break;
    	    
    	case HTAB:
    	    *cmd_ptr = C_HTAB;
    	    found = TRUE;
    	    break;
    	    
    	case LINEFEED:
    	    *cmd_ptr = C_LINEFEED;
    	    found = TRUE;
    	    break;
    	    
    	case VTAB:
    	    *cmd_ptr = C_VTAB;
    	    found = TRUE;
    	    break;
    	    
    	case FORMFEED:
    	    *cmd_ptr = C_FORMFEED;
    	    found = TRUE;
    	    break;
    	    
    	case CARRIAGE_RETURN:
    	    *cmd_ptr = C_CARRIAGE_RETURN;
    	    found = TRUE;
    	    break;

    	case SHIFT_OUT:
    	    *cmd_ptr = C_SHIFT_OUT;
    	    found = TRUE;
    	    break;
    	    
    	case SHIFT_IN:
    	    *cmd_ptr = C_SHIFT_IN;
    	    found = TRUE;
    	    break;
    	    
    	case ESC:
    	    *cmd_ptr = C_ESC;
    	    found = TRUE;
    	    break;
    	    
    	case INDEX:
    	    *cmd_ptr = C_INDEX;
    	    found = TRUE;
    	    break;
    	    
    	case NEXT_LINE:
    	    *cmd_ptr = C_NEXT_LINE;
    	    found = TRUE;
    	    break;
    	    
    	case H_TAB_SET:
    	    *cmd_ptr = C_H_TAB_SET;
    	    found = TRUE;
    	    break;
    	    
    	case REVERSE_INDEX:
    	    *cmd_ptr = C_REVERSE_IDX;
    	    found = TRUE;
    	    break;

    	} /* (switch) */

    	if (found)
    	{
    	    /* Found special char. Increase pointer */
	    write_str ++;
    	}
    	
    }
    
    if (!found) /* Still not any found ? Try to print as plain ASCII */
    {
    	*cmd_ptr = C_ASCII;
    	p_tab[0] = *write_str ++;
    	
    	found = TRUE;
    }
    
    	
    
    /* Return pointer to first character AFTER last interpreted char */
    *writestr_ptr = write_str;
    
    ReturnBool ("WriteToConsole", found);
}


/************************
**  getparamcommand()  **
************************/

static const struct Command
{
    BYTE Command;
    UBYTE MaxParams;
    
} csi2command[] = {

    { C_INSERT_CHAR,		1 },	/* 0x40 */
    { C_CURSOR_UP,		1 },	/* 0x41 */
    { C_CURSOR_DOWN,		1 },	/* 0x41 */
    { C_CURSOR_FORWARD,		1 },	/* 0x43	*/
    { C_CURSOR_BACKWARD,	1 },	/* 0x44 */
    { C_CURSOR_NEXT_LINE,	1 },	/* 0x45 */
    { C_CURSOR_PREV_LINE,	1 },	/* 0x46 */
    { -1, },
    { C_CURSOR_POS,		2 },	/* 0x48 */
    { C_CURSOR_HTAB,		1 },	/* 0x49 */
    
    { C_ERASE_IN_DISPLAY,	0 },	/* 0x4A	*/
    { C_ERASE_IN_LINE,		0 },	/* 0x4B */
    { C_INSERT_LINE,		0 },	/* 0x4C */
    { C_DELETE_LINE,		0 },	/* 0x4D */
    { -1, },
    { -1, },
    { C_DELETE_CHAR,		1 },	/* 0x50 */
    { -1, },
    { -1, },
    { C_SCROLL_UP,		1 },	/* 0x53 */
    { C_SCROLL_DOWN,		1 },	/* 0x54 */
    { -1, },
    { -1, },
    { C_CURSOR_TAB_CTRL,	1 },	/* 0x57	*/
    { -1, },
    { -1, }, 
    { C_CURSOR_BACKTAB,		1 }	/* 0x5A	*/
};


static BOOL getparamcommand(BYTE *cmd_ptr, UBYTE ** writestr_ptr, UBYTE *str_end, UBYTE *p_tab)
{
    /* This function checks for a command with parameters in
    ** the string. The problem is that the parameters come
    ** before the comand ID, and the parameters are optional.
    ** This means that a parameter which has the same value
    ** as a command ID may be mistakenly taken for being
    ** end of the command. Therefore we must continue scanning
    ** even if we found a command ID.
    */
    
    #define MAX_COMMAND_PARAMS 4
    
    UBYTE p_tab_idx = 0;
    
    BYTE cmd = -1;
    BYTE cmd_next_idx = 0; /* Index to byte after the command */
    
    /* write_str points to first character after <CSI> */
    UBYTE *write_str = *writestr_ptr;

    UBYTE num_params = 0;
    
    BOOL done  = FALSE,
    	 found = FALSE;
    
    while (!done)
    {
    	/* In case it's a parameter */
    	p_tab[p_tab_idx] = *write_str;
    	
    	switch (*write_str)
    	{
    	case 0x40:
     	case 0x41:
    	case 0x42:
    	case 0x43:
    	case 0x44:
    	case 0x45:
    	case 0x46:
    	case 0x48:
    	case 0x49:
    	case 0x50:
    	case 0x53:
    	case 0x54:
    	case 0x57:
    	case 0x5A: {
    	    UBYTE idx = *write_str - FIRST_CSI_CMD;
    	    
    	    if (num_params <= csi2command[idx].MaxParams) /* Valid command ? */
    	    {
    	    	cmd = csi2command[idx].Command;
    	    	cmd_next_idx = write_str - *writestr_ptr;
    	    }
    	    
    	    /* Could be a parameter for another command */
    	    num_params ++;
    	    p_tab[p_tab_idx ++] = *write_str;
    	} break;
    	    
    	case ';': /* parameter separator */
    	case '>': /* Used in SGR command */
    	    break;
    	    
    	default:
    	    p_tab[p_tab_idx ++] = *write_str;
    	    break;

    	} /* switch */

    	if (num_params > MAX_COMMAND_PARAMS)
    	{
    	    done = TRUE;
    	}
    	
    	write_str ++;
    	
    	if (write_str > str_end)
    	    done = TRUE;

    } /* while (!done) */
    
    if (cmd != -1)
    {
    	*cmd_ptr = cmd;
    	found = TRUE;
	
	/* Continue parsing on the first byte after the command */
    	*writestr_ptr += cmd_next_idx;
    }
    return (found);
}


/**************************
**  CreateConsoleTask()  **
**************************/
struct Task *createConsoleTask(APTR taskparams, struct ConsoleBase *ConsoleDevice)
{
    struct Task *task;
    APTR stack;
    
    task = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (task)
    {
    	NEWLIST(&task->tc_MemEntry);
    	task->tc_Node.ln_Type = NT_TASK;
    	task->tc_Node.ln_Name = "console.device";
    	task->tc_Node.ln_Pri  = COTASK_PRIORITY;

    	stack=AllocMem(COTASK_STACKSIZE, MEMF_PUBLIC);
    	if(stack != NULL)
    	{
	    task->tc_SPLower=stack;
	    task->tc_SPUpper=(BYTE *)stack + COTASK_STACKSIZE;

#if AROS_STACK_GROWS_DOWNWARDS
	    task->tc_SPReg = (BYTE *)task->tc_SPUpper-SP_OFFSET - sizeof(APTR);
	    ((APTR *)task->tc_SPUpper)[-1] = taskparams;
#else
	    task->tc_SPReg=(BYTE *)task->tc_SPLower-SP_OFFSET + sizeof(APTR);
	    *(APTR *)task->tc_SPLower = taskparams;
#endif

	    if(AddTask(task, consoleTaskEntry, NULL) != NULL)
	    {
	    	return (task);
	    }	
	    FreeMem(stack, COTASK_STACKSIZE);
    	}
        FreeMem(task,sizeof(struct Task));
    }
    return (NULL);

}




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
#include <intuition/classes.h>

#include <devices/conunit.h>
#include <string.h>
#include <stdio.h>

#include "console_gcc.h"

#include "consoleif.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

static BOOL getparamcommand(BYTE *cmd_ptr, UBYTE **writestr_ptr, LONG toparse, UBYTE *p_tab, Object *unit, struct ConsoleBase *ConsoleDevice);
static BOOL string2command(BYTE *cmd_ptr, UBYTE **writestr_ptr, LONG toparse, UBYTE *p_tab, Object *unit, struct ConsoleBase *ConsoleDevice);

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

/* SGR is the command with most params: 4 */
#define MAX_COMMAND_PARAMS 4


ULONG writeToConsole(struct ConUnit *unit, STRPTR buf, ULONG towrite, struct ConsoleBase *ConsoleDevice)
{
    UBYTE param_tab[MAX_COMMAND_PARAMS];
    
    BYTE command;
    UBYTE *orig_write_str, *write_str;
    LONG written, orig_towrite;

    write_str = orig_write_str = (UBYTE *)buf;
    
    
    EnterFunc(bug("WriteToConsole(ioreq=%p)\n"));
    
	
    orig_towrite = towrite;
    
    D(bug("Number of chars to write %d\n", towrite));
    
    
    /* Interpret string into a command and execute command */
    
    /* DEBUG aid */

#if DEBUG
{
    UWORD i;    
    for (i = 0; i < towrite; i ++)
    	kprintf("%x", write_str[i]);
	
    kprintf("\n");
    
}    
#endif
    
    while (towrite > 0)
    {
    	if (!string2command(&command, &write_str, towrite, param_tab, (Object *)unit, ConsoleDevice))
    	    break;
    	
	
	Console_DoCommand((Object *)unit, command, param_tab);
	
	towrite = orig_towrite - (write_str - orig_write_str);
    	
    } /* while (characters left to interpret) */
    
    written = write_str - orig_write_str;
    
    ReturnInt("WriteToConsole", LONG, written);
}


/**********************
** string2command()  **
**********************/


static const UBYTE str_slm[] = {0x32, 0x30, 0x68 }; /* Set linefeed mode    */
static const UBYTE str_rnm[] = {0x32, 0x30, 0x6C }; /* Reset newline mode   */
static const UBYTE str_dsr[] = {0x36, 0x6E };       /* device status report */
static const UBYTE str_con[] = {' ', 'p'};    	    /* cursor visible */
static const UBYTE str_cof[] = {'0', ' ', 'p'};     /* cursor invisible */

#define NUM_SPECIAL_COMMANDS 5
static const struct special_cmd_descr
{
    BYTE	Command;
    STRPTR	CommandStr;
    BYTE	Length; 
} scd_tab[NUM_SPECIAL_COMMANDS] = {

    {C_SET_LF_MODE, 		(STRPTR)str_slm, 3 },
    {C_RESET_NEWLINE_MODE,	(STRPTR)str_rnm, 3 },
    {C_DEVICE_STATUS_REPORT, 	(STRPTR)str_dsr, 2 },
    {C_CURSOR_VISIBLE,		(STRPTR)str_con, 2 },
    {C_CURSOR_INVISIBLE,	(STRPTR)str_cof, 3 }

};

#if DEBUG
static UBYTE *cmd_names[NUM_CONSOLE_COMMANDS] =
{
	
    "Ascii",		/* C_ASCII = 0	*/
    
    "Esc",		/* C_ESC	*/
    "Bell",		/* C_BELL,	*/
    "Backspace",	/* C_BACKSPACE,	*/
    "HTab",		/* C_HTAB,	*/
    "Linefeed",		/* C_LINEFEED,	*/
    "VTab",		/* C_VTAB,	*/
    "Formefeed",	/* C_FORMFEED,	*/
    "Carriage return",	/* C_CARRIAGE_RETURN,	*/
    "Shift In",		/* C_SHIFT_IN,	*/
    "Shift Out",	/* C_SHIFT_OUT,	*/
    "Index",		/* C_INDEX,	*/
    "Nex Line",		/* C_NEXT_LINE,	*/
    "Tab set",		/* C_H_TAB_SET, */
    "Reverse Idx",	/* C_REVERSE_IDX, */
    "Set LF Mode",	/* C_SET_LF_MODE, */
    "Reset Newline Mode",	/* C_RESET_NEWLINE_MODE,	*/
    "Device Status Report",	/* C_DEVICE_STATUS_REPORT,	*/
    
    "Insert Char",	/* C_INSERT_CHAR, */
    "Cursor Up",	/* C_CURSOR_UP,		*/
    "Cursor Down",	/* C_CURSOR_DOWN,	*/
    "Cursor Forward",	/* C_CURSOR_FORWARD,	*/
    "Cursor Backward",	/* C_CURSOR_BACKWARD,	*/
    "Cursor Next Line",	/* C_CURSOR_NEXT_LINE,	*/
    "Cursor Prev Line",	/* C_CURSOR_PREV_LINE,	*/
    "Cursor Pos",	/* C_CURSOR_POS,	*/
    "Cursor HTab",	/* C_CURSOR_HTAB,	*/
    "Erase In Display",	/* C_ERASE_IN_DISPLAY,	*/
    "Erase In Line",	/* C_ERASE_IN_LINE,	*/
    "Insert Line",	/* C_INSERT_LINE,	*/
    "Delete Line",	/* C_DELETE_LINE,	*/
    "Delete Char",	/* C_DELETE_CHAR,	*/
    "Scroll Up",	/* C_SCROLL_UP,		*/
    "Scroll Down",	/* C_SCROLL_DOWN,	*/
    "Cursor Tab Ctrl",	/*C_CURSOR_TAB_CTRL,	*/
    "Cursor Backtab",	/* C_CURSOR_BACKTAB	*/
    "Select Graphic Rendation",
    "Cursor Visible",	/* C_CURSOR_VISIBLE     */
    "Cursor Invisible"	/* C_CURSOR_INVISIBLE   */
};
#endif

static BOOL string2command( BYTE 	*cmd_ptr
		, UBYTE 		**writestr_ptr
		, LONG			toparse
		, UBYTE 		*p_tab
		, Object		*unit
		, struct ConsoleBase 	*ConsoleDevice)
{
    UBYTE *write_str = *writestr_ptr;

    UBYTE *csi_str = write_str;
    LONG csi_toparse;
    
    BOOL found = FALSE,
    	 csi   = FALSE;

    	 
    EnterFunc(bug("StringToCommand(toparse=%d)\n", toparse));
    		
    
    /* Look for <CSI> */
    if (*write_str == CSI)
    {

	csi_str ++;
	csi = TRUE;
	csi_toparse = toparse - 1;
    }
    else if (toparse >= 2)
    {
    	if ( (write_str[0] == ESC) && (write_str[1] == '[') )
    	{
	    csi_str += 2;
	    csi_toparse = toparse - 2;
	    csi = TRUE;
    	}
    }
    
    if (csi)
    {
        D(bug("CSI found, getting command\n"));
	
	/* Search for the longest commands first */

#define SGR_COMMAND_LEN	9
	
	/* SGR needs special handling because of the '>' separator */
	if (csi_toparse >= SGR_COMMAND_LEN)
	{
	    if (    ( csi_str[1] == ';' )
		 && ((csi_str[2] & 0x30) == 0x30)
		 && ( csi_str[3] == ';' )
		 && ((csi_str[4] & 0x40) == 0x40)
		 && ( csi_str[5] == ';')
		 && ( csi_str[6] == '>')
		 && ( csi_str[8] == 0x6D) )
	    {
	    	p_tab[0] = csi_str[0];
		p_tab[1] = csi_str[2] & 0x0F;
		p_tab[2] = csi_str[4] & 0x0F;
		p_tab[3] = csi_str[7];
		
		*cmd_ptr = C_SELECT_GRAPHIC_RENDATION;
		
		found = TRUE;
	    }
	}
	
	if (!found)
	{
	    BYTE i;
	    /* Look for some special commands */
	    for (i = 0; ((i < NUM_SPECIAL_COMMANDS) && (!found)) ; i ++ )
	    {
	    	/* Check whether command sequence is longer than input */
	    	if (scd_tab[i].Length > csi_toparse)
	    	    continue; /* if so, check next command sequence */
	    	
		D(bug("Comparing for special command %d, idx %d, cmdstr %p, len %d, csistr %p \n", 
			scd_tab[i].Command, i, scd_tab[i].CommandStr, scd_tab[i].Length, csi_str));
 		/* Command match ? */    
	    	if (0 == strncmp(csi_str, scd_tab[i].CommandStr, scd_tab[i].Length))
		{
		    D(bug("Special command found\n"));
	    	    csi_str += scd_tab[i].Length;
	    	    *cmd_ptr = scd_tab[i].Command;
	    	
	    	    found = TRUE;
	    	}
	    	
	    } /* for (each special command) */
	    
	}

	/* A parameter command ? (Ie. one of the commands that takes parameters) */
	if (!found)
	    found = getparamcommand(cmd_ptr, &csi_str, csi_toparse, p_tab, unit, ConsoleDevice);
	
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

    D(bug("FOUND CMD: %s\n", cmd_names[*cmd_ptr]));    
    
    /* Return pointer to first character AFTER last interpreted char */
    *writestr_ptr = write_str;
    
    ReturnBool ("StringToCommand", found);
}


/************************
**  getparamcommand()  **
************************/

/* !!! IMPORTANT !!!
   If you add a command her, you should also add default values for
   its parameters in Console::GetDefaultParams()
*/
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


#define PARAM_BUF_SIZE 2
/* Parameters for commands are parsed and filled into this one */
struct cmd_params
{
    UBYTE numparams; /* Parameters stored */
    
    /* Since parameters may be optional, only supplied parameters
    are saved, along with their number. For example
    for the command CURSOR POSITION, if only the sencond parameter
    (column) is specified in the write stream, then
    numparams will be 1 and for the one entry, paramno will be 1 (C counting)
    and val will be <column>.
    Row will have to be set to some default value.
    */
    struct cmd_param
    {
    	UBYTE paramno; /* Starts counting at 0 */
	UBYTE val;
    } tab[PARAM_BUF_SIZE];
};


static BOOL getparamcommand(BYTE 	*cmd_ptr
		, UBYTE 		**writestr_ptr
		, LONG 			toparse
		, UBYTE 		*p_tab
		, Object 		*unit
		, struct ConsoleBase 	*ConsoleDevice)
{
    /* This function checks for a command with parameters in
    ** the string. The problem is that the parameters come
    ** before the comand ID, and the parameters are optional.
    ** This means that a parameter which has the same value
    ** as a command ID may be mistakenly taken for being
    ** end of the command. Therefore we must continue scanning
    ** even if we found a command ID.
    */
    
    struct cmd_params params;
        
    BYTE cmd = -1;
    BYTE cmd_next_idx = 0; /* Index to byte after the command */
    
    /* write_str points to first character after <CSI> */
    UBYTE *write_str = *writestr_ptr;

    UBYTE num_params = 0;
    
    BOOL done  = FALSE,
    	 found = FALSE;
	 
    BOOL next_can_be_separator = TRUE,
    	next_can_be_param = TRUE,
	next_can_be_commandid = TRUE,
	last_was_param = FALSE;

    UBYTE num_separators_found = 0;
       
    while (!done)
    {
    	/* In case it's a parameter */
	
	if (toparse <= 0)
    	    done = TRUE;

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
	
	case 0x4A:
	case 0x4B:
	case 0x4C:
	case 0x4D:
	
    	case 0x50:
    	case 0x53:
    	case 0x54:
    	case 0x57:
    	case 0x5A: {
    	    UBYTE idx = *write_str - FIRST_CSI_CMD;
	    UBYTE maxparams = csi2command[idx].MaxParams;
	    
	    if (next_can_be_commandid)
	    {
#warning Should also do a MinParams compare    	    
		if (num_params <= maxparams) /* Valid command ? */
		{

		    /* Assure that there are not to many separators in a command  */
		    if ((num_separators_found < maxparams)
#warning 0-param commands can be moved to special-command-handlin in string2command()
		       || ((num_separators_found == 0) && (maxparams == 0)))
		    {
			cmd = csi2command[idx].Command;
		
			/* Save index to where the next command will start */
			cmd_next_idx = write_str - *writestr_ptr + 1;
		
			params.numparams = num_params;
		    }
		}
    	    }
	    
	    done = TRUE;
    	    

    	} break;
    	    
    	case ';': /* parameter separator, skip it */

	    if (!next_can_be_separator)
	    {
	        /* Error */
	    	done = TRUE;
		break;
	    }
	    
	    next_can_be_separator = FALSE;
	    next_can_be_param = TRUE;
	    next_can_be_commandid = FALSE;
	    last_was_param = FALSE;
	    
	    num_separators_found ++;
	    
    	    break;
    	
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    if (!next_can_be_param)
	    {
	    	/* Error */
		done = TRUE;
		break;
	    }

	    if (!last_was_param)
	    {
		num_params++;
		if (num_params > MAX_COMMAND_PARAMS)
		{
		    done = TRUE;
		    break;
		}
		params.tab[num_params - 1].paramno = num_params - 1;
		params.tab[num_params - 1].val = 0;
		
		last_was_param = TRUE;
	     }

    	    params.tab[num_params - 1].val *= 10;
	    params.tab[num_params - 1].val += (*write_str) - '0';

	    next_can_be_separator = TRUE;
	    next_can_be_commandid = TRUE;
	    break;
	    
    	default:
	    /* Error */
	    done = TRUE;
    	    break;

    	} /* switch */

    	
    	write_str ++;
    	

    } /* while (!done) */
    
    if (cmd != -1)
    {
    	*cmd_ptr = cmd;
    	found = TRUE;
	
	/* Continue parsing on the first byte after the command */
    	*writestr_ptr += cmd_next_idx;
    }
        
    if (found)
    {
        UBYTE i;
    	/* First fill in some default values in p_tab */
	Console_GetDefaultParams(unit, *cmd_ptr, p_tab);
	
	for (i = 0; i < params.numparams; i ++)
	{
	    /* Override with parsed values */
	    D(bug("CMD %s: Setting param %d to %d\n"
	    	, cmd_names[*cmd_ptr]
		, params.tab[i].paramno
		, params.tab[i].val));
		
	    p_tab[params.tab[i].paramno] = params.tab[i].val;
	}
    }
    
    return found;
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

VOID printstring(STRPTR string, ULONG len, struct ConsoleBase *ConsoleDevice)
{
    while (len --)
    {
    	kprintf("%d/%c ", *string, *string);
	string ++;
    }
    
    kprintf("\n");
}

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Support functions for console.device
    Lang: english
*/

#include <proto/exec.h>
#include <exec/lists.h>
#include <exec/io.h>
#include <exec/memory.h>

#include <proto/intuition.h>
#include <intuition/classes.h>

#include <devices/conunit.h>
#include <string.h>
#include <stdio.h>

#include "console_gcc.h"

#include "consoleif.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

static BOOL getparamcommand(BYTE *cmd_ptr, UBYTE **writestr_ptr, UBYTE *numparams_ptr, LONG toparse, IPTR *p_tab, Object *unit, struct ConsoleBase *ConsoleDevice);
static BOOL string2command(BYTE *cmd_ptr, UBYTE **writestr_ptr, UBYTE *numparams_ptr, LONG toparse, IPTR *p_tab, Object *unit, struct ConsoleBase *ConsoleDevice);

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

/*
** SGR is the command with most params: 4
**   stegerg: RKRMs say it can have any number of parameters in any order. So instead of 4
**            we assume and hope that there will never be more than 16 params :-\
*/

#define MAX_COMMAND_PARAMS 16


ULONG writeToConsole(struct ConUnit *unit, STRPTR buf, ULONG towrite, struct ConsoleBase *ConsoleDevice)
{
    IPTR param_tab[MAX_COMMAND_PARAMS];

    BYTE command;
    UBYTE numparams;
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
        numparams = 0;

    	if (!string2command(&command, &write_str, &numparams, towrite, param_tab, (Object *)unit, ConsoleDevice))
    	    break;


	Console_DoCommand((Object *)unit, command, numparams, param_tab);

	towrite = orig_towrite - (write_str - orig_write_str);

    } /* while (characters left to interpret) */

    written = write_str - orig_write_str;

    ReturnInt("WriteToConsole", LONG, written);
}


/**********************
** string2command()  **
**********************/


static const UBYTE str_slm[] = {0x32, 0x30, 0x68 }; /* Set linefeed mode    */
static const UBYTE str_rnm[] = {0x32, 0x30, 0x6C }; /* Reset linefeed mode   */
static const UBYTE str_ssm[] = {0x3E, 0x31, 0x68 }; /* Set autoscroll mode */
static const UBYTE str_rsm[] = {0x3E, 0x31, 0x6C }; /* Reset autoscroll mode */
static const UBYTE str_swm[] = {0x3E, 0x37, 0x68 }; /* Set autowrap mode */
static const UBYTE str_rwm[] = {0x3E, 0x37, 0x6C }; /* Reset autowrap mode */
static const UBYTE str_dsr[] = {0x36, 0x6E };       /* device status report */
static const UBYTE str_con[] = {' ', 'p'};    	    /* cursor visible */
static const UBYTE str_con2[] = {'1', ' ', 'p'};    /* cursor visible */
static const UBYTE str_cof[] = {'0', ' ', 'p'};     /* cursor invisible */

#define NUM_SPECIAL_COMMANDS 10
static const struct special_cmd_descr
{
    BYTE	Command;
    STRPTR	CommandStr;
    BYTE	Length;
} scd_tab[NUM_SPECIAL_COMMANDS] = {

    {C_SET_LF_MODE, 		(STRPTR)str_slm, 3 },
    {C_RESET_LF_MODE,	    	(STRPTR)str_rnm, 3 },
    {C_SET_AUTOSCROLL_MODE, 	(STRPTR)str_ssm, 3 },
    {C_RESET_AUTOSCROLL_MODE,	(STRPTR)str_rsm, 3 },
    {C_SET_AUTOWRAP_MODE,   	(STRPTR)str_swm, 3 },
    {C_RESET_AUTOWRAP_MODE, 	(STRPTR)str_rwm, 3 },
    {C_DEVICE_STATUS_REPORT, 	(STRPTR)str_dsr, 2 },
    {C_CURSOR_VISIBLE,		(STRPTR)str_con, 2 },
    {C_CURSOR_VISIBLE,          (STRPTR)str_con2, 3 },
    {C_CURSOR_INVISIBLE,	(STRPTR)str_cof, 3 }

};

#if DEBUG
static UBYTE *cmd_names[NUM_CONSOLE_COMMANDS] =
{

    "Ascii",			/* C_ASCII = 0	    	    	*/

    "Esc",			/* C_ESC	    	    	*/
    "Bell",			/* C_BELL,	    	    	*/
    "Backspace",		/* C_BACKSPACE,	    	    	*/
    "HTab",			/* C_HTAB,	    	    	*/
    "Linefeed",			/* C_LINEFEED,	    	    	*/
    "VTab",			/* C_VTAB,	    	    	*/
    "Formefeed",		/* C_FORMFEED,	    	    	*/
    "Carriage return",		/* C_CARRIAGE_RETURN,	    	*/
    "Shift In",			/* C_SHIFT_IN,	    	    	*/
    "Shift Out",		/* C_SHIFT_OUT,	    	    	*/
    "Index",			/* C_INDEX,	    	    	*/
    "Nex Line",			/* C_NEXT_LINE,	    	    	*/
    "Tab set",			/* C_H_TAB_SET,     	    	*/
    "Reverse Idx",		/* C_REVERSE_IDX,   	    	*/
    "Set LF Mode",		/* C_SET_LF_MODE,   	    	*/
    "Reset LF Mode",	    	/* C_RESET_lF_MODE,    	    	*/
    "Device Status Report",	/* C_DEVICE_STATUS_REPORT,  	*/

    "Insert Char",		/* C_INSERT_CHAR,   	    	*/
    "Cursor Up",		/* C_CURSOR_UP,		    	*/
    "Cursor Down",		/* C_CURSOR_DOWN,	    	*/
    "Cursor Forward",		/* C_CURSOR_FORWARD,	    	*/
    "Cursor Backward",		/* C_CURSOR_BACKWARD,	    	*/
    "Cursor Next Line",		/* C_CURSOR_NEXT_LINE,	    	*/
    "Cursor Prev Line",		/* C_CURSOR_PREV_LINE,	    	*/
    "Cursor Pos",		/* C_CURSOR_POS,	    	*/
    "Cursor HTab",		/* C_CURSOR_HTAB,	    	*/
    "Erase In Display",		/* C_ERASE_IN_DISPLAY,	    	*/
    "Erase In Line",		/* C_ERASE_IN_LINE,	    	*/
    "Insert Line",		/* C_INSERT_LINE,	    	*/
    "Delete Line",		/* C_DELETE_LINE,	    	*/
    "Delete Char",		/* C_DELETE_CHAR,	    	*/
    "Scroll Up",		/* C_SCROLL_UP,		    	*/
    "Scroll Down",		/* C_SCROLL_DOWN,	    	*/
    "Cursor Tab Ctrl",		/* C_CURSOR_TAB_CTRL,	    	*/
    "Cursor Backtab",		/* C_CURSOR_BACKTAB,	    	*/
    "Select Graphic Rendition", /* C_SELECT_GRAPHIC_RENDITION 	*/
    "Cursor Visible",		/* C_CURSOR_VISIBLE,        	*/
    "Cursor Invisible",		/* C_CURSOR_INVISIBLE,      	*/
    "Set Raw Events",	    	/* C_SET_RAWEVENTS, 	    	*/
    "Reset Raw Events",     	/* C_RESET_RAWEVENTS	    	*/
    "Set Auto Wrap Mode",   	/* C_SET_AUTOWRAP_MODE	    	*/
    "Reset Auto Wrap Mode", 	/* C_RESET_AUTOWRAP_MODE    	*/
    "Set Auto Scroll Mode", 	/* C_SET_AUTOSCROLL_MODE    	*/
    "Reset Auto Scroll Mode",	/* C_RESET_AUTOSCROLL_MODE  	*/
    "Set Page Length",	    	/* C_SET_PAGE_LENGTH	    	*/
    "Set Line Length",	    	/* C_SET_LINE_LENGTH	    	*/
    "Set Left Offset",	    	/* C_SET_LEFT_OFFSET	    	*/
    "Set Top Offset"	    	/* C_SET_TOP_OFFSET 	    	*/
};
#endif

static BOOL check_special(STRPTR string, LONG toparse)
{
    return
    (
        (*string == CSI) || (toparse >= 2 && (string[0] == ESC) && (string[1] == '[')) ||  /* CSI */
    	(*string == BELL)            ||
    	(*string == BACKSPACE)       ||
    	(*string == HTAB)            ||
    	(*string == LINEFEED)        ||
    	(*string == FORMFEED)        ||
    	(*string == CARRIAGE_RETURN) ||
    	(*string == SHIFT_OUT)       ||
    	(*string == SHIFT_IN)        ||
    	(*string == ESC)             ||
    	(*string == INDEX)           ||
    	(*string == H_TAB_SET)       ||
    	(*string == REVERSE_INDEX)
    );
}

static BOOL string2command( BYTE 	*cmd_ptr
		, UBYTE 		**writestr_ptr
		, UBYTE			*numparams_ptr
		, LONG			toparse
		, IPTR    		*p_tab
		, Object		*unit
		, struct ConsoleBase 	*ConsoleDevice)
{
    UBYTE *write_str = *writestr_ptr;

    UBYTE *csi_str = write_str;
    LONG csi_toparse = 0;

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
	    found = getparamcommand(cmd_ptr, &csi_str, numparams_ptr, csi_toparse, p_tab, unit, ConsoleDevice);

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
        *cmd_ptr = C_ASCII_STRING;

	p_tab[0] = (IPTR)write_str;
    	*numparams_ptr = 2;
    	found = TRUE;

	do
	{
	    toparse--;
	    write_str++;
	} while (toparse && !check_special(write_str, toparse));

	p_tab[1] = (IPTR)(write_str - (UBYTE *)p_tab[0]); /* store the string length */
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
   If you add a command here, you should also add default values for
   its parameters in Console::GetDefaultParams()
*/
static const struct Command
{
    BYTE Command;
    UBYTE MaxParams;

} csi2command[] = {

    { C_INSERT_CHAR		, 1 			},	/* 0x40 @ */
    { C_CURSOR_UP		, 1 			},	/* 0x41 A */
    { C_CURSOR_DOWN		, 1 			},	/* 0x42 B */
    { C_CURSOR_FORWARD		, 1 			},	/* 0x43	C */
    { C_CURSOR_BACKWARD		, 1 			},	/* 0x44 D */
    { C_CURSOR_NEXT_LINE	, 1 			},	/* 0x45 E */
    { C_CURSOR_PREV_LINE	, 1 			},	/* 0x46 F */
    { -1			, 			},  	/* 0x47 G */
    { C_CURSOR_POS		, 2 			},	/* 0x48 H */
    { C_CURSOR_HTAB		, 1			},	/* 0x49 I */
    
    { C_ERASE_IN_DISPLAY	, 0			},	/* 0x4A	J */
    { C_ERASE_IN_LINE		, 0			},	/* 0x4B K */
    { C_INSERT_LINE		, 0			},	/* 0x4C L */
    { C_DELETE_LINE		, 0 			},	/* 0x4D M */
    { -1			, 			},  	/* 0x4E N */
    { -1			, 			},  	/* 0x4F O */
    { C_DELETE_CHAR		, 1 			},	/* 0x50 P */
    { -1			,			},  	/* 0x51 Q */
    { -1			,			},  	/* 0x52 R */
    { C_SCROLL_UP		, 1			},	/* 0x53 S */
    { C_SCROLL_DOWN		, 1 			},	/* 0x54 T */
    { -1			, 			},  	/* 0x55 U */
    { -1			, 			},  	/* 0x56 V */
    { C_CURSOR_TAB_CTRL		, 1 			},	/* 0x57	W */
    { -1			, 			},  	/* 0x58 X */
    { -1			, 			},  	/* 0x59 Y */
    { C_CURSOR_BACKTAB		, 1 			},	/* 0x5A	Z */
    { -1			, 			},	/* 0x5B	[ */
    { -1			, 			},	/* 0x5C	\ */
    { -1			, 			},	/* 0x5D	] */
    { -1			, 			},	/* 0x5E	^ */
    { -1			, 			},	/* 0x5F	_ */
    { -1			, 			},	/* 0x60	` */
    { -1			, 			},	/* 0x61	a */
    { -1			, 			},	/* 0x62	b */
    { -1			, 			},	/* 0x63	c */
    { -1			, 			},	/* 0x64	d */
    { -1			, 			},	/* 0x65	e */
    { -1			, 			},	/* 0x66	f */
    { -1			, 			},	/* 0x67	g */
    { -1			, 			},	/* 0x68	h */
    { -1			, 			},	/* 0x69	i */
    { -1			, 			},	/* 0x6A	j */
    { -1			, 			},	/* 0x6B	k */
    { -1			, 			},	/* 0x6C	l */
    { C_SELECT_GRAPHIC_RENDITION, MAX_COMMAND_PARAMS	},	/* 0x6D m */
    { -1    	    	    	,   	    	    	},  	/* 0x6E n */
    { -1    	    	    	,   	    	    	},  	/* 0x6F o */
    { -1    	    	    	,   	    	    	},  	/* 0x70 p */
    { -1    	    	    	,   	    	    	},  	/* 0x71 q */
    { -1    	    	    	,   	    	    	},  	/* 0x72 r */
    { -1    	    	    	,   	    	    	},  	/* 0x73 s */
    { C_SET_PAGE_LENGTH	    	,   	    	    	},  	/* 0x74 t */
    { C_SET_LINE_LENGTH    	,   	    	    	},  	/* 0x75 u */
    { -1    	    	    	,   	    	    	},  	/* 0x76 v */
    { -1    	    	    	,   	    	    	},  	/* 0x77 w */
    { C_SET_LEFT_OFFSET	    	,   	    	    	},  	/* 0x78 x */
    { C_SET_TOP_OFFSET 	    	,   	    	    	},  	/* 0x79 y */
    { -1    	    	    	,   	    	    	},  	/* 0x7A z */
    { C_SET_RAWEVENTS  	    	, MAX_COMMAND_PARAMS   	},  	/* 0x7B { */
    { -1    	    	    	,   	    	    	},  	/* 0x7C | */
    { C_RESET_RAWEVENTS    	, MAX_COMMAND_PARAMS   	},  	/* 0x7D } */
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
		, UBYTE			*numparams_ptr
		, LONG 			toparse
		, IPTR  		*p_tab
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
	{
    	    done = TRUE;
	    break;
	}

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
    	case 0x5A:
	case 0x6D:
	case 0x74:
	case 0x75:
	case 0x78:
	case 0x79:
	case 0x7B:
	case 0x7D:
	{
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

    	break;
    	}

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
	case '>': /* because of SGR background color param :-( */
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
	    if (*write_str == '>')
	    {
	        params.tab[num_params - 1].val += 5;
	    } else {
	        params.tab[num_params - 1].val += (*write_str) - '0';
	    }
	    
	    next_can_be_separator = TRUE;
	    next_can_be_commandid = TRUE;
	    break;
	    
    	default:
	    /* Error */
	    done = TRUE;
    	    break;

    	} /* switch */

    	
    	write_str ++;
    	toparse --;

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
	
	*numparams_ptr = params.numparams;
    }
    
    return found;
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

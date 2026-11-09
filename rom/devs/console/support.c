/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Support functions for console.device
*/

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

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

static BOOL getparamcommand(BYTE *cmd_ptr, UBYTE ** writestr_ptr,
    UBYTE *numparams_ptr, LONG toparse, IPTR *p_tab, Object *unit,
    struct ConsoleBase *ConsoleDevice);
static BOOL string2command(BYTE *cmd_ptr, UBYTE ** writestr_ptr,
    UBYTE *numparams_ptr, LONG toparse, IPTR *p_tab, Object *unit,
    struct ConsoleBase *ConsoleDevice);

#define ESC 0x1B
#define CSI 0x9B

#define NIL             0x00
#define BELL            0x07
#define BACKSPACE       0x08
#define HTAB            0x09
#define LINEFEED        0x0A
#define VTAB            0x0B
#define FORMFEED        0x0C
#define CARRIAGE_RETURN 0x0D
#define SHIFT_OUT       0x0E
#define SHIFT_IN        0x0F
#define INDEX           0x84
#define NEXT_LINE       0x85
#define H_TAB_SET       0x88
#define REVERSE_INDEX   0x8D


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


/* An escape sequence runs <CSI> params(0x30-0x3F) intermediates(0x20-0x2F)
   final(0x40-0x7E). It is unterminated while only parameter or intermediate
   bytes have arrived, so it must not be interpreted or printed yet. */
static BOOL csi_incomplete(const UBYTE *buf, LONG len)
{
    const UBYTE *p;
    LONG rem, i;

    if (len >= 1 && buf[0] == CSI)
    {
        p = buf + 1;
        rem = len - 1;
    }
    else if (len == 1 && buf[0] == ESC)
        return TRUE;
    else if (len >= 2 && buf[0] == ESC && buf[1] == '[')
    {
        p = buf + 2;
        rem = len - 2;
    }
    else
        return FALSE;

    for (i = 0; i < rem; i++)
    {
        if (p[i] < 0x20 || p[i] > 0x3F)
            return FALSE;
    }
    return TRUE;
}


ULONG writeToConsole(struct ConUnit *unit, STRPTR buf, ULONG towrite,
    struct ConsoleBase *ConsoleDevice)
{
    IPTR param_tab[MAX_COMMAND_PARAMS];
    struct intConUnit *icu = ICU(unit);

    BYTE command = 0;
    UBYTE numparams;
    UBYTE *orig_write_str, *write_str;
    LONG orig_towrite;
    UBYTE *allocbuf = NULL;
    ULONG accepted = towrite;
    BOOL plainText = TRUE;

    EnterFunc(bug("WriteToConsole(ioreq=%p)\n"));

    if (icu->pendingCSILen > 0)
    {
        UWORD pl = icu->pendingCSILen;
        allocbuf = AllocMem(pl + towrite, MEMF_ANY);
        if (!allocbuf)
            ReturnInt("WriteToConsole", LONG, 0);

        icu->pendingCSILen = 0;
        CopyMem(icu->pendingCSI, allocbuf, pl);
        CopyMem(buf, allocbuf + pl, towrite);
        buf = (STRPTR) allocbuf;
        towrite += pl;
    }

    write_str = orig_write_str = (UBYTE *) buf;
    orig_towrite = towrite;

    {
        ULONG i;

        for (i = 0; i < towrite; i++)
        {
            if ((UBYTE)buf[i] < 0x20 ||
                ((UBYTE)buf[i] >= 0x7f && (UBYTE)buf[i] < 0xa0))
            {
                plainText = FALSE;
                break;
            }
        }
    }

    D(bug("Number of chars to write %d\n", towrite));

#if DEBUG
    {
        UWORD i;
        for (i = 0; i < towrite; i++)
            bug("%x", write_str[i]);

        bug("\n");

    }
#endif
    if (towrite > 0 && !plainText)
        Console_UnRenderCursor((Object *) unit);

    while (towrite > 0)
    {
        if (csi_incomplete(write_str, towrite)
            && towrite <= (LONG) sizeof(icu->pendingCSI))
        {
            CopyMem(write_str, icu->pendingCSI, towrite);
            icu->pendingCSILen = towrite;
            write_str += towrite;
            break;
        }

        numparams = 0;

        if (!string2command(&command, &write_str, &numparams, towrite,
                param_tab, (Object *) unit, ConsoleDevice))
            break;

        Console_DoCommand((Object *) unit, command, numparams, param_tab);

        towrite = orig_towrite - (write_str - orig_write_str);
    } /* while (characters left to interpret) */

    if (orig_towrite > 0 && !plainText)
        Console_RenderCursor((Object *) unit);

    if (allocbuf)
        FreeMem(allocbuf, orig_towrite);

    ReturnInt("WriteToConsole", LONG, accepted);
}


/**********************
** string2command()  **
**********************/


/* Commands that are recognized by the complete character sequence after
   the CSI. CommandStr holds Length characters. */
static const struct special_cmd_descr
{
    BYTE Command;
    BYTE Length;
    UBYTE CommandStr[3];
} scd_tab[] =
{
    {C_SET_LF_MODE,           3, {0x32, 0x30, 0x68}}, /* Set linefeed mode */
    {C_RESET_LF_MODE,         3, {0x32, 0x30, 0x6C}}, /* Reset linefeed mode */
    {C_SET_AUTOSCROLL_MODE,   3, {0x3E, 0x31, 0x68}}, /* Set autoscroll mode */
    {C_RESET_AUTOSCROLL_MODE, 3, {0x3E, 0x31, 0x6C}}, /* Reset autoscroll mode */
    {C_SET_AUTOWRAP_MODE,     3, {0x3E, 0x37, 0x68}}, /* Set autowrap mode */
    {C_RESET_AUTOWRAP_MODE,   3, {0x3E, 0x37, 0x6C}}, /* Reset autowrap mode */
    {C_DEVICE_STATUS_REPORT,  2, {0x36, 0x6E}},       /* device status report */
    {C_CURSOR_VISIBLE,        2, {' ', 'p'}},         /* cursor visible */
    {C_CURSOR_VISIBLE,        3, {'1', ' ', 'p'}},    /* cursor visible */
    {C_CURSOR_INVISIBLE,      3, {'0', ' ', 'p'}},    /* cursor invisible */
    {C_WINDOW_STATUS_REQUEST, 2, {' ', 'q'}},         /* window status request */
    {C_WINDOW_STATUS_REQUEST, 3, {'0', ' ', 'q'}}     /* window status request */
};
#define NUM_SPECIAL_COMMANDS (sizeof(scd_tab) / sizeof(scd_tab[0]))

#if DEBUG
static UBYTE *cmd_names[NUM_CONSOLE_COMMANDS] = {

    "Ascii",                    /* C_ASCII = 0                  */

    "Esc",                      /* C_ESC                        */
    "Bell",                     /* C_BELL,                      */
    "Backspace",                /* C_BACKSPACE,                 */
    "HTab",                     /* C_HTAB,                      */
    "Linefeed",                 /* C_LINEFEED,                  */
    "VTab",                     /* C_VTAB,                      */
    "Formefeed",                /* C_FORMFEED,                  */
    "Carriage return",          /* C_CARRIAGE_RETURN,           */
    "Shift In",                 /* C_SHIFT_IN,                  */
    "Shift Out",                /* C_SHIFT_OUT,                 */
    "Index",                    /* C_INDEX,                     */
    "Nex Line",                 /* C_NEXT_LINE,                 */
    "Tab set",                  /* C_H_TAB_SET,                 */
    "Reverse Idx",              /* C_REVERSE_IDX,               */
    "Set LF Mode",              /* C_SET_LF_MODE,               */
    "Reset LF Mode",            /* C_RESET_lF_MODE,             */
    "Device Status Report",     /* C_DEVICE_STATUS_REPORT,      */

    "Insert Char",              /* C_INSERT_CHAR,               */
    "Cursor Up",                /* C_CURSOR_UP,                 */
    "Cursor Down",              /* C_CURSOR_DOWN,               */
    "Cursor Forward",           /* C_CURSOR_FORWARD,            */
    "Cursor Backward",          /* C_CURSOR_BACKWARD,           */
    "Cursor Next Line",         /* C_CURSOR_NEXT_LINE,          */
    "Cursor Prev Line",         /* C_CURSOR_PREV_LINE,          */
    "Cursor Pos",               /* C_CURSOR_POS,                */
    "Cursor HTab",              /* C_CURSOR_HTAB,               */
    "Erase In Display",         /* C_ERASE_IN_DISPLAY,          */
    "Erase In Line",            /* C_ERASE_IN_LINE,             */
    "Insert Line",              /* C_INSERT_LINE,               */
    "Delete Line",              /* C_DELETE_LINE,               */
    "Delete Char",              /* C_DELETE_CHAR,               */
    "Scroll Up",                /* C_SCROLL_UP,                 */
    "Scroll Down",              /* C_SCROLL_DOWN,               */
    "Cursor Tab Ctrl",          /* C_CURSOR_TAB_CTRL,           */
    "Cursor Backtab",           /* C_CURSOR_BACKTAB,            */
    "Select Graphic Rendition", /* C_SELECT_GRAPHIC_RENDITION   */
    "Window Status Request",    /* C_WINDOW_STATUS_REQUEST      */
    "Cursor Visible",           /* C_CURSOR_VISIBLE,            */
    "Cursor Invisible",         /* C_CURSOR_INVISIBLE,          */
    "Set Raw Events",           /* C_SET_RAWEVENTS,             */
    "Reset Raw Events",         /* C_RESET_RAWEVENTS            */
    "Set Auto Wrap Mode",       /* C_SET_AUTOWRAP_MODE          */
    "Reset Auto Wrap Mode",     /* C_RESET_AUTOWRAP_MODE        */
    "Set Auto Scroll Mode",     /* C_SET_AUTOSCROLL_MODE        */
    "Reset Auto Scroll Mode",   /* C_RESET_AUTOSCROLL_MODE      */
    "Set Page Length",          /* C_SET_PAGE_LENGTH            */
    "Set Line Length",          /* C_SET_LINE_LENGTH            */
    "Set Left Offset",          /* C_SET_LEFT_OFFSET            */
    "Set Top Offset"            /* C_SET_TOP_OFFSET             */
};
#endif

/* The control codes that are commands by themselves, with their commands.
   Each of them starts a command. The last NUM_TEXT_CTRL_CODES of them do
   not end a run of plain text that has already begun (see check_special()),
   so they have to stay at the end of the table. */
static const UBYTE ctrl_codes[][2] =
{
    {NIL,             C_NIL},
    {BELL,            C_BELL},
    {BACKSPACE,       C_BACKSPACE},
    {HTAB,            C_HTAB},
    {LINEFEED,        C_LINEFEED},
    {FORMFEED,        C_FORMFEED},
    {CARRIAGE_RETURN, C_CARRIAGE_RETURN},
    {SHIFT_OUT,       C_SHIFT_OUT},
    {SHIFT_IN,        C_SHIFT_IN},
    {ESC,             C_ESC},
    {INDEX,           C_INDEX},
    {H_TAB_SET,       C_H_TAB_SET},
    {REVERSE_INDEX,   C_REVERSE_IDX},
    {VTAB,            C_VTAB},
    {NEXT_LINE,       C_NEXT_LINE}
};
#define NUM_CTRL_CODES      (sizeof(ctrl_codes) / sizeof(ctrl_codes[0]))
#define NUM_TEXT_CTRL_CODES 2

/* Look a code up in the first num entries of ctrl_codes[]. Returns its
   command, or -1 if it is not there. */
static LONG find_ctrl_code(UBYTE code, ULONG num)
{
    ULONG i;

    for (i = 0; i < num; i++)
    {
        if (ctrl_codes[i][0] == code)
            return ctrl_codes[i][1];
    }
    return -1;
}

/* Does this character end a run of plain text? */
static BOOL check_special(STRPTR string, LONG toparse)
{
    /* Every code looked for is a C0 or C1 control code (0x00-0x1F or
       0x80-0x9F); all other characters are plain text. <ESC>[ needs no test
       of its own, because ESC is in the table. */
    if (*string & 0x60)
        return FALSE;

    return (*string == CSI) ||
        find_ctrl_code(*string, NUM_CTRL_CODES - NUM_TEXT_CTRL_CODES) >= 0;
}

static BOOL string2command(BYTE *cmd_ptr, UBYTE ** writestr_ptr,
    UBYTE *numparams_ptr, LONG toparse, IPTR *p_tab, Object *unit,
    struct ConsoleBase *ConsoleDevice)
{
    UBYTE *write_str = *writestr_ptr;
    UBYTE *csi_str = write_str;
    LONG csi_toparse = 0;

    BOOL found = FALSE, csi = FALSE;

    EnterFunc(bug("StringToCommand(toparse=%d)\n", toparse));

    /* Look for <CSI> */
    if (*write_str == CSI)
    {
        csi_str++;
        csi = TRUE;
        csi_toparse = toparse - 1;
    }
    else if (toparse >= 2)
    {
        if ((write_str[0] == ESC) && (write_str[1] == '['))
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
            for (i = 0; ((i < NUM_SPECIAL_COMMANDS) && (!found)); i++)
            {
                /* Check whether command sequence is longer than input */
                if (scd_tab[i].Length > csi_toparse)
                    continue;   /* if so, check next command sequence */

                D(bug
                    ("Comparing for special command %d, idx %d, cmdstr %p, len %d, csistr %p \n",
                        scd_tab[i].Command, i, scd_tab[i].CommandStr,
                        scd_tab[i].Length, csi_str));
                /* Command match ? */
                if (0 == strncmp(csi_str, scd_tab[i].CommandStr,
                        scd_tab[i].Length))
                {
                    D(bug("Special command found\n"));
                    csi_str += scd_tab[i].Length;
                    *cmd_ptr = scd_tab[i].Command;

                    found = TRUE;
                }
            } /* for (each special command) */
        }

        /* A parameter command? (I.e. one of the commands that take
         * parameters) */
        if (!found)
            found =
                getparamcommand(cmd_ptr, &csi_str, numparams_ptr,
                csi_toparse, p_tab, unit, ConsoleDevice);

    } /* if (CSI was found) */

    if (found)
        write_str = csi_str;
    else
    {
        /* Look for standalone codes */
        LONG code = find_ctrl_code(*write_str, NUM_CTRL_CODES);

        if (code >= 0)
        {
            *cmd_ptr = code;
            found = TRUE;
        }

        if (found)
        {
            /* Found special char. Increase pointer */

            write_str++;
        }
    }

    if (!found) /* Still not any found? Try to print as plain ASCII */
    {
        *cmd_ptr = C_ASCII_STRING;

        p_tab[0] = (IPTR) write_str;
        *numparams_ptr = 2;
        found = TRUE;

        do
        {
            toparse--;
            write_str++;
        }
        while (toparse && !check_special(write_str, toparse));

        /* store the string length */
        p_tab[1] = (IPTR) (write_str - (UBYTE *) p_tab[0]);
    }

    D(bug("FOUND CMD: %s\n", cmd_names[*cmd_ptr]));

    /* Return pointer to first character AFTER last interpreted char */
    *writestr_ptr = write_str;

    ReturnBool("StringToCommand", found);
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

} csi2command[] =
{
    {C_INSERT_CHAR,              1},                     /* 0x40 @ */
    {C_CURSOR_UP,                1},                     /* 0x41 A */
    {C_CURSOR_DOWN,              1},                     /* 0x42 B */
    {C_CURSOR_FORWARD,           1},                     /* 0x43 C */
    {C_CURSOR_BACKWARD,          1},                     /* 0x44 D */
    {C_CURSOR_NEXT_LINE,         1},                     /* 0x45 E */
    {C_CURSOR_PREV_LINE,         1},                     /* 0x46 F */
    {-1,                         },                      /* 0x47 G */
    {C_CURSOR_POS,               2},                     /* 0x48 H */
    {C_CURSOR_HTAB,              1},                     /* 0x49 I */
    {C_ERASE_IN_DISPLAY,         0},                     /* 0x4A J */
    {C_ERASE_IN_LINE,            1},                     /* 0x4B K */
    {C_INSERT_LINE,              1},                     /* 0x4C L */
    {C_DELETE_LINE,              1},                     /* 0x4D M */
    {-1,                         },                      /* 0x4E N */
    {-1,                         },                      /* 0x4F O */
    {C_DELETE_CHAR,              1},                     /* 0x50 P */
    {-1,                         },                      /* 0x51 Q */
    {-1,                         },                      /* 0x52 R */
    {C_SCROLL_UP,                1},                     /* 0x53 S */
    {C_SCROLL_DOWN,              1},                     /* 0x54 T */
    {-1,                         },                      /* 0x55 U */
    {-1,                         },                      /* 0x56 V */
    {C_CURSOR_TAB_CTRL,          1},                     /* 0x57 W */
    {-1,                         },                      /* 0x58 X */
    {-1,                         },                      /* 0x59 Y */
    {C_CURSOR_BACKTAB,           1},                     /* 0x5A Z */
    {-1,                         },                      /* 0x5B [ */
    {-1,                         },                      /* 0x5C \ */
    {-1,                         },                      /* 0x5D ] */
    {-1,                         },                      /* 0x5E ^ */
    {-1,                         },                      /* 0x5F _ */
    {-1,                         },                      /* 0x60 ` */
    {-1,                         },                      /* 0x61 a */
    {-1,                         },                      /* 0x62 b */
    {-1,                         },                      /* 0x63 c */
    {-1,                         },                      /* 0x64 d */
    {-1,                         },                      /* 0x65 e */
    {-1,                         },                      /* 0x66 f */
    {-1,                         },                      /* 0x67 g */
    {-1,                         },                      /* 0x68 h */
    {-1,                         },                      /* 0x69 i */
    {-1,                         },                      /* 0x6A j */
    {-1,                         },                      /* 0x6B k */
    {-1,                         },                      /* 0x6C l */
    {C_SELECT_GRAPHIC_RENDITION, MAX_COMMAND_PARAMS},    /* 0x6D m */
    {-1,                         },                      /* 0x6E n */
    {-1,                         },                      /* 0x6F o */
    {-1,                         },                      /* 0x70 p */
    {-1,                         },                      /* 0x71 q */
    {-1,                         },                      /* 0x72 r */
    {-1,                         },                      /* 0x73 s */
    {C_SET_PAGE_LENGTH,          1},                     /* 0x74 t */
    {C_SET_LINE_LENGTH,          1},                     /* 0x75 u */
    {-1,                         },                      /* 0x76 v */
    {-1,                         },                      /* 0x77 w */
    {C_SET_LEFT_OFFSET,          1},                     /* 0x78 x */
    {C_SET_TOP_OFFSET,           1},                     /* 0x79 y */
    {-1,                         },                      /* 0x7A z */
    {C_SET_RAWEVENTS,            MAX_COMMAND_PARAMS},    /* 0x7B { */
    {-1,                         },                      /* 0x7C | */
    {C_RESET_RAWEVENTS,          MAX_COMMAND_PARAMS},    /* 0x7D } */
};
#define NUM_CSI_COMMANDS (sizeof(csi2command) / sizeof(csi2command[0]))


#define PARAM_BUF_SIZE MAX_COMMAND_PARAMS
/* Parameters for commands are parsed and filled into this one */
struct cmd_params
{
    UBYTE numparams;            /* Parameters stored */
    BOOL overflow;              /* Decimal parameter exceeded IPTR */

    /* Since parameters may be optional, only supplied parameters
       are saved, along with their number. For example
       for the command CURSOR POSITION, if only the second parameter
       (column) is specified in the write stream, then
       numparams will be 1 and val[0] will be <column>.
       Row will have to be set to some default value.
     */
    IPTR val[PARAM_BUF_SIZE];
};


static BOOL getparamcommand(BYTE *cmd_ptr, UBYTE ** writestr_ptr,
    UBYTE *numparams_ptr, LONG toparse, IPTR *p_tab, Object *unit,
    struct ConsoleBase *ConsoleDevice)
{
    /* This function checks for a command with parameters in
     ** the string. The problem is that the parameters come
     ** before the comand ID, and the parameters are optional.
     ** This means that a parameter which has the same value
     ** as a command ID may be mistakenly taken for being
     ** end of the command. Therefore we must continue scanning
     ** even if we found a command ID.
     */

    /* Only the counters need clearing: a val[] entry is set to zero when
       its parameter starts */
    struct cmd_params params;
    BYTE cmd = -1;
    BYTE cmd_next_idx = 0;      /* Index to byte after the command */

    /* write_str points to first character after <CSI> */
    UBYTE *write_str = *writestr_ptr;

    UBYTE num_params = 0;
    BOOL done = FALSE, found = FALSE;
    BOOL next_can_be_separator = TRUE,
        next_can_be_param = TRUE,
        next_can_be_commandid = TRUE, last_was_param = FALSE;
    UBYTE num_separators_found = 0;

    params.numparams = 0;
    params.overflow = FALSE;

    while (!done)
    {
        /* In case it's a parameter */

        if (toparse <= 0)
        {
            done = TRUE;
            break;
        }

        /* A character with an entry in csi2command[] ends the sequence */
        if (*write_str >= FIRST_CSI_CMD &&
            *write_str < FIRST_CSI_CMD + NUM_CSI_COMMANDS &&
            csi2command[*write_str - FIRST_CSI_CMD].Command != -1)
        {
            {
                UBYTE idx = *write_str - FIRST_CSI_CMD;
                UBYTE maxparams = csi2command[idx].MaxParams;

                if (next_can_be_commandid)
                {
/* FIXME: Should also do a MinParams compare */
                    if (num_params <= maxparams) /* Valid command? */
                    {

                        /* Assure that there are not too many separators in
                         * a command  */
                        if ((num_separators_found < maxparams)
/* FIXME: 0-param commands can be moved to special-command-handlin in string2command() */
                            || ((num_separators_found == 0)
                                && (maxparams == 0)))
                        {
                            cmd = csi2command[idx].Command;

                            /* Save index to where the next command will
                             * start */
                            cmd_next_idx = write_str - *writestr_ptr + 1;

                            params.numparams = num_params;
                        }
                    }
                }

                done = TRUE;
            }
        }
        else switch (*write_str)
        {
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

            num_separators_found++;

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
                params.val[num_params - 1] = 0;

                last_was_param = TRUE;
            }

            if (!params.overflow)
            {
                IPTR digit = (*write_str == '>') ?
                    5 : (*write_str) - '0';
                IPTR maxval = ~(IPTR)0;

                if (params.val[num_params - 1] >
                    (maxval - digit) / 10)
                {
                    params.overflow = TRUE;
                }
                else
                {
                    params.val[num_params - 1] =
                        params.val[num_params - 1] * 10 + digit;
                }
            }

            next_can_be_separator = TRUE;
            next_can_be_commandid = TRUE;
            break;

        default:
            /* Error */
            done = TRUE;
            break;
        } /* switch */

        write_str++;
        toparse--;
    } /* while (!done) */

    if (cmd != -1)
    {
        found = TRUE;

        /* Continue parsing on the first byte after the command */
        *writestr_ptr += cmd_next_idx;

        if (params.overflow)
        {
            *cmd_ptr = C_NIL;
            *numparams_ptr = 0;
            return TRUE;
        }

        *cmd_ptr = cmd;
    }

    if (found)
    {
        UBYTE i;
        /* First fill in some default values in p_tab */
        Console_GetDefaultParams(unit, *cmd_ptr, p_tab);

        for (i = 0; i < params.numparams; i++)
        {
            /* Override with parsed values */
            D(bug("CMD %s: Setting param %d to %d\n", cmd_names[*cmd_ptr],
                    i, params.val[i]));

            p_tab[i] = params.val[i];
        }

        *numparams_ptr = params.numparams;
    }

    return found;
}

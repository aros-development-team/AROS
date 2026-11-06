#ifndef DEVICES_CONSOLE_H
#define DEVICES_CONSOLE_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_IO_H
#   include <exec/io.h>
#endif

/* console.device commands */
#define CD_ASKKEYMAP		(CMD_NONSTD + 0)
#define CD_SETKEYMAP		(CMD_NONSTD + 1)
#define CD_ASKDEFAULTKEYMAP	(CMD_NONSTD + 2)
#define CD_SETDEFAULTKEYMAP	(CMD_NONSTD + 3)

/* Commands below CMD_NONSTD + 0x80 are left to replacement console devices */
#define CD_GETCAPS		(CMD_NONSTD + 0x80)	/* io_Data -> ULONG capability flags, io_Length >= 4 */
#define CD_GETPASTEMODE		(CMD_NONSTD + 0x81)	/* io_Data -> ULONG, 1 while the program asked for bracketed paste */

/* CD_GETCAPS flags, for the unit the request was opened on. console.device
   does not answer CD_GETCAPS; an error means none. */
#define CONCAPF_OWNWINDOW	(1 << 0)	/* ConUnit cu_Window is valid until CloseDevice() */
#define CONCAPF_STREAMREADS	(1 << 1)	/* CMD_READ returns the bytes available at once */
#define CONCAPF_CSI7		(1 << 2)	/* input sends ESC [ instead of the 8-bit CSI */
#define CONCAPF_PASTEREQUEST	(1 << 3)	/* sends CSI 0 SPACE v to ask the handler to paste */
#define CONCAPF_BRACKETEDPASTE	(1 << 4)	/* answers CD_GETPASTEMODE */
#define CONCAPF_LINEREADS	(1 << 5)	/* a cooked read hands back one line at a time */

/* SGR parameters */
#define SGR_PRIMARY	0
#define SGR_BOLD	1
#define SGR_ITALIC	3
#define SGR_UNDERSCORE	4
#define SGR_NEGATIVE	7
#define SGR_NORMAL	22
#define SGR_NOTITALIC	23
#define SGR_NOTUNDERSCORE 24
#define SGR_POSITIVE	27

#define SGR_BLACK	30
#define SGR_RED		31
#define SGR_GREEN	32
#define SGR_YELLOW	33
#define SGR_BLUE	34
#define SGR_MAGENTA	35
#define SGR_CYAN	36
#define SGR_WHITE	37
#define SGR_DEFAULT	39

#define SGR_BLACKBG	40
#define SGR_REDBG	41
#define SGR_GREENBG	42
#define SGR_YELLOWBG	43
#define SGR_BLUEBG	44
#define SGR_MAGENTABG	45
#define SGR_CYANBG	46
#define SGR_WHITEBG	47
#define SGR_DEFAULTBG	49

#define SGR_CLR0	30
#define SGR_CLR1	31
#define SGR_CLR2	32
#define SGR_CLR3	33
#define SGR_CLR4	34
#define SGR_CLR5	35
#define SGR_CLR6	36
#define SGR_CLR7	37

#define SGR_CLR0BG	40
#define SGR_CLR1BG	41
#define SGR_CLR2BG	42
#define SGR_CLR3BG	43
#define SGR_CLR4BG	44
#define SGR_CLR5BG	45
#define SGR_CLR6BG	46
#define SGR_CLR7BG	47

/* DSR parameters */
#define DSR_CPR		6

/* CTC parameters */
#define CTC_HSETTAB	0
#define CTC_HCLRTAB	2
#define CTC_HCLRTABSALL	5

/* TBC parameters */
#define TBC_HCLRTAB	0
#define TBC_HCLRTABSALL	3

/* SM & RM parameters */
#define M_LNM	20
#define M_ASM	">1"
#define M_AWM	"?7"

#endif /* DEVICES_CONSOLE_H */

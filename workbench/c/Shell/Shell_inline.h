/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "buffer.c"

#include "cliEcho.c"			/* commands echoing		*/
#include "cliLen.c"			/* string length		*/
#include "cliNan.c"			/* not-a-number test		*/
#include "cliPrompt.c"			/* print the prompt		*/
#include "cliVarNum.c"			/* SetVar with a LONG		*/

/* convertLine */
#include "convertArg.c"			/* <.bra .ket> handling		*/
#include "convertBackTicks.c"		/* `Back-Ticks` handling	*/
#include "convertLineDot.c"		/* .dot commands handling	*/
#include "convertRedir.c"		/* < > >> redirections		*/
#include "convertVar.c"			/* ${var} handling		*/
#include "convertLine.c"		/* command line handling	*/

#include "interpreter.c"		/* internal CLI state		*/
#include "redirection.c"		/* manage Redirection		*/
#include "readLine.c"			/* read from current input	*/

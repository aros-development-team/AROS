#ifndef REXX_RXSLIB_H
#define REXX_RXSLIB_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ARexx data structures
    Lang: English
*/

#ifndef REXX_STORAGE_H
#include <rexx/storage.h>
#endif

#define RXSNAME  "rexxsyslib.library"
#define RXSDIR	 "REXX"
#define RXSTNAME "ARexx"

/* Information in Rxslib should not be used in client programs
 * Therefor don't provide any information to these client programs
 */
struct Rxslib;

/* These are not necessary for client program either I think
#define RLFB_TRACE  RTFB_TRACE
#define RLFB_HALT   RTFB_HALT
#define RLFB_SUSP   RTFB_SUSP
#define RLFB_STOP   6
#define RLFB_CLOSE  7

#define RLFMASK     ((1<<RLFB_TRACE) | (1<<RLFB_HALT) | (1<<RLFB_SUSP))

#define RXSCHUNK    1024
#define RXSNEST     32
#define RXSTPRI     0
#define RXSSTACK    4096
*/

/* I'm not sure about these ones but let's dissable them for now
#define CTB_SPACE   0
#define CTB_DIGIT   1
#define CTB_ALPHA   2
#define CTB_REXXSYM 3
#define CTB_REXXOPR 4
#define CTB_REXXSPC 5
#define CTB_UPPER   6
#define CTB_LOWER   7

#define CTF_SPACE   (1 << CTB_SPACE)
#define CTF_DIGIT   (1 << CTB_DIGIT)
#define CTF_ALPHA   (1 << CTB_ALPHA)
#define CTF_REXXSYM (1 << CTB_REXXSYM)
#define CTF_REXXOPR (1 << CTB_REXXOPR)
#define CTF_REXXSPC (1 << CTB_REXXSPC)
#define CTF_UPPER   (1 << CTB_UPPER)
#define CTF_LOWER   (1 << CTB_LOWER)
*/

#endif

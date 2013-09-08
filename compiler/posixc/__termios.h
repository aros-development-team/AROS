#ifndef __AROSC_TERMIOS_H
#define __AROSC_TERMIOS_H

/*
 * Copyright © 1995-2012, The AROS Development Team. All rights reserved.
 * $Id$
 *
 * internal header file for POSIX.1-2008 termios.h support
 */

#include <termios.h>

struct termios_intern {
    /* Public part; needs to be backwards compatible */
    tcflag_t c_iflag;	/* input mode flags */
    tcflag_t c_oflag;	/* output mode flags */
    tcflag_t c_cflag;	/* control mode flags */
    tcflag_t c_lflag;	/* local mode flags */
    cc_t c_cc[NCCS];	/* control characters */
    /* Internal part; may be changed */
    speed_t c_ispeed;	/* input speed */
    speed_t c_ospeed;	/* output speed */
};

#endif /* __AROSC_TERMIOS_H */

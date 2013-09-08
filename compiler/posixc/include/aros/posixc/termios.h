#ifndef _TERMIOS_H
#define _TERMIOS_H 1

/*
 * Copyright © 1995-2012, The AROS Development Team. All rights reserved.
 * $Id$
 *
 * POSIX.1-2008 header file <termios.h>
 */

#include <aros/system.h>

/* FIXME: Are these OK ? */
typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

#define NCCS 32
struct termios {
    tcflag_t c_iflag;	/* input mode flags */
    tcflag_t c_oflag;	/* output mode flags */
    tcflag_t c_cflag;	/* control mode flags */
    tcflag_t c_lflag;	/* local mode flags */
    cc_t c_cc[NCCS];	/* control characters */
    char internal[64];  /* Private */
};

/* c_cc characters */
#define VEOF		0
#define VEOL		1
#define VERASE		2
#define VINTR		3
#define VKILL		4
#define VMIN		5
#define VQUIT		6
#define VSTART		7
#define VSTOP		8
#define VSUSP		9
#define VTIME		10

/* Input Modes */
#define BRKINT	0000001
#define ICRNL	0000002
#define IGNBRK	0000004
#define IGNCR	0000010
#define IGNPAR	0000020
#define INLCR	0000040
#define INPCK	0000100
#define ISTRIP	0000200
#define IXANY	0000400
#define IXOFF	0001000
#define IXON	0002000
#define PARMRK	0004000

/* Output Modes */
#define OPOST	0000001
#define ONLCR	0000002
#define OCRNL	0000004
#define ONOCR	0000010
#define ONLRET	0000020
#define OFDEL	0000040
#define OFILL	0000100
#define NLDLY   0000200
#define NL0     0000000
#define NL1     0000200
#define CRDLY   0001400
#define CR0     0000000
#define CR1     0000400
#define CR2     0001000
#define CR3     0001400
#define TBLDLY  0006000
#define TAB0    0000000
#define TAB1    0002000
#define TAB2    0004000
#define TAB3    0006000
#define BSDLY   0010000
#define BS0     0000000
#define BS1     0010000
#define VRDLY   0020000
#define VT0     0000000
#define VT1     0020000
#define FFDLY   0040000
#define FF0     0000000
#define FF1     0040000

/* Baud Rate Selection */
#define B0	(speed_t)0000000 /* hang up */
#define B50	(speed_t)0000001
#define B75	(speed_t)0000002
#define B110	(speed_t)0000003
#define B134	(speed_t)0000004
#define B150	(speed_t)0000005
#define B200	(speed_t)0000006
#define B300	(speed_t)0000007
#define B600	(speed_t)0000010
#define B1200	(speed_t)0000011
#define B1800	(speed_t)0000012
#define B2400	(speed_t)0000013
#define B4800	(speed_t)0000014
#define B9600	(speed_t)0000015
#define B19200	(speed_t)0000016
#define B38400	(speed_t)0000017

/* Control Modes */
#define CSIZE   0000003
#define   CS5   0000000
#define   CS6   0000001
#define   CS7   0000002
#define   CS8   0000003
#define CSTOPB  0000004
#define CREAD   0000010
#define PARENB  0000020
#define PARODD  0000040
#define HUPCL   0000100
#define CLOCAL  0000200

/* Local Modes */
#define ECHO	0000001
#define ECHOE	0000002
#define ECHOK	0000004
#define ECHONL	0000010
#define ICANON	0000020
#define IEXTEN	0000100
#define ISIG	0000200
#define NOFLSH	0000400
#define TOSTOP	0001000

/* Attribute Selection */
#define TCSANOW		0
#define TCSADRAIN	1
#define TCSAFLUSH	2

/* Line Control */
#define TCIFLUSH	0
#define TCOFLUSH	1
#define TCIOFLUSH	2

#define TCIOFF		0
#define TCION		1
#define TCOOFF		2
#define TCOON		3

#include <aros/types/pid_t.h>


__BEGIN_DECLS

speed_t cfgetispeed(const struct termios *);
speed_t cfgetospeed(const struct termios *);
int cfsetispeed(struct termios *, speed_t);
int cfsetospeed(struct termios *, speed_t);
/* NOTIMPL int tcdrain(int); */
/* NOTIMPL int tcflow(int, int); */
/* NOTIMPL int tcflush(int, int); */
int tcgetattr(int __fd, struct termios *__termios_p);
/* NOTIMPL pid_t tcgetsid(int); */
/* NOTIMPL int tcsendbreak(int, int); */
int tcsetattr(int __fd, int __optional_actions,
              const struct termios *__termios_p);

__END_DECLS

#endif

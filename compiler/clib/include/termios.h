#ifndef _TERMIOS_H
#define _TERMIOS_H 1

/*
 * Copyright © 1995-2012, The AROS Development Team. All rights reserved.
 * $Id$
 *
 * POSIX.1-2008 header file <termios.h>
 */

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
#define VINTR		0
#define VQUIT		1
#define VERASE		2
#define VKILL		3
#define VEOF		4
#define VTIME		5
#define VMIN		6
#define VSWTC		7
#define VSTART		8
#define VSTOP		9
#define VSUSP		10
#define VEOL		11
#define VREPRINT	12
#define VDISCARD	13
#define VWERASE		14
#define VLNEXT		15
#define VEOL2		16

/* c_iflag bits */
#define IGNBRK	0000001
#define BRKINT	0000002
#define IGNPAR	0000004
#define PARMRK	0000010
#define INPCK	0000020
#define ISTRIP	0000040
#define INLCR	0000100
#define IGNCR	0000200
#define ICRNL	0000400
#define IUCLC	0001000
#define IXON	0002000
#define IXANY	0004000
#define IXOFF	0010000
#define IMAXBEL	0020000
#define IUTF8   0040000

/* c_oflag bits */
#define OPOST	0000001
#define OLCUC	0000002
#define ONLCR	0000004
#define OCRNL	0000010
#define ONOCR	0000020
#define ONLRET	0000040
#define OFILL	0000100
#define OFDEL	0000200

/* c_cflag bit meaning */
#define B0	0000000 /* hang up */
#define B50	0000001
#define B75	0000002
#define B110	0000003
#define B134	0000004
#define B150	0000005
#define B200	0000006
#define B300	0000007
#define B600	0000010
#define B1200	0000011
#define B1800	0000012
#define B2400	0000013
#define B4800	0000014
#define B9600	0000015
#define B19200	0000016
#define B38400	0000017
#define CSIZE   0000060
#define   CS5   0000000
#define   CS6   0000020
#define   CS7   0000040
#define   CS8   0000060
#define CSTOPB  0000100
#define CREAD   0000200
#define PARENB  0000400
#define PARODD  0001000
#define HUPCL   0002000
#define CLOCAL  0004000
#define B57600	0010001
#define B115200	0010002

/* c_lflag bits */
#define ISIG	0000001
#define ICANON	0000002
#define ECHO	0000010
#define ECHOE	0000020
#define ECHOK	0000040
#define ECHONL	0000100
#define NOFLSH	0000200
#define TOSTOP	0000400
#define IEXTEN	0100000

/* tcflow() and TCXONC use these */
#define TCOOFF		0
#define TCOON		1
#define TCIOFF		2
#define TCION		3

/* tcflush() and TCFLSH	use these */
#define TCIFLUSH	0
#define TCOFLUSH	1
#define TCIOFLUSH	2

/* tcsetattr uses these */
#define TCSANOW		0
#define TCSADRAIN	1
#define TCSAFLUSH	2

extern int tcgetattr(int __fd, struct termios *__termios_p);
extern int tcsetattr(int __fd, int __optional_actions,
                    const struct termios *__termios_p);

extern speed_t cfgetispeed(const struct termios *__termios_p);
extern speed_t cfgetospeed(const struct termios *__termios_p);

extern int cfsetispeed(struct termios *__termios_p, speed_t __speed);
extern int cfsetospeed(struct termios *__termios_p, speed_t __speed);

#endif

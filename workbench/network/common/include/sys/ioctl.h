#ifndef	SYS_IOCTL_H
#define	SYS_IOCTL_H \
       "$Id: ioctl.h,v 4.2 1996/01/27 21:14:12 jraja Exp $"
/*
 *	SocketIoctl codes
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development, Inc.
 *                       All rights reserved.
 */

#include <sys/types.h>

struct winsize
{
    unsigned short int ws_row;
    unsigned short int ws_col;
    unsigned short int ws_xpixel;
    unsigned short int ws_ypixel;
};

__BEGIN_DECLS
int ioctl(int fd, int request, ...);
__END_DECLS

/*
 * Ioctl's have the command encoded in the lower word, and the size of
 * any in or out parameters in the upper word.  The high 3 bits of the
 * upper word are used to encode the in/out status of the parameter.
 */

#define	IOCPARM_MASK	0x1fff		/* parameter length, at most 13 bits */
#define	IOCPARM_LEN(x)	(((x) >> 16) & IOCPARM_MASK)
#define	IOCBASECMD(x)	((x) & ~IOCPARM_MASK)
#define	IOCGROUP(x)	(((x) >> 8) & 0xff)

#define	IOCPARM_MAX	4096		                /* max size of ioctl */
#define	IOC_VOID	0x20000000	                    /* no parameters */
#define	IOC_OUT		0x40000000	              /* copy out parameters */
#define	IOC_IN		0x80000000	               /* copy in parameters */
#define	IOC_INOUT	(IOC_IN|IOC_OUT)
#define	IOC_DIRMASK	0xe0000000	             /* mask for IN/OUT/VOID */

#define _IOC(inout,group,num,len) \
	(inout | (((int)(len) & IOCPARM_MASK) << 16) | ((group) << 8) | (num))
#define	_IO(g,n)	_IOC(IOC_VOID,	(g), (n), 0)
#define	_IOR(g,n,t)	_IOC(IOC_OUT,	(g), (n), sizeof(t))
#define	_IOW(g,n,t)	_IOC(IOC_IN,	(g), (n), sizeof(t))
/* this should be _IORW, but stdio got there first */
#define	_IOWR(g,n,t)	_IOC(IOC_INOUT,	(g), (n), sizeof(t))

/* 
 * Console I/O controls
 */
#define TIOCSWINSZ  _IOW('t', 103, struct winsize)  /* set window size */
#define TIOCGWINSZ  _IOR('t', 104, struct winsize)  /* get window size */

/* 
 * File I/O controls
 */
#define	FIOCLEX		_IO('f', 1)	          /* set close on exec on fd */
#define	FIONCLEX	_IO('f', 2)	             /* remove close on exec */
#define	FIONREAD	_IOR('f', 127, long)          /* get # bytes to read */
#define	FIONBIO		_IOW('f', 126, long)   /* set/clear non-blocking i/o */
#define	FIOASYNC	_IOW('f', 125, long)          /* set/clear async i/o */
#define	FIOSETOWN	_IOW('f', 124, long)    /* set owner (struct Task *) */
#define	FIOGETOWN	_IOR('f', 123, long)    /* get owner (struct Task *) */

/* 
 * Socket I/O controls
 *
 * SIOCSPGRP and SIOCGPGRP are identical to the FIOSETOWN and FIOGETOWN,
 * respectively.
 */
#define SIOCSHIWAT	_IOW('s',  0, int)             /* set high watermark */
#define SIOCGHIWAT	_IOR('s',  1, int)             /* get high watermark */
#define SIOCSLOWAT	_IOW('s',  2, int)             /* set low watermark  */
#define SIOCGLOWAT	_IOR('s',  3, int)             /* get low watermark  */
#define	SIOCATMARK	_IOR('s',  7, long)                  /* at oob mark? */
#define	SIOCSPGRP	_IOW('s',  8, long)             /* set process group */
#define	SIOCGPGRP	_IOR('s',  9, long)             /* get process group */

#define	SIOCADDRT	_IOW('r', 10, struct ortentry)          /* add route */
#define	SIOCDELRT	_IOW('r', 11, struct ortentry)       /* delete route */

#define	SIOCSIFADDR	_IOW ('i',12, struct ifreq)     /* set ifnet address */
#define SIOCGIFNAME     _IOWR('i',31, struct ifreq)     /* get IF name by index */
#define SIOCGIFINDEX    _IOWR('i',32, struct ifreq)     /* get IF index */
#define	SIOCGIFADDR	_IOWR('i',33, struct ifreq)     /* get ifnet address */
#define	SIOCSIFDSTADDR	_IOW ('i',14, struct ifreq)       /* set p-p address */
#define	SIOCGIFDSTADDR	_IOWR('i',34, struct ifreq)       /* get p-p address */
#define	SIOCSIFFLAGS	_IOW ('i',16, struct ifreq)       /* set ifnet flags */
#define	SIOCGIFFLAGS	_IOWR('i',17, struct ifreq)       /* get ifnet flags */
#define	SIOCGIFBRDADDR	_IOWR('i',35, struct ifreq)    /* get broadcast addr */
#define	SIOCSIFBRDADDR	_IOW ('i',19, struct ifreq)    /* set broadcast addr */
#define	SIOCGIFCONF	_IOWR('i',36, struct ifconf)       /* get ifnet list */
#define	SIOCGIFNETMASK	_IOWR('i',37, struct ifreq)     /* get net addr mask */
#define	SIOCSIFNETMASK	_IOW ('i',22, struct ifreq)     /* set net addr mask */
#define	SIOCGIFMETRIC	_IOWR('i',23, struct ifreq)         /* get IF metric */
#define	SIOCSIFMETRIC	_IOW ('i',24, struct ifreq)         /* set IF metric */
#define	SIOCDIFADDR	_IOW ('i',25, struct ifreq)        /* delete IF addr */
#define	SIOCAIFADDR	_IOW ('i',26, struct ifaliasreq) /* add/chg IF alias */

#define	SIOCSARP	_IOW('I', 30, struct arpreq)        /* set arp entry */
#define	SIOCGARP	_IOWR('I',38, struct arpreq)        /* get arp entry */
#define	SIOCDARP	_IOW('I', 32, struct arpreq)     /* delete arp entry */

#define SIOCADDMULTI	_IOW ('i', 49, struct ifreq)    /* add m'cast addr */
#define SIOCDELMULTI	_IOW ('i', 50, struct ifreq)    /* del m'cast addr */
#define SIOCGIFMTU	_IOWR('i', 51, struct ifreq)    /* get IF mtu */
#define SIOCSIFMTU	_IOW ('i', 52, struct ifreq)    /* set IF mtu */
#define SIOCGIFPHYS	_IOWR('i', 53, struct ifreq)    /* get IF wire */
#define SIOCSIFPHYS	_IOW ('i', 54, struct ifreq)    /* set IF wire */
#define SIOCGETVIFCNT	_IOWR('u', 51, struct sioc_vif_req)/* vif pkt cnt */
#define SIOCGETSGCNT	_IOWR('u', 52, struct sioc_sg_req) /* sg pkt cnt */

#define SIOCGIFASYNCMAP _IOWR('i', 124, struct ifreq)   /* get ppp asyncmap */
#define SIOCSIFASYNCMAP  _IOW('i', 125, struct ifreq)   /* set ppp asyncmap */

/*
 * IPv6 interface address ioctls.
 * Requires <netinet/in_var.h> for struct in6_ifreq / in6_aliasreq.
 */
#define SIOCAIFADDR_IN6	 _IOW ('i', 26, struct in6_aliasreq) /* add/chg IPv6 alias */
#define SIOCDIFADDR_IN6  _IOW ('i', 25, struct in6_ifreq)   /* delete IPv6 addr */
#define SIOCGIFADDR_IN6	 _IOWR('i', 33, struct in6_ifreq)   /* get IPv6 address */
#define SIOCGIFDSTADDR_IN6 _IOWR('i', 34, struct in6_ifreq) /* get IPv6 p-p addr */
#define SIOCGIFNETMASK_IN6 _IOWR('i', 37, struct in6_ifreq) /* get IPv6 netmask */

/*
 * Private extensions to the BSD44 ioctl interface) 
 */
#define SIOCGARPT	_IOWR('I',66, struct arptabreq)     /* get arp table */

/*
 * EXPERIMENTAL support for the internal SLIP & PPP drivers
 *
 * ifr_data carries a struct IOExtSer *
 */
#define SIOCGSERREQ	_IOWR('i',70, struct ifreq)   /* get ser IO req */
#define SIOCFSERREQ	_IOW ('i',71, struct ifreq)    /* return IO req */

#endif /* !SYS_IOCTL_H */

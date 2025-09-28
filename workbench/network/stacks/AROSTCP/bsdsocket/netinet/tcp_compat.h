#ifndef NETINET_TCP_COMPAT_H
#define NETINET_TCP_COMPAT_H

/* Legacy definitions used by AROSTCP - code using these needs updated */

/* increment for tcp_iss each second */
#define	TCP_ISSINCR	(125*1024)

/* timestamp wrap-around time */
#define TCP_PAWS_IDLE	(24 * 24 * 60 * 60 * PR_SLOWHZ)

extern int	tcp_iss;		/* tcp initial send seq # */
extern int	tcp_ccgen;		/* global connection count */

/* Macro to increment a CC: skip 0 which has a special meaning */
#define CC_INC(c)	(++(c) == 0 ? ++(c) : (c))

#endif

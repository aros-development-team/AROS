/*
 * Copyright (C) 2026 The AROS Development Team.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * mld6.h - Multicast Listener Discovery for IPv6 (RFC 2710).
 */

#ifndef NETINET6_MLD6_H
#define NETINET6_MLD6_H

/* MLD membership states per in6_multi */
#define MLD6_IDLE_MEMBER	0	/* no report pending */
#define MLD6_LAZY_MEMBER	1	/* unsolicited report timer running */
#define MLD6_SLEEPING_MEMBER	2	/* query-response timer running */
#define MLD6_AWAKENING_MEMBER	3	/* suppressed by another's report */

#ifdef KERNEL
void mld6_init(void);
void mld6_input(struct mbuf *, int, int);
void mld6_start_listening(struct in6_multi *);
void mld6_stop_listening(struct in6_multi *);
void mld6_fasttimeo(void);
#endif

#endif /* NETINET6_MLD6_H */

#ifndef BSDSOCKET_SOCKETBASETAGS_H
#define BSDSOCKET_SOCKETBASETAGS_H
/*
 * $Id$
 *
 * Copyright (C) 1994 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * 
 *
 *       TAG Values for SocketBaseTagList()
 */

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*
 * utility/tagitem.h specifies that bits 16-30 in tags are reserved. So we 
 * don't use them for maximum compatability.
 */

/*
 * Argument passing convention (bit 15)
 */
#define SBTF_REF 0x8000		/* 0x0000 == VAL */

/*
 * Code (bits 1-14)
 */
#define SBTB_CODE 1
#define SBTS_CODE 0x3FFF
#define SBTM_CODE(tag) (((UWORD)(tag) >> SBTB_CODE) & SBTS_CODE)

/* 
 * Direction (bit 0)
 */
#define SBTF_SET  0x1		/* 0 == GET */

/*
 * Macros to set things up
 * We keep the TAG_USER (bit 31) set to be compatible with tagitem.h
 * conventions.
 */
#define SBTM_GETREF(code) \
  (TAG_USER | SBTF_REF | (((code) & SBTS_CODE) << SBTB_CODE))
#define SBTM_GETVAL(code) \
  (TAG_USER | (((code) & SBTS_CODE) << SBTB_CODE))
#define SBTM_SETREF(code) \
  (TAG_USER | SBTF_REF | (((code) & SBTS_CODE) << SBTB_CODE) | SBTF_SET)
#define SBTM_SETVAL(code) \
  (TAG_USER | (((code) & SBTS_CODE) << SBTB_CODE) | SBTF_SET)

/*
 * Tag code definitions. These codes are used with one of the above macros.
 *
 * All arguments are ULONG's or pointers (PTR suffix).
 *
 * NOTE: Tag code 0 is not used (see utility/tagitem.h).
 */

/* signal masks */
#define SBTC_BREAKMASK		1
#define SBTC_SIGIOMASK		2
#define SBTC_SIGURGMASK		3
#define SBTC_SIGEVENTMASK  4                 /* AMITCP v4.0 Stack Addition */

/*
 * Stack reconfigure/address-change notification (AROS additions).
 *
 * A consumer sets SBTC_SIG_RECONFIG_MASK to a signal mask; the stack signals
 * it when an in-place reconfigure (reload) begins and again when it ends.  The
 * consumer tells begin from end by reading SBTC_RECONFIG_STATE (NETRC_*), and
 * can detect that a reload happened at all via SBTC_RECONFIG_GENERATION, which
 * is bumped after each completed reload.  SBTC_SIG_ADDRESS_CHANGE_MASK (the
 * Roadshow-defined code 57) is signalled when interface addresses change.
 */
#ifndef SBTC_SIG_ADDRESS_CHANGE_MASK
#define SBTC_SIG_ADDRESS_CHANGE_MASK 57      /* set/get: signal on address change */
#endif
#define SBTC_SIG_RECONFIG_MASK   70          /* set/get: signal on reconfigure begin & end */
#define SBTC_RECONFIG_STATE      71          /* get: NETRC_* current reconfigure state */
#define SBTC_RECONFIG_GENERATION 72          /* get: ULONG, bumped after each reload */
#define SBTC_RECONFIG_ACK        73          /* set: consumer acks reconfigure-begin (quiesced) */

#define NETRC_ONLINE        0                /* stack configured and running */
#define NETRC_RECONFIGURING 1                /* an in-place reload is in progress */

/*
 * The out-of-band control channel that drives the reconfigure reported by the
 * SBTC_RECONFIG_* tags above (named MsgPort + NetControlMsg) lives in its own
 * header, <bsdsocket/netcontrol.h> - it is NOT here because socketbasetags.h is
 * pulled in by <netdb.h> for every networking program and must not force
 * <exec/ports.h> (struct Message) on all of them.
 */

/* error code handling */
#define SBTC_ERRNO		6
#define SBTC_HERRNO		7

/* socket descriptor table related tags */
#define SBTC_DTABLESIZE         8

/* link library fd allocation callback
 * 
 * Argument is a callback function with following prototype
 *
 * int fd = fdCallback(int fd, int action);
 *     D0                  D0      D1
 *
 * see net.lib sources for an example
 */
#define SBTC_FDCALLBACK         9
/*
 * "action" values:
 */
#define FDCB_FREE  0
#define FDCB_ALLOC 1
#define FDCB_CHECK 2

/* syslog variables (see netinclude:sys/syslog.h for values) */
#define SBTC_LOGSTAT		10
#define SBTC_LOGTAGPTR		11
#define SBTC_LOGFACILITY	12
#define SBTC_LOGMASK		13

/*
 * The argument of following error string tags is a ULONG,
 * where the error number is stored. On return the string pointer is 
 * returned on this same ULONG. (GET ONLY)
 *
 * NOTE: error numbers defined in <exec/errors.h> are negative and must be
 * negated (turned to positive) before passing to the SocketBaseTagList().
 */
#define SBTC_ERRNOSTRPTR	14 /* <sys/errno.h> */
#define SBTC_HERRNOSTRPTR	15 /* <netdb.h> */
#define SBTC_IOERRNOSTRPTR	16 /* <exec/errors.h> SEE NOTE ABOVE */
#define SBTC_S2ERRNOSTRPTR	17 /* <devices/sana2.h> */
#define SBTC_S2WERRNOSTRPTR	18 /* <devices/sana2.h> */


/* errno pointer & size SETTING (only) */
#define SBTC_ERRNOBYTEPTR	21
#define SBTC_ERRNOWORDPTR	22
#define SBTC_ERRNOLONGPTR	24
/*
 * Macro for generating the errnoptr tag code from a (constant) size.
 * only 1,2 & 4 are legal 'size' values. If the 'size' value is illegal,
 * the tag is set to 0, which causes SocketBaseTagList() to fail.
 */
#define SBTC_ERRNOPTR(size)    ((size == sizeof(int)) ? SBTC_ERRNOLONGPTR   :\
				((size == sizeof(short)) ? SBTC_ERRNOWORDPTR :\
				 ((size == sizeof(char)) ? SBTC_ERRNOBYTEPTR :\
				  0)))

/* h_errno pointer */
#define SBTC_HERRNOLONGPTR	25

/* release string pointer (read only!) */
#define SBTC_RELEASESTRPTR 29

#ifdef notyet
/*
 * Different boolean variables
 */
/* use 4.3BSD compatible sockaddr structures */
#define SBTC_COMPAT43           30
#endif

#endif /* !BSDSOCKET_SOCKETBASETAGS_H */

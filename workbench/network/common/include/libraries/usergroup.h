#ifndef LIBRARIES_USERGROUP_H
#define LIBRARIES_USERGROUP_H
/*
 * Definitions of AmiTCP/IP usergroup.library for 32 bit C compilers
 *
 * Copyright © 1994-2002 AmiTCP/IP Group & The MorphOS Team
 * Network Solutions Development, Inc.
 * All rights reserved.
 *
 * $Id$
 */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef _PWD_H_
#include <pwd.h>
#endif 
#ifndef _GRP_H_
#include <grp.h>
#endif
#ifndef _UTMP_H_
#include <utmp.h>
#endif


#define USERGROUPNAME "AmiTCP:libs/usergroup.library"

/* Extended password encryption begins with underscore */
#define _PASSWORD_EFMT1 '_'

/* Maximum length for password */
#define _PASSWORD_LEN   128

/* A user can belong to NGROUPS different groups */
#define NGROUPS 32

/* Max length of a login name */
#define MAXLOGNAME      32

/* Credentials of a process. Uses a 16-bit alignement for compatibility reasons */
#ifdef __GNUC__
#pragma pack(2)
#endif
struct UserGroupCredentials {
	uid_t           cr_ruid;
	gid_t           cr_rgid;
	unsigned short  cr_umask;             /* umask (mode_t) */
	uid_t           cr_euid;
	short           cr_ngroups;           /* number of groups */
	gid_t           cr_groups[NGROUPS];
	struct Task     *cr_session;          /* pid_t (mode) */
	char            cr_login[MAXLOGNAME]; /* setlogin() name */
};
#ifdef __GNUC__
#pragma pack()
#endif

/*
 * ID conversion macros
 */
#define UG2MU(id) ((id) == 0 ? 65535 : (id) == -2 ? 0 : (id))
#define MU2UG(id) ((id) == 65535 ? 0L : (id) == 0L ? -2L : (id))

/*
 * Context tags
 */
#define UGT_ERRNOBPTR 0x80000001
#define UGT_ERRNOWPTR 0x80000002
#define UGT_ERRNOLPTR 0x80000004
#define UGT_ERRNOPTR(size)\
  ((size == 4) ? UGT_ERRNOLPTR :\
   (size == 2) ? UGT_ERRNOWPTR :\
   (size == 1) ? UGT_ERRNOBPTR : 1L)
#define UGT_OWNER     0x80000011
#define UGT_INTRMASK  0x80000010


#endif /* !LIBRARIES_USERGROUP_H */

#ifndef _PWD_H_
#define	_PWD_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/_types.h>
#include <sys/cdefs.h>

/*
    Implementation Note:
    You do not need to define size_t in this file. (POSIX)
*/

#ifndef __AROS_GID_T_DECLARED
#define __AROS_GID_T_DECLARED
typedef __gid_t     gid_t;
#endif

#ifndef __AROS_UID_T_DECLARED
#define __AROS_UID_T_DECLARED
typedef __uid_t     uid_t;
#endif

struct passwd
{
  char  *pw_name;    /* Username */
  char  *pw_passwd;  /* Password */
  uid_t  pw_uid;     /* User ID */
  gid_t  pw_gid;     /* Group ID */
  char  *pw_gecos;   /* Real name */
  char  *pw_dir;     /* Home directory  */
  char  *pw_shell;   /* Shell */
};

__BEGIN_DECLS

struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);

#if __POSIX_VISIBLE >= 200112 || __XSI_VISIBLE >= 500
/* NOTIMPL int getpwnam_r(const char *name, struct passwd *pwd, char *buffer,
        size_t bufsize, struct passwd **result); */
/* NOTIMPL int getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize,
        struct passwd **result); */
#endif

#if __XSI_VISIBLE
void endpwent(void);
struct passwd *getpwent(void);
void setpwent(void);
#endif

__END_DECLS

#endif /* _PWD_H_ */

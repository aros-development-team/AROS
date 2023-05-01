#ifndef _POSIXC_PWD_H_
#define	_POSIXC_PWD_H_

/*
    Copyright © 2003-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/system.h>

#include <aros/types/size_t.h>
#include <aros/types/gid_t.h>
#include <aros/types/uid_t.h>

struct passwd
{
  char  *pw_name;    /* Username */
  uid_t  pw_uid;     /* User ID */
  gid_t  pw_gid;     /* Group ID */
  char  *pw_dir;     /* Home directory  */
  char  *pw_shell;   /* Shell */
  char  *pw_passwd;  /* Password */
  char  *pw_gecos;   /* Real name */
};

__BEGIN_DECLS

void endpwent(void);
struct passwd *getpwent(void);
struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);
/* NOTIMPL int getpwnam_r(const char *name, struct passwd *pwd, char *buffer,
        size_t bufsize, struct passwd **result); */
/* NOTIMPL int getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize,
        struct passwd **result); */
void setpwent(void);

__END_DECLS

#endif /* _POSIXC_PWD_H_ */

#ifndef _PWD_H_
#define	_PWD_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>

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

#endif /* _PWD_H_ */

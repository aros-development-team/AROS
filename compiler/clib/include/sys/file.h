#ifndef _SYS__FILE_H_
#define _SYS__FILE_H_
/*
    Copyright � 2008, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: 4.4BSD header file sys/stat.h
    Lang: english
*/

/* flock() operations */

#ifndef SYS_TYPES_H
#include <sys/types.h>
#endif

#define LOCK_SH 1       /* Shared lock.  */
#define LOCK_EX 2       /* Exclusive lock.  */
#define LOCK_UN 8       /* Unlock.  */

/* operation modifiers (combined with OR) */

#define LOCK_NB 4       /* Non-blocking operation */

/* function declarations */

__BEGIN_DECLS

int flock (int fd, int operation);

__END_DECLS

#endif /* _SYS__FILE_H_ */

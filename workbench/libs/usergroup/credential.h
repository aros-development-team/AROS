/* 
 * credential.h -- credential resource
 *
 * Original Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Based upon usergroup.library from AmiTCP/IP.
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#include <sys/types.h>
#include <aros/types/pid_t.h>

#define CREDENTIALNAME "credential.resource"
#define _PATH_CREDENTIAL "LIBS:credential.resource"

#define UG_CredRes  0xCAED

/*
 * Credentials
 */
struct ucred {
  short   cr_ref;		/* reference count */
  short   cr_ngroups;		/* number of groups */
  uid_t   cr_uid;		/* effective user id */
  gid_t   cr_groups[NGROUPS];	/* groups */
};

#define cr_gid cr_groups[0]

/*
 * Process owner
 */
struct	pcred {
  struct ucred *pc_ucred;	/* current credentials */
  uid_t         p_ruid;		/* real user id */
  gid_t         p_rgid;		/* real group id */
  short	        p_refcnt;	/* number of references */
};

#define p_ngroups pc_ucred->cr_ngroups
#define p_groups  pc_ucred->cr_groups
#define p_euid    pc_ucred->cr_uid
#define p_egid    pc_ucred->cr_gid

/*
 * Constant IDs
 */
#define ROOT_UID 0
#define NOID    -1
#define NOBODY  -2

#define ISROOT(p) ((p)->p_euid == ROOT_UID)

struct session {
  pid_t           s_leader;
  struct MsgPort *s_consoletask;
  char            s_login[MAXLOGNAME];    /* setlogin() name */
};

/*
 * This have got some 4.4BSD proc fields
 */
struct proc {
  struct session  p_session[1];
  struct pcred    p_cred[1];
  mode_t          p_umask;
};

#define p_ucred   p_cred->pc_ucred

struct CredentialResource {
  struct Library r_Lib;
  struct proc    r_proc[1];	/* global "process" */
  struct ucred   r_ucred[1];
  struct lastlog r_lastlog[1];
  struct utmp    r_utmp[1];
  UBYTE          r_name[((sizeof(CREDENTIALNAME) +3) & ~3)];
  UBYTE          r_vstring[1];
};

struct CredentialResource *CredentialInit(const char *name);

extern struct CredentialResource *CredentialBase;
struct proc *procfind(pid_t pid);
struct ucred *crcopy(struct ucred *cred);

#ifdef notyet
struct ucred *crget(void);
void crfree(struct ucred *cr);
struct proc *proccopy(struct proc *, pid_t new);
#pragma libcall CredentialBase procfind  6 001
#pragma libcall CredentialBase proccopy  C 0902
#pragma libcall CredentialBase crcopy   12 901
#pragma libcall CredentialBase crget    18 00
#pragma libcall CredentialBase crfree   1E 901
#endif

int suser(struct ucred *cred);

/* These macros have typechecking */
#define MinRemove(mn) do { struct MinNode *_mnp=(mn);Remove((struct Node*)_mnp); } while(0)
#define MinAddHead(ml, mn) do { struct MinList *_mlp=(ml); struct MinNode *_mnp=(mn); AddHead((struct List*)_mlp, (struct Node*)_mnp); } while(0)

#define lock(x)     
#define unlock(x)   


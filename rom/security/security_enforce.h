#ifndef _SECURITY_ENFORCE_H
#define _SECURITY_ENFORCE_H

#include <dos/dosextens.h>

/* Flags for muVolume */

#define secFSE_TRUE_MUFS                (0)	/* A true MUFS volume */
#define secFSE_ENFORCED                 (1)	/* Enforced by the packet interceptor */
#define secFSE_READONLY                 (2)	/* This FS is read only */
#define secFSE_NOSUID                   (4)	/* Do not allow setuid from this volume */

/* Default Protections for the root directory */

#define secFSE_DEF_ROOTPROTECTION       (FIBF_OTR_READ|FIBF_GRP_READ|FIBF_DELETE|FIBF_EXECUTE)

#define secFSE_DEF_ROOTOWNER            secOWNER_SYSTEM

/* The name of the FSTAB file */

#define secFSE_FSTAB_FILENAME           "fstab"

/* Handle Despatch */

typedef BOOL (*PKTFUNC)(struct secVolume *,struct DosPacket*, struct secExtOwner*);

struct secFSE_PktHandler {
    LONG        action;
    PKTFUNC     func;
};

/*
 *      Private Function Prototypes
 */

extern void ReadFSTab(struct SecurityBase *secBase);
extern BOOL BootStrapRendevous(struct SecurityBase *secBase);
extern LONG IsAllowed(struct SecurityBase *secBase, struct secVolume *Vol,struct secExtOwner *task, 
		ULONG object, LONG prot, LONG access);

#endif /* _SECURITY_ENFORCE_H */

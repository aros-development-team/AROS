#ifndef _AROS_HOSTTHREAD_H
#define _AROS_HOSTTHREAD_H

#ifdef __AROS__
#include <exec/nodes.h>
#else

/* Just to let the compiler know what is it */
struct MinNode
{
    struct MinNode * mln_Succ,
		   * mln_Pred;
};

#define HT_GetMsg()		THandle->HTIFace->GetMsg(THandle)
#define HT_CauseInterrupt(data) THandle->HTIFace->CauseInterrupt(THandle, data)

#endif

struct ThreadHandle;

struct HostThreadInterface
{
    void *(*GetMsg)(struct ThreadHandle *th);
    void *(*CauseInterrupt)(struct ThreadHandle *th, void *data);
};

struct ThreadHandle
{
    struct MinNode node;				/* A node to add the handle to internal list			   */
    void *handle;					/* Host OS raw thread handle, can be used to manipulate the thread */
    unsigned long id;					/* Thread ID, another thing used in some OSes			   */
    struct HostThreadInterface *HTIFace;		/* A pointer to host-side API					   */
    unsigned long (*entry)(struct ThreadHandle *th);	/* Thread entry point						   */
    /* Private data follows, do not rely on the size! */
};

#endif

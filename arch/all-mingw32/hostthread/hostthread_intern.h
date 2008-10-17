#ifndef HOSTTHREAD_INTERN_H
#define HOSTTHREAD_INTERN_H

#include <aros/hostthread.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>

struct ThreadNode {
    struct ThreadHandle th;
    struct List intservers;
};

struct MyHTInterface
{
    struct HostThreadInterface hostside;
    APTR (*CreateNewThread)(struct ThreadNode *tn);
    ULONG (*KillThread)(struct ThreadNode *tn);
    ULONG (*PutThreadMsg)(struct ThreadNode *tn, void *msg);
};

struct HostThreadBase {
    struct Node hlb_Node;
    void *HTLib;
    struct MyHTInterface *HTIFace;
    struct SignalSemaphore sem;
    struct MinList threads_list;
};

#endif

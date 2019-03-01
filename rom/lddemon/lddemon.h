#ifndef LDDEMON_H
#define LDDEMON_H

#include <aros/config.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>

struct IntLDDemonBase
{
    /* Public*/
    struct Node		        node;
    struct Library          *dl_DOSBase;
    struct List             dl_Flavours; // List of the flavours (extensions) supported by LDLoadSeg Function.

    /* Private*/
    struct SignalSemaphore  dl_LDObjectsListSigSem;
    struct List             dl_LDObjectsList;

    struct Interrupt	    dl_LDHandler;

    struct MsgPort	        *dl_LDDemonPort;
    struct Process	        *dl_LDDemonTask;

#if defined(__AROSEXEC_SMP__)
    struct Library 	        *dl_ExecLockRes;
#endif

    struct Library 	        *(*__OpenLibrary)();
    BYTE		            (*__OpenDevice)();

    ULONG		            dl_LDReturn;
};

#endif /* LDDEMON_H */

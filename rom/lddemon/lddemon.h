#include <aros/config.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>

struct LDDemonBase
{
    struct Node		   node;
    struct SignalSemaphore dl_LDObjectsListSigSem;
    struct List            dl_LDObjectsList;
    struct Interrupt	   dl_LDHandler;
    struct MsgPort	 * dl_LDDemonPort;
    struct Process	 * dl_LDDemonTask;
#if defined(__AROSEXEC_SMP__)
    struct Library 	 * dl_ExecLockRes;
#endif
    struct Library 	 * (*__OpenLibrary)();
    BYTE		   (*__OpenDevice)();
    ULONG		   dl_LDReturn;

    struct Library       * dl_DOSBase;
};

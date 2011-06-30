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
    ULONG		   dl_LDReturn;
};

/* 
 * This is actually a sample file, which definitely will be overriden
 * for every architecture.
 */

#include <exec/semaphores.h>

#define EXCEPTIONS_COUNT 1
#define IRQ_COUNT        1

struct KernelBase
{
    struct Node            kb_Node;
    struct MinList         kb_Exceptions[EXCEPTIONS_COUNT];
    struct MinList         kb_Interrupts[IRQ_COUNT];
    struct MinList         kb_Modules;
    struct MinList	  *kb_KernelModules;
    struct SignalSemaphore kb_ModSem;
};

#include <exec/types.h>
#include <exec/alerts.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <asm/cpu.h>

#if defined(__x86_64__)
#include <aros/x86_64/cpucontext.h>
#elif defined(__i386__)
#include <aros/i386/cpucontext.h>
#else
#error "Unsupported x86 architecture"
#endif

#include "defs.h"

static APTR TrapHandlerHandle;

static BOOL ShouldHandleFault(IPTR fault_address, struct ExceptionContext *regs)
{
  if ((regs->cs & 0x3) == 0)
    return FALSE;

  if (PageHandlerTask == NULL)
    return FALSE;

  if (fault_address < (IPTR)VirtAddrStart || fault_address >= (IPTR)VirtAddrEnd)
    return FALSE;

  return TRUE;
}

static int TrapPageFault(void *ctx, void *handlerData, void *handlerData2)
{
  struct ExceptionContext *regs = (struct ExceptionContext *)ctx;
  IPTR fault_address = (IPTR)rdcr(cr2);
  struct TrapStruct *this_fault;
  struct Task *this_task;
  LONG wake_signal;
  ULONG wait_mask;

  (void)handlerData;
  (void)handlerData2;

  if (!ShouldHandleFault(fault_address, regs))
    return 0;

  this_task = FindTask(NULL);
  if (this_task == NULL)
    return 0;

  Forbid ();
  this_fault = (struct TrapStruct*)RemHead (&Free);
  Permit ();

  if (this_fault == NULL)
    {
    Alert (NoTrapStructsAlertNum);
    return 1;
    }

  wake_signal = AllocSignal (-1L);
  if (wake_signal < 0)
    wake_signal = EMERGENCY_SIGNAL;

  this_fault->FaultTask = this_task;
  this_fault->FaultAddress = fault_address;
  this_fault->TopOfStackFrame = NULL;
  this_fault->PageDescrAddr = NULL;
  this_fault->TableDescrAddr = NULL;
  this_fault->PhysAddr = 0;
  this_fault->WakeupSignal = (UWORD)wake_signal;
  this_fault->FramesRemoved = 0;
  this_fault->RemFrameFlags = 0;
  this_fault->FD = NULL;

  Forbid ();
  AddTail (&PageReq, (struct Node*)this_fault);
  Permit ();

  Signal (PageHandlerTask, 1UL << PageFaultSignal);

  wait_mask = 1UL << wake_signal;
  if (OrigWait != NULL)
    (*OrigWait) (wait_mask);
  else
    Wait (wait_mask);

  if (wake_signal != EMERGENCY_SIGNAL)
    FreeSignal ((ULONG)wake_signal);

  return 1;
}

BOOL VMMInstallTrapHandler(void)
{
  APTR KernelBase = OpenResource ("kernel.resource");

  if (KernelBase == NULL)
    return FALSE;

  TrapHandlerHandle = KrnAddExceptionHandler (14, TrapPageFault, KernelBase, NULL);
  return (TrapHandlerHandle != NULL);
}

void VMMRemoveTrapHandler(void)
{
  if (TrapHandlerHandle != NULL)
    {
    KrnRemExceptionHandler (TrapHandlerHandle);
    TrapHandlerHandle = NULL;
    }
}

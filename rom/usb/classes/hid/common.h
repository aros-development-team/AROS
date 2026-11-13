
#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/initializers.h>

#include <devices/timer.h>
#include <devices/input.h>
#include <utility/utility.h>
#include <dos/dos.h>
#include <intuition/intuition.h>

#include <devices/usb.h>
#include <devices/usbhardware.h>
#include <libraries/usbclass.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

/* Use the inline varargs NewObject() so that the alib stub is
   not linked in. The class is kickstart resident and initialised before
   intuition.library exists. */
#define INTUITION_INLINE_NEWOBJECT
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/commodities.h>
#include <proto/intuition.h>
#include <proto/poseidon.h>
#include <proto/utility.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/input.h>
#include <proto/expansion.h>
#include <proto/exec.h>
#include <proto/muimaster.h>

#define NewList NEWLIST

#include <stdarg.h>

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

/* Tasks that feed input.device must sit above DOS handlers (5) and the band
   dynamic schedulers manage. With IND_ADDEVENT, input.device (pri 20)
   auto-repeats a key until *this* task delivers the key-up, and only this task
   re-arms the interrupt IN pipe; a busy handler at 5 could hold both back for
   seconds. A floor, not an override: a higher pgc_SubTaskPri wins. */
#define INPUT_CLASS_TASK_PRI 10

static inline void nApplyInputTaskPriFloor(void)
{
    struct Task *task = FindTask(NULL);
    if(task->tc_Node.ln_Pri < INPUT_CLASS_TASK_PRI)
    {
        SetTaskPri(task, INPUT_CLASS_TASK_PRI);
    }
}

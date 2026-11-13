
#include LC_LIBDEFS_FILE

#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <exec/alerts.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/execbase.h>
#include <exec/initializers.h>
#include <exec/interrupts.h>
#include <exec/io.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/resident.h>
#include <exec/semaphores.h>
#include <exec/types.h>

#include <devices/timer.h>
#include <dos/dos.h>
#include <utility/utility.h>

#include <devices/usb.h>
#include <devices/usbhardware.h>
#include <libraries/usbclass.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/utility.h>

#define NewList NEWLIST

#include <stdarg.h>

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))


#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/errors.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <dos/dos.h>

#include <devices/usb.h>
#include <devices/usbhardware.h>
#include <devices/usb_video.h>
#include <libraries/usbclass.h>

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/poseidon.h>
#include <proto/utility.h>

#define NewList NEWLIST

#include <stdarg.h>

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

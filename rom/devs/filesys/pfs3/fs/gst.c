/*
   Use this file to create a Master GST of all the non-conflicting headers
   in Include:.  The following compiler command will create the most often
   used version of the Master GST

        sc noobjname mgst=include:all.gst all.c

   Remember that you may need to make new GST's depending on your compiler
   switches used in a project.  Some switches affect the GST as well as
   your code.  For instance, if you use the UNSIGNEDCHAR switch, you will
   need to build a GST specifically for use with this model

        sc noobjname uchar mgst=include:all_u.gst all.c
*/

#define __USE_SYSBASE
#define USE_BUILTIN_MATH

#include <string.h>
#include <math.h>
#include <ctype.h>
#include <dirent.h>
#include <dos.h>
#include <errno.h>
#include <error.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <stat.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <devices/bootblock.h>
#include <devices/cd.h>
#include <devices/console.h>
#include <devices/hardblocks.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <devices/scsidisk.h>
#include <devices/timer.h>
#include <devices/trackdisk.h>
#include <dos/datetime.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include <dos/doshunks.h>
#include <dos/dostags.h>
#include <dos/exall.h>
#include <dos/filehandler.h>
#include <dos/notify.h>
#include <dos/rdargs.h>
#include <dos/record.h>
#include <dos/stdio.h>
#include <dos/var.h>
#include <exec/alerts.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/exec.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/io.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/ports.h>
#include <exec/resident.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <exec/types.h>
#include <hardware/blit.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <libraries/asl.h>
#include <libraries/iffparse.h>
#include <libraries/lowlevel.h>
#include <libraries/nonvolatile.h>
#include <proto/asl.h>
#include <proto/disk.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/input.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/lowlevel.h>
#include <proto/nonvolatile.h>
#include <proto/timer.h>
#include <proto/utility.h>
#include <proto/wb.h>
#include <resources/filesysres.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <utility/utility.h>

#include <clib/alib_protos.h>       /*  MH  */
#include <clib/multiuser_protos.h>  /*  MH  */
#include <libraries/multiuser.h>    /*  MH  */
#include <proto/multiuser.h>        /*  MH  */
#include <pragmas/multiuser.h>      /*  MH  */

#include "debug.h"                  /*  MH  */


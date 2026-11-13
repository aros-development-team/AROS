/* device.h: */
#ifndef ACDR_DEVICE_H
#define ACDR_DEVICE_H

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <devices/timer.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <utility/date.h>

#include "generic.h"

typedef struct Interrupt	INTERRUPT;
typedef struct Task		TASK;
typedef struct FileLock 	LOCK;	    /*	See LOCKLINK	*/
typedef struct FileInfoBlock	FIB;
typedef struct DosPacket	PACKET;
typedef struct Process		PROC;
typedef struct DeviceNode	DEVNODE;
typedef struct DosInfo		DOSINFO;
typedef struct FileHandle	FH;
typedef struct MsgPort		PORT;
typedef struct Message		MSG;
typedef struct MinList		LIST;
typedef struct MinNode		NODE;
typedef struct DateStamp	STAMP;
typedef struct InfoData 	INFODATA;

#define FILE_DIR    1
#define FILE_FILE   -1

#endif /* ACDR_DEVICE_H */


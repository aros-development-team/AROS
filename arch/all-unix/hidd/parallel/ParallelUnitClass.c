/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parallel Unit hidd class implementation.
    Lang: english
*/

#define __OOP_NOATTRBASES__

/* Some POSIX includes */
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

/* the rest are Amiga includes */
#define timeval aros_timeval

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/alib.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/lists.h>
#include <utility/tagitem.h>
#include <hidd/parallel.h>
#include <hidd/unixio.h>

#include <aros/symbolsets.h>

#include "parallel_intern.h"
#undef timeval

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

void parallelunit_io(int fd, int mode, void *ptr);

static char * unitname[] =
{
	"/dev/lp0",
	"/dev/lp1",
	"/dev/lp2"
};

/*************************** Classes *****************************/

static OOP_AttrBase HiddParallelUnitAB;
static OOP_AttrBase HiddUnixIOAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_ParallelUnit, &HiddParallelUnitAB },
    { IID_Hidd_UnixIO      , &HiddUnixIOAttrBase },
    { NULL,	NULL }
};

/******* ParallelUnit::New() ***********************************/
OOP_Object *UXParUnit__Root__New(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
	struct HIDDParallelUnitData * data;
	const struct TagItem tags[] = {
	    {aHidd_UnixIO_Opener      , (IPTR)"parallel.hidd"},
	    {aHidd_UnixIO_Architecture, (IPTR)AROS_ARCHITECTURE},
	    {TAG_END, 0}
	};
	struct TagItem *tag, *tstate;
	ULONG unitnum = 0;
	
	EnterFunc(bug("ParallelUnit::New()\n"));

	tstate = msg->attrList;
	while ((tag = NextTagItem(&tstate))) {
		ULONG idx;

		if (IS_HIDDPARALLELUNIT_ATTR(tag->ti_Tag, idx)) {
			switch (idx) {
				case aoHidd_ParallelUnit_Unit:
					unitnum = (ULONG)tag->ti_Data;
				break;
			}
		}

	} /* while (tags to process) */
		
	D(bug("!!!!Request for unit number %d\n",unitnum));

	obj = (OOP_Object *)OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);

	if (obj)
	{
	    data = OOP_INST_DATA(cl, obj);

	    data->unitnum = unitnum;

	    data->unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);
	    if (NULL != data->unixio)
	    {
		D(bug("Opening %s.\n",unitname[data->unitnum]));

		data->unixio_int.fd = Hidd_UnixIO_OpenFile(data->unixio, unitname[data->unitnum], O_NONBLOCK|O_RDWR, 0, NULL);

		D(bug("Opened %s on handle %d\n",unitname[data->unitnum], data->unixio_int.fd));

		if (-1 != data->unixio_int.fd)
		{
		    int error;

		    data->unixio_int.mode = vHidd_UnixIO_RW;
		    data->unixio_int.handler = parallelunit_io;
		    data->unixio_int.handlerData = data;

		    D(bug("Adding UnixIO AsyncIO interrupt!\n"));
		    error = Hidd_UnixIO_AddInterrupt(data->unixio, &data->unixio_int);

		    if (!error)
			ReturnPtr("ParallelUnit::New()", OOP_Object *, obj);

		    Hidd_UnixIO_CloseFile(data->unixio, data->unixio_int.fd, NULL);
		}
		/* There's no need to dispose UnixIO object */
	    }

	    OOP_DisposeObject(obj);
	} /* if (obj) */

	ReturnPtr("ParallelUnit::New()", OOP_Object *, NULL);
}

/******* ParallelUnit::Dispose() ***********************************/
OOP_Object *UXParUnit__Root__Dispose(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
	struct HIDDParallelUnitData * data;
	EnterFunc(bug("ParallelUnit::Dispose()\n"));

	data = OOP_INST_DATA(cl, obj);
	D(bug("Freeing filedescriptor (%d)!\n",data->unixio_int.fd));

	if (-1 != data->unixio_int.fd)
	{ 
		Hidd_UnixIO_RemInterrupt(data->unixio, &data->unixio_int);
		Hidd_UnixIO_CloseFile(data->unixio, data->unixio_int.fd, NULL);
	}
	OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
	ReturnPtr("ParallelUnit::Dispose()", OOP_Object *, obj);
}

/******* ParallelUnit::Init() **********************************/
BOOL UXParUnit__Hidd_ParallelUnit__Init(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Init *msg)
{
	struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);
	
	EnterFunc(bug("ParallelUnit::Init()\n"));
	data->DataReceivedCallBack = msg->DataReceived;
	data->DataReceivedUserData = msg->DataReceivedUserData;
	data->DataWriteCallBack		= msg->WriteData;
	data->DataWriteUserData		= msg->WriteDataUserData;

	ReturnBool("ParallelUnit::Init()", TRUE);
}

/******* ParallelUnit::Write() **********************************/
ULONG UXParUnit__Hidd_ParallelUnit__Write(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Write *msg)
{
	struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);
	ULONG len = 0;
	
	EnterFunc(bug("ParallelUnit::Write()\n"));

	if (TRUE == data->stopped)
		return 0;

	D(bug("Writing %d bytes to fd %d (stream: %s)\n",
				msg->Length,
				data->unixio_int.fd,
				msg->Outbuffer));

	len = Hidd_UnixIO_WriteFile(data->unixio, data->unixio_int.fd, msg->Outbuffer, msg->Length, NULL);

	ReturnInt("ParallelUnit::Write()",ULONG, len);
}

/******* ParallelUnit::Start() **********************************/
VOID UXParUnit__Hidd_ParallelUnit__Start(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Start *msg)
{
	struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);

	/*
	 * Allow or start feeding the UART with data. Get the data
	 * from upper layer.
	 */
	if (TRUE == data->stopped) { 
		if (NULL != data->DataWriteCallBack)
			 data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);
		/*
		 * Also mark the stopped flag as FALSE.
		 */
		data->stopped = FALSE;
	}
}  

/******* ParallelUnit::Stop() **********************************/
VOID UXParUnit__Hidd_ParallelUnit__Stop(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Stop *msg)
{
	struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);

	/*
	 * The next time the interrupt comes along and asks for
	 * more data we just don't do anything...
	 */
	data->stopped = TRUE;
}

/****** ParallelUnit::GetStatus ********************************/
UWORD UXParUnit__Hidd_ParallelUnit__GetStatus(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_GetStatus *msg)
{
#if 0
	struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);
#endif

	return 0;
}


/************* The software interrupt handler that gets data from PORT *****/


#define READBUFFER_SIZE 513

void parallelunit_io(int fd, int mode, void *ptr)
{
    struct HIDDParallelUnitData *data = ptr;

    if (mode & vHidd_UnixIO_Read)
    {
    	ssize_t len;
    	UBYTE buffer[READBUFFER_SIZE];

	/*
	** Read the data from the port ...
	*/
	len = Hidd_UnixIO_ReadFile(data->unixio, fd, buffer, READBUFFER_SIZE, NULL);

	/*
	** ... and deliver them to whoever is interested. 
	*/
	if (NULL != data->DataReceivedCallBack)
		data->DataReceivedCallBack(buffer, len, data->unitnum, data->DataReceivedUserData);

    }

    if (mode & vHidd_UnixIO_Write)
    {
	if (NULL != data->DataWriteCallBack)
		data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);
    }
}

/******* init_parallelunitclass ********************************/

static int UXParUnit_InitAttrBases(LIBBASETYPEPTR LIBBASE)
{
    return OOP_ObtainAttrBases(attrbases);
}


static int UXParUnit_ExpungeAttrBases(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

ADD2INITLIB(UXParUnit_InitAttrBases, 0)
ADD2EXPUNGELIB(UXParUnit_ExpungeAttrBases, 0)

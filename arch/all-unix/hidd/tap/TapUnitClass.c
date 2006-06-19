/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id: TapUnitClass.c 23835 2005-12-20 14:43:30Z stegerg $

    Desc: Tap Unit hidd class implementation.
    Lang: english
*/

#define __OOP_NOATTRBASES__

/* Some POSIX includes */
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include "unix_funcs.h"


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
#include <hidd/tap.h>
#include <hidd/unixio.h>

#include <aros/symbolsets.h>
#include <aros/macros.h>


#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

void tapunit_receive_data();
void tapunit_write_more_data();

#define MAX(a,b) (((a) > (b))?(a):(b))
#define MIN(a,b) (((a) > (b))?(b):(a))


#include "tap_intern.h"
#undef timeval

static char * unitname[] =
{
	"aros0",
	"aros1",
	"aros2"
};

/*************************** Classes *****************************/

static OOP_AttrBase HiddTapUnitAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_TapUnit, &HiddTapUnitAB },
    { NULL,	NULL }
};

/******* TapUnit::New() ***********************************/
OOP_Object *UXTapUnit__Root__New(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
	struct HIDDTapUnitData * data;
	static const struct TagItem tags[] = {{ TAG_END, 0}};
	struct TagItem *tag, *tstate;
	ULONG unitnum = 0;
	
	EnterFunc(bug("TapUnit::New()\n"));

	tstate = msg->attrList;
	while ((tag = NextTagItem((const struct TagItem **)&tstate))) {
		ULONG idx;

		if (IS_HIDDTAPUNIT_ATTR(tag->ti_Tag, idx)) {
			switch (idx) {
				case aoHidd_TapUnit_Unit:
					unitnum = (ULONG)tag->ti_Data;
				break;
			}
		}

	} /* while (tags to process) */
		
	D(bug("!!!!Request for unit number %d\n",unitnum));

	obj = (OOP_Object *)OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);

	if (obj) {
		data = OOP_INST_DATA(cl, obj);
		
		data->unitnum = unitnum;

		D(bug("Creating network device %s.\n",unitname[data->unitnum]));

		data->filedescriptor = unix_open_nonblock("/dev/net/tun");
		if (data->filedescriptor < 0)
			data->filedescriptor = unix_open_nonblock("/dev/tap");

		D(bug("Opened %s on handle %d\n",unitname[data->unitnum], data->filedescriptor));
		
		if (-1 != data->filedescriptor) {
			struct ifreq ifr;
			memset(&ifr, 0, sizeof(ifr));
			ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
			strncpy(ifr.ifr_name, unitname[data->unitnum], IFNAMSIZ);
			
			/*
			** Configure the tty driver ?!?!?!
			*/
			{
				data->replyport_read = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC|MEMF_CLEAR);
				data->replyport_write= AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC|MEMF_CLEAR);

				if (data->replyport_read && data->replyport_write) {
					/*
					** Init the msg ports. They don't need a signal to be allocated
					*/
					NEWLIST(&data->replyport_read->mp_MsgList);
					data->replyport_read ->mp_Node.ln_Type = NT_MSGPORT;

					NEWLIST(&data->replyport_write->mp_MsgList);
					data->replyport_write->mp_Node.ln_Type = NT_MSGPORT;

					data->softint_read	= AllocMem(sizeof(struct Interrupt), MEMF_CLEAR);
					data->softint_write = AllocMem(sizeof(struct Interrupt), MEMF_CLEAR);

					if (data->softint_read && data->softint_write) {
						data->softint_read->is_Data = data;
						data->softint_read->is_Code = tapunit_receive_data;

						data->softint_write->is_Data = data;
						data->softint_write->is_Code = tapunit_write_more_data;

						data->replyport_read->mp_Flags = PA_SOFTINT;
						data->replyport_read->mp_SoftInt = data->softint_read;

						data->replyport_write->mp_Flags = PA_SOFTINT;
						data->replyport_write->mp_SoftInt = data->softint_write;

						data->unixio_read	= OOP_NewObject(NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);
						data->unixio_write = OOP_NewObject(NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);

						if (NULL != data->unixio_read && NULL != data->unixio_write)
						{
							Hidd_UnixIO_IOControlFile(data->unixio_read, 
							    data->filedescriptor,
							    TUNSETIFF,
							    &ifr,
							    NULL);
							    
							ULONG error;
							D(bug("Creating UnixIO AsyncIO command!\n"));

							error = Hidd_UnixIO_AsyncIO(data->unixio_read,
							                            data->filedescriptor,
							                            vHidd_UnixIO_Terminal,
							                            data->replyport_read,
							                            vHidd_UnixIO_Read | vHidd_UnixIO_Keep,
							                            SysBase);

							error = Hidd_UnixIO_AsyncIO(data->unixio_write,
							                            data->filedescriptor,
							                            vHidd_UnixIO_Terminal,
							                            data->replyport_write,
							                            vHidd_UnixIO_Write | vHidd_UnixIO_Keep,
							                            SysBase);
							goto exit;

						}

						if (NULL != data->unixio_read)
							OOP_DisposeObject(data->unixio_read);

						if (NULL != data->unixio_write)
							OOP_DisposeObject(data->unixio_write);
					}
					
					if (data->softint_read) 
						FreeMem(data->softint_read, sizeof(struct Interrupt));
					if (data->softint_write)
						FreeMem(data->softint_write, sizeof(struct Interrupt));
				}
				
				if (data->replyport_read)
					FreeMem(data->replyport_read , sizeof(struct MsgPort));
				if (data->replyport_write)
					FreeMem(data->replyport_write, sizeof(struct MsgPort));

			} 
			
			close(data->filedescriptor);	
		}

		OOP_DisposeObject(obj);
		obj = NULL;
	} /* if (obj) */

	D(bug("%s - an error occurred!\n",__FUNCTION__));

exit:
	ReturnPtr("TapUnit::New()", OOP_Object *, obj);
}

/******* TapUnit::Dispose() ***********************************/
OOP_Object *UXTapUnit__Root__Dispose(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
	struct HIDDTapUnitData * data;
	EnterFunc(bug("TapUnit::Dispose()\n"));

	data = OOP_INST_DATA(cl, obj);
	D(bug("Freeing filedescriptor (%d)!\n",data->filedescriptor));

	if (-1 != data->filedescriptor) { 
		Hidd_UnixIO_AbortAsyncIO(data->unixio_read,
		                         data->filedescriptor,
		                         SysBase);

		close(data->filedescriptor);
	
		FreeMem(data->replyport_read,	sizeof(struct MsgPort));
		FreeMem(data->replyport_write, sizeof(struct MsgPort));

		FreeMem(data->softint_read , sizeof(struct Interrupt));
		FreeMem(data->softint_write, sizeof(struct Interrupt));

		OOP_DisposeObject(data->unixio_read);
		OOP_DisposeObject(data->unixio_write);
	}
	OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
	ReturnPtr("TapUnit::Dispose()", OOP_Object *, obj);
}



/******* TapUnit::Init() **********************************/
BOOL UXTapUnit__Hidd_TapUnit__Init(OOP_Class *cl, OOP_Object *o, struct pHidd_TapUnit_Init *msg)
{
	struct HIDDTapUnitData * data = OOP_INST_DATA(cl, o);
	
	EnterFunc(bug("TapUnit::Init()\n"));
	data->DataReceivedCallBack = msg->DataReceived;
	data->DataReceivedUserData = msg->DataReceivedUserData;
	data->DataWriteCallBack		= msg->WriteData;
	data->DataWriteUserData		= msg->WriteDataUserData;

	ReturnBool("TapUnit::Init()", TRUE);
}

/******* TapUnit::Write() **********************************/
ULONG UXTapUnit__Hidd_TapUnit__Write(OOP_Class *cl, OOP_Object *o, struct pHidd_TapUnit_Write *msg)
{
	struct HIDDTapUnitData * data = OOP_INST_DATA(cl, o);
	ULONG len = 0;
	
	EnterFunc(bug("TapUnit::Write()\n"));

	if (TRUE == data->stopped)
		return 0;

	D(bug("Writing %d bytes to fd %d. The stream is:",
				msg->Length,
				data->filedescriptor));
	
	D({
	    UBYTE *ptr = msg->Outbuffer;
	    int j;
	    
	    for (j=0; j < MIN(64,msg->Length); j++)
	    {
        	if ((j%16) == 0)
            	    bug("\n%03x:", j);
                bug(" %02x", ptr[j]);
    	    }
            bug("\n");
	});

	len = write(data->filedescriptor,
	            msg->Outbuffer,
	            msg->Length);


	ReturnInt("TapUnit::Write()",ULONG, len);
}

/******* TapUnit::Start() **********************************/
VOID UXTapUnit__Hidd_TapUnit__Start(OOP_Class *cl, OOP_Object *o, struct pHidd_TapUnit_Start *msg)
{
	struct HIDDTapUnitData * data = OOP_INST_DATA(cl, o);

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

/******* TapUnit::Stop() **********************************/
VOID UXTapUnit__Hidd_TapUnit__Stop(OOP_Class *cl, OOP_Object *o, struct pHidd_TapUnit_Stop *msg)
{
	struct HIDDTapUnitData * data = OOP_INST_DATA(cl, o);

	/*
	 * The next time the interrupt comes along and asks for
	 * more data we just don't do anything...
	 */
	data->stopped = TRUE;
}

/****** TapUnit::GetStatus ********************************/
UWORD UXTapUnit__Hidd_TapUnit__GetStatus(OOP_Class *cl, OOP_Object *o, struct pHidd_TapUnit_GetStatus *msg)
{
#if 0
	struct HIDDTapUnitData * data = OOP_INST_DATA(cl, o);
#endif

	return 0;
}


/************* The software interrupt handler that gets data from PORT *****/


#define READBUFFER_SIZE 2048

AROS_UFH3(void, tapunit_receive_data,
	 AROS_UFHA(APTR, iD, A1),
	 AROS_UFHA(APTR, iC, A5),
	 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
	AROS_USERFUNC_INIT

	struct HIDDTapUnitData * data = iD;
	ssize_t len;
	UBYTE buffer[READBUFFER_SIZE];

	/*
	** Read the data from the port ...
	*/
	len = read(data->filedescriptor, buffer, READBUFFER_SIZE);
	/*
	** ... and deliver them to whoever is interested. 
	*/

	D({
	    int j;
	    bug("Got %d bytes from the port.", len);
	    
	    for (j=0; j < MIN(64,len); j++)
	    {
        	if ((j%16) == 0)
            	    bug("\n%03x:", j);
                bug(" %02x", buffer[j]);
    	    }
            bug("\n");
	});

	if (NULL != data->DataReceivedCallBack)
		data->DataReceivedCallBack(buffer, len, data->unitnum, data->DataReceivedUserData);

	AROS_USERFUNC_EXIT
}

AROS_UFH3(void, tapunit_write_more_data,
	 AROS_UFHA(APTR, iD, A1),
	 AROS_UFHA(APTR, iC, A5),
	 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
	AROS_USERFUNC_INIT

	struct HIDDTapUnitData * data = iD;
//	struct Message * msg;

	/*
	** Ask for more data be written to the unit
	*/
	D(bug("Asking for more data to be written to unit %d\n",data->unitnum));

	if (NULL != data->DataWriteCallBack)
		data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);

	AROS_USERFUNC_EXIT
}


/******* init_tapunitclass ********************************/

AROS_SET_LIBFUNC(UXTapUnit_InitAttrBases, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    return OOP_ObtainAttrBases(attrbases);
    
    AROS_SET_LIBFUNC_EXIT
}


AROS_SET_LIBFUNC(UXTapUnit_ExpungeAttrBases, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(UXTapUnit_InitAttrBases, 0)
ADD2EXPUNGELIB(UXTapUnit_ExpungeAttrBases, 0)

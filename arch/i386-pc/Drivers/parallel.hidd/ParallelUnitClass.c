/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parallel Unit hidd class implementation.
    Lang: english
*/

#define __OOP_NOATTRBASES__

/* the rest are Amiga includes */
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/alib.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/lists.h>
#include <aros/symbolsets.h>

#include <utility/tagitem.h>
#include <hidd/parallel.h>
#include <hidd/unixio.h>
#include <hidd/irq.h>

#include "parallel_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

void parallelunit_receive_data();
void parallelunit_write_more_data();

static inline unsigned char inb(unsigned short port)
{

    unsigned char  _v; 

    __asm__ __volatile__
    ("inb %w1,%0"
     : "=a" (_v)
     : "Nd" (port)
    );
    
    return _v; 
} 

static inline void outb(unsigned char value, unsigned short port)
{
    __asm__ __volatile__
    ("outb %b0,%w1"
     :
     : "a" (value), "Nd" (port)
    );
}

#define parallel_usleep(x) __asm__ __volatile__("\noutb %al,$0x80\n")

static inline void parallel_out(struct HIDDParallelUnitData * data, 
                                int offset, 
                                int value)
{
	outb(value, data->baseaddr+offset);
}

static inline unsigned int parallel_in(struct HIDDParallelUnitData * data,
                                       int offset)
{
	return inb(data->baseaddr+offset);
}

/*************************** Classes *****************************/

/* IO bases for every Parallel port */

#if 0
ULONG bases[] = { 0x3bc, 0x378, 0x278};
#else
ULONG bases[] = { 0x378, 0x278, 0x3bc};
#endif

static OOP_AttrBase HiddParallelUnitAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_ParallelUnit, &HiddParallelUnitAB },
    { NULL,	NULL }
};

/******* ParallelUnit::New() ***********************************/
OOP_Object *PCParUnit__Root__New(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
	struct HIDDParallelUnitData * data;
//	static const struct TagItem tags[] = {{ TAG_END, 0}};
	struct TagItem *tag, *tstate;
	ULONG unitnum = 0;
	
	EnterFunc(bug("ParallelUnit::New()\n"));

	tstate = msg->attrList;
	while ((tag = NextTagItem((const struct TagItem **)&tstate))) {
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

	if (obj) {
		data = OOP_INST_DATA(cl, obj);
    
		data->baseaddr = bases[unitnum];
		data->unitnum  = unitnum;
		
	    #if 0
		/*
		 * Enable the interrupt
		 */
		parallel_outp(data, UART_PCP, UART_PCP_IRQ_EN);
	    #endif
	    
	    	/* Reset? */
		
	    	parallel_out(data, PAR_PCP, PAR_PCP_SLCT_IN);
		parallel_usleep(60);
	    	parallel_out(data, PAR_PCP, PAR_PCP_SLCT_IN | PAR_PCP_INIT);
		
		/* Reset ? */
		
	} /* if (obj) */

	D(bug("%s - an error occurred!\n",__FUNCTION__));

	ReturnPtr("ParallelUnit::New()", OOP_Object *, obj);
}

/******* ParallelUnit::Dispose() ***********************************/
OOP_Object *PCParUnit__Root__Dispose(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
	struct HIDDParallelUnitData * data;
	EnterFunc(bug("ParallelUnit::Dispose()\n"));

	data = OOP_INST_DATA(cl, obj);

    #if 0
	/* stop all interrupts */
	serial_outp(data, UART_PCP, 0);
    #endif
    
	OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
	ReturnPtr("ParallelUnit::Dispose()", OOP_Object *, obj);
}



/******* ParallelUnit::Init() **********************************/
BOOL PCParUnit__Hidd_ParallelUnit__Init(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Init *msg)
{
    struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("ParallelUnit::Init()\n"));
    data->DataReceivedCallBack = msg->DataReceived;
    data->DataReceivedUserData = msg->DataReceivedUserData;
    data->DataWriteCallBack    = msg->WriteData;
    data->DataWriteUserData    = msg->WriteDataUserData;

    ReturnBool("ParallelUnit::Init()", TRUE);
}

/******* ParallelUnit::Write() **********************************/
ULONG PCParUnit__Hidd_ParallelUnit__Write(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Write *msg)
{
	struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);
	ULONG len = msg->Length, count;
//	ULONG error;
//	unsigned char status;
	
	EnterFunc(bug("ParallelUnit::Write()\n"));
	/*
	 * If the output is currently stopped just don't do anything here.
	 */
	if (TRUE == data->stopped)
		return 0;

    	parallel_usleep(1);
	
    	for(count = 0; count < len; count++)
	{
    	    parallel_out(data, PAR_DATA, msg->Outbuffer[count]);
	    
    	    while((parallel_in(data, PAR_SP) & (PAR_SP_BUSY | PAR_SP_ERROR)) !=
		  (PAR_SP_BUSY | PAR_SP_ERROR))
	    {
		parallel_usleep(1);
	    }
	
	    parallel_usleep(1);
	    parallel_out(data, PAR_PCP, PAR_PCP_SLCT_IN | PAR_PCP_INIT | PAR_PCP_STROBE);
	    parallel_usleep(1);
	    parallel_out(data, PAR_PCP, PAR_PCP_SLCT_IN | PAR_PCP_INIT);
	    parallel_usleep(1);
	}
	
	ReturnInt("ParallelUnit::Write()",ULONG, count);
}

/******* ParallelUnit::Start() **********************************/
VOID PCParUnit__Hidd_ParallelUnit__Start(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Start *msg)
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
VOID PCParUnit__Hidd_ParallelUnit__Stop(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Stop *msg)
{
	struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);

	/*
	 * The next time the interrupt comes along and asks for
	 * more data we just don't do anything...
	 */
	data->stopped = TRUE;
}

/****** ParallelUnit::GetStatus ********************************/
UWORD PCParUnit__Hidd_ParallelUnit__GetStatus(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_GetStatus *msg)
{
	struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);

	BYTE status;
	UWORD rc = 0;
	
	status = parallel_in(data, PAR_SP);
	
	if (status & PAR_SP_BUSY)
		rc |= (1 << 0);
	if (status & PAR_SP_PE)
		rc |= (1 << 1);
	if (status & PAR_SP_SLCT)
		rc |= (1 << 2);

	return rc;
}


#if 0 /* !! STUFF BELOW DESABLED !! */

/************* The software interrupt handler that gets data from PORT *****/


#define READBUFFER_SIZE 513

AROS_UFH3(void, parallelunit_receive_data,
	 AROS_UFHA(APTR, iD, A1),
	 AROS_UFHA(APTR, iC, A5),
	 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
	AROS_USERFUNC_INIT

	struct HIDDSerialUnitData * data = iD;
	int len = 0;
	UBYTE buffer[READBUFFER_SIZE];

	buffer[len++] = parallel_inp(data, PAR_DATA);
	/*
	** ... and deliver them to whoever is interested. 
	*/

	if (NULL != data->DataReceivedCallBack)
		data->DataReceivedCallBack(buffer, len, data->unitnum, data->DataReceivedUserData);

	AROS_USERFUNC_EXIT
}

AROS_UFH3(void, parallelunit_write_more_data,
	 AROS_UFHA(APTR, iD, A1),
	 AROS_UFHA(APTR, iC, A5),
	 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
	AROS_USERFUNC_INIT

	struct HIDDParallelUnitData * data = iD;
	struct Message * msg;

	/*
	** Get the unixio message from my port and free it
	*/
	msg = GetMsg(data->replyport_read);
	FreeMem(msg, sizeof(struct uioMessage));

	/*
	** Ask for more data be written to the unit
	*/
	D(bug("Asking for more data to be written to unit %d\n",data->unitnum));

	if (NULL != data->DataWriteCallBack)
		data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);

	AROS_USERFUNC_EXIT
}

#endif /* !! END DISABLED STUFF !! */

/******* init_parallelunitclass ********************************/

AROS_SET_LIBFUNC(PCParUnit_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
	
    ReturnInt("PCParUnit_Init", ULONG, OOP_ObtainAttrBases(attrbases));
    
    AROS_SET_LIBFUNC_EXIT
}


AROS_SET_LIBFUNC(PCParUnit_Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
	
    OOP_ReleaseAttrBases(attrbases);
    ReturnInt("PCParUnit_Expunge", ULONG, TRUE);
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(PCParUnit_Init, 0)
ADD2EXPUNGELIB(PCParUnit_Expunge, 0)

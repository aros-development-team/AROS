/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial Unit hidd class implementation.
    Lang: english
*/


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

#include <utility/tagitem.h>
#include <hidd/serial.h>
#include <hidd/unixio.h>
#include <hidd/irq.h>

#include <devices/serial.h>

#include "serial_intern.h"
#include <asm/registers.h>

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

static void serialunit_receive_data(APTR iD, UWORD data, struct ExecBase *);
static ULONG serialunit_write_more_data(APTR iD, APTR iC, struct ExecBase *);

UWORD get_ustcnt(struct HIDDSerialUnitData * data);
BOOL set_baudrate(struct HIDDSerialUnitData * data, ULONG speed);

/*
 * Min. and max. supported speed by the UART
 */
#define MINSPEED 300
#define MAXSPEED 230400

#undef SysBase

static inline void serial_out_w(struct HIDDSerialUnitData * data, 
                                ULONG offset, 
                                UWORD value)
{
	AROS_GET_SYSBASE
	D(bug("poke.w 0x%x,0x%x\n",data->baseaddr+offset,value));
	WREG_W((data->baseaddr+offset)) = value;
}

static inline UWORD serial_in_w(struct HIDDSerialUnitData * data,
                                ULONG offset)
{
	AROS_GET_SYSBASE
	UWORD value = RREG_W((data->baseaddr+offset));
	D(bug("peek.w 0x%x = 0x%x\n",data->baseaddr+offset,value));
	return value;
}

#if 0
static inline void serial_out_b(struct HIDDSerialUnitData * data, 
                                ULONG offset, 
                                UBYTE value)
{
	AROS_GET_SYSBASE
	D(bug("poke.b 0x%x,0x%x\n",data->baseaddr+offset,value));
	WREG_B((data->baseaddr+offset)) = value;
}
#endif

static inline UBYTE serial_in_b(struct HIDDSerialUnitData * data,
                                ULONG offset)
{
	AROS_GET_SYSBASE
	UBYTE value = RREG_B(data->baseaddr+offset);
	D(bug("peek.b 0x%x = 0x%x\n",data->baseaddr+offset,value));
	return value;
}

#define SysBase (CSD(cl->UserData)->sysbase)

/*************************** Classes *****************************/

/* IO bases for every COM port */
ULONG bases[] = {USTCNT1, USTCNT2};


/******* SerialUnit::New() ***********************************/
static OOP_Object *serialunit_new(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
	struct HIDDSerialUnitData * data;
	struct TagItem *tag, *tstate;
	ULONG unitnum = 0;
	
	EnterFunc(bug("SerialUnit::New()\n"));

	tstate = msg->attrList;
	while ((tag = NextTagItem((struct TagItem **)&tstate))) {
		ULONG idx;

#define csd CSD(cl->UserData)
		if (IS_HIDDSERIALUNIT_ATTR(tag->ti_Tag, idx)) {
#undef csd
			switch (idx)
			{
				case aoHidd_SerialUnit_Unit:
					unitnum = (ULONG)tag->ti_Data;
				break;
			}
		}

	} /* while (tags to process) */
	
	obj = (OOP_Object *)OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);

	if (obj) {
		WORD dummy;
		data = OOP_INST_DATA(cl, obj);
    
		data->baseaddr   = bases[unitnum];
    
		data->datalength = 8;
		data->parity     = FALSE;
		data->baudrate   = SER_DEFAULT_BAUDRATE;
		data->unitnum    = unitnum;

		CSD(cl->UserData)->units[data->unitnum] = data;

		//D(bug("Unit %d at 0x0%x\n", data->unitnum, data->baseaddr));

		/* Init UART - See 14-10 of dragonball documentation */
		serial_out_w(data, O_USTCNT, UEN_F | RXEN_F | TXEN_F | RXRE_F);
		dummy = RREG_W(URX1);
		//D(bug("Setting baudrate now!"));
		/* Now set the baudrate */
		set_baudrate(data, data->baudrate);

		/* Set the interrupts and the levels according to the baudrate */
		serial_out_w(data, O_USTCNT, (get_ustcnt(data) | UEN_F | RXEN_F | TXEN_F | RXRE_F));

	} /* if (obj) */

	ReturnPtr("SerialUnit::New()", OOP_Object *, obj);
}

/******* SerialUnit::Dispose() ***********************************/
static OOP_Object *serialunit_dispose(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
	struct HIDDSerialUnitData * data;
	EnterFunc(bug("SerialUnit::Dispose()\n"));

	data = OOP_INST_DATA(cl, obj);

	CSD(cl->UserData)->units[data->unitnum] = NULL;

	/* stop all interrupts, disabling the UART (might save power) */
	serial_out_w(data, O_USTCNT, 0);

	OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
	ReturnPtr("SerialUnit::Dispose()", OOP_Object *, obj);
}



/******* SerialUnit::Init() **********************************/
BOOL serialunit_init(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Init *msg)
{
	struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  
	EnterFunc(bug("SerialUnit::Init()\n"));
	data->DataReceivedCallBack = msg->DataReceived;
	data->DataReceivedUserData = msg->DataReceivedUserData;
	data->DataWriteCallBack	   = msg->WriteData;
	data->DataWriteUserData    = msg->WriteDataUserData;

	ReturnBool("SerialUnit::Init()", TRUE);
}

/******* SerialUnit::Write() **********************************/
ULONG serialunit_write(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Write *msg)
{
	struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
	ULONG len = msg->Length;
	ULONG count = 0;
	UWORD utx;
  
	EnterFunc(bug("SerialUnit::Write()\n"));

	/*
	 * If the output is currently stopped just don't do anything here.
	 */
	if (TRUE == data->stopped)
		return 0;

	utx = serial_in_w(data, O_UTX);

	/*
	 * I may only write something here if nothing is in the fifo right
	 * now because otherwise this might be handled through an interrupt.
	 */
	if (utx & FIFO_EMPTY_F) {
		/* write data into FIFO */
		do {
			//D(bug("%c",msg->Outbuffer[count]));
			serial_out_w(data, O_UTX, msg->Outbuffer[count++]);
			len--;
			utx = serial_in_w(data, O_UTX);
		} while (len > 0 && (utx & TX_AVAIL_F));
	}

	ReturnInt("SerialUnit::Write()",ULONG, count);
}

/***************************************************************************/

static ULONG valid_baudrates[] =
{
	MINSPEED | LIMIT_LOWER_BOUND,
	MAXSPEED | LIMIT_UPPER_BOUND,
	~0
};


/******* SerialUnit::SetBaudrate() **********************************/
BOOL serialunit_setbaudrate(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_SetBaudrate *msg)
{
	struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
	BOOL valid = FALSE;
  
	if (msg->baudrate != data->baudrate) {
		valid = set_baudrate(data, msg->baudrate);
	} /* if */
	return valid;
}

static UBYTE valid_datalengths[] =
{
	7,
	8,
	~0
};

/******* SerialUnit::SetParameters() **********************************/
BOOL serialunit_setparameters(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_SetParameters *msg)
{
	struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
	BOOL valid = TRUE;
	int i = 0;
	struct TagItem * tags = msg->tags;
	
	while (TAG_END != tags[i].ti_Tag && TRUE == valid) {
		switch (tags[i].ti_Tag) {
			case TAG_DATALENGTH:
				if (tags[i].ti_Data >= 7 && tags[i].ti_Data <= 8)
					data->datalength = tags[i].ti_Data;
				else
					valid = FALSE;
			break;
			
			case TAG_STOP_BITS:
				if (1 == tags[i].ti_Data ||
				    2 == tags[i].ti_Data)
					data->stopbits = tags[i].ti_Data;
				else
					valid = FALSE;            
			break;

			case TAG_PARITY:
				if (PARITY_EVEN == tags[i].ti_Data ||
				    PARITY_ODD  == tags[i].ti_Data) {
					data->parity     = TRUE;
					data->paritytype = tags[i].ti_Data;
				}
				else
					valid = FALSE;
			break;

			case TAG_PARITY_OFF:
				data->parity = FALSE;
			break;

			case TAG_SET_MCR:
#warning MCR??
			break;
			
			case TAG_SKIP:
			case TAG_IGNORE:
			break;
		  
			default:
				valid = FALSE;
		}
		i++;
	}

	serial_out_w(data, O_USTCNT, get_ustcnt(data));

	return valid;
}

/******* SerialUnit::SendBreak() **********************************/
BYTE serialunit_sendbreak(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_SendBreak *msg)
{
	struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);

	UWORD code = serial_in_w(data, O_UTX);
	serial_out_w(data, O_UTX, code | SEND_BREAK_F);

	return SerErr_LineErr;
}

/******* SerialUnit::Start() **********************************/
VOID serialunit_start(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Start *msg)
{
	struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  
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

/******* SerialUnit::Stop() **********************************/
VOID serialunit_stop(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Stop *msg)
{
	struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  
	/*
	 * The next time the interrupt comes along and asks for
	 * more data we just don't do anything...
	 */
	data->stopped = TRUE;
}

/****** SerialUnit::GetCapabilities ********************************/
VOID serialunit_getcapabilities(OOP_Class * cl, OOP_Object *o, struct TagItem * tags)
{
	if (NULL != tags) {
		int i = 0;
		BOOL end = FALSE;
		while (FALSE == end) {
			switch (tags[i].ti_Tag) {
				case HIDDA_SerialUnit_BPSRate:
					tags[i].ti_Data = (STACKIPTR)valid_baudrates;
				break;
        
				case HIDDA_SerialUnit_DataLength:
					tags[i].ti_Data = (STACKIPTR)valid_datalengths;
				break;
        
				case TAG_DONE:
					end = TRUE;
				break;
			}
			i++;
		}
	}
}

/****** SerialUnit::GetStatus ********************************/
UWORD serialunit_getstatus(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_GetStatus *msg)
{
	struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
	UWORD status = 0;

	data = NULL;
	/*
	 *
	 */

	return status;
}


/************* The function that gets data from UART *****/


#undef OOPBase
#undef SysBase
#undef UtilityBase

#define READBUFFER_SIZE 65

static void serialunit_receive_data(APTR iD, UWORD urx, struct ExecBase * SysBase)
{
	struct HIDDSerialUnitData * data = iD;
	int len = 0;
	UBYTE buffer[READBUFFER_SIZE];

	/*
	** Read the data from the port ...
	** !!! The xcopilot implementation seem rather stupid. I can only get one
	** byte per interrupt. I hope the real thing is a bit better... !!!
	*/
	while (len < sizeof(buffer)) {
		buffer[len++] = (UBYTE)urx;
//D(bug("Got byte form serial port: %d (%c)\n",(UBYTE)urx,(UBYTE)urx));
		
		/*
		 * Check for the next incoming byte - whether there is one.
		 * I have to do it this way, because xcopilot returns no
		 * DATA_READY flag once the register is read with 16 bit access.
		 */
		urx = serial_in_w(data, O_URX);
		if (0 == (DATA_READY_F & urx)) {
			break;
		}
	}

	/*
	** ... and deliver them to whoever is interested.
	*/

	if (NULL != data->DataReceivedCallBack)
		data->DataReceivedCallBack(buffer, len, data->unitnum, data->DataReceivedUserData);
}

static ULONG serialunit_write_more_data(APTR iD, APTR iC, struct ExecBase * SysBase)
{
	struct HIDDSerialUnitData * data = iD;

	/*
	 * If the output is currently stopped just don't do
	 * anything here.
	 */
	if (TRUE == data->stopped)
		return -1;

	/*
	** Ask for more data be written to the unit
	*/
//	D(bug("Asking for more data to be written to unit %d\n",data->unitnum));

	if (NULL != data->DataWriteCallBack)
		data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);
	return 0;
}


/******* init_serialunitclass ********************************/

#define SysBase     (csd->sysbase)
#define OOPBase     (csd->oopbase)
#define UtilityBase (csd->utilitybase)


#define NUM_ROOT_METHODS 2
#define NUM_SERIALUNIT_METHODS moHidd_SerialUnit_NumMethods

OOP_Class *init_serialunitclass (struct class_static_data *csd)
{
	OOP_Class *cl = NULL;
    
	struct OOP_MethodDescr serialunithiddroot_descr[NUM_ROOT_METHODS + 1] = 
	{
		{(IPTR (*)())serialunit_new,		moRoot_New},
		{(IPTR (*)())serialunit_dispose,	moRoot_Dispose},
/*
		{(IPTR (*)())serialunit_set,		moRoot_Set},
		{(IPTR (*)())serialunit_get,		moRoot_Get},
*/
		{NULL, 0UL}
	};
    
	struct OOP_MethodDescr serialunithidd_descr[NUM_SERIALUNIT_METHODS + 1] =
	{
		{(IPTR (*)())serialunit_init,		moHidd_SerialUnit_Init},
		{(IPTR (*)())serialunit_write,		moHidd_SerialUnit_Write},
		{(IPTR (*)())serialunit_setbaudrate,	moHidd_SerialUnit_SetBaudrate},
		{(IPTR (*)())serialunit_setparameters,	moHidd_SerialUnit_SetParameters},
		{(IPTR (*)())serialunit_sendbreak,	moHidd_SerialUnit_SendBreak},
		{(IPTR (*)())serialunit_start,		moHidd_SerialUnit_Start},
		{(IPTR (*)())serialunit_stop,		moHidd_SerialUnit_Stop},
		{(IPTR (*)())serialunit_getcapabilities,moHidd_SerialUnit_GetCapabilities},
		{(IPTR (*)())serialunit_getstatus      ,moHidd_SerialUnit_GetStatus},
		{NULL, 0UL}
	};
    
	struct OOP_InterfaceDescr ifdescr[] =
	{
		{serialunithiddroot_descr	, IID_Root		, NUM_ROOT_METHODS},
		{serialunithidd_descr		, IID_Hidd_SerialUnit	, NUM_SERIALUNIT_METHODS},
		{NULL, NULL, 0}
	};

	OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
    	
	struct TagItem tags[] =
	{
		{ aMeta_SuperID,                (IPTR)CLID_Root},
		{ aMeta_InterfaceDescr,         (IPTR)ifdescr},
		{ aMeta_ID,                     (IPTR)CLID_Hidd_SerialUnit},
		{ aMeta_InstSize,               (IPTR)sizeof (struct HIDDSerialUnitData) },
		{TAG_DONE, 0UL}
	};


	EnterFunc(bug("    init_serialunitclass(csd=%p)\n", csd));

	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	D(bug("Class=%p\n", cl));
	if (cl) {
		__IHidd_SerialUnitAB = OOP_ObtainAttrBase(IID_Hidd_SerialUnit);
		if (NULL != __IHidd_SerialUnitAB) {
			D(bug("SerialUnit Class ok\n"));
			cl->UserData = (APTR)csd;

			OOP_AddClass(cl);
		} else {
			free_serialunitclass(csd);
			cl = NULL;
		}
	}

	ReturnPtr("init_serialunitclass", OOP_Class *, cl);
}


void free_serialunitclass(struct class_static_data *csd)
{
	EnterFunc(bug("free_serialhiddclass(csd=%p)\n", csd));

	if(csd) {
		OOP_RemoveClass(csd->serialhiddclass);
	
		if(csd->serialhiddclass) OOP_DisposeObject((OOP_Object *) csd->serialhiddclass);
		csd->serialhiddclass = NULL;
	}

	ReturnVoid("free_serialhiddclass");
}


UWORD get_ustcnt(struct HIDDSerialUnitData * data)
{
	UWORD ustcnt = 0;
	switch (data->datalength) {
		case 8: ustcnt |= EITHER8OR7_F;
		break;
	}
  
	switch (data->stopbits) {
		case 1: /* 1 stopbit */
			/* nothing to do */
		break;
    
		case 2: /* 2 stopbits */
			ustcnt |= STOP_F;
		break;
	  
		default:
		break;
	}
  
	if (TRUE == data->parity) {
		ustcnt |= PARITY_EN_F;

		switch (data->paritytype) {
			case PARITY_ODD:
				ustcnt |= ODD_F;
			break;
		}
	}

	if (data->baudrate < 1200) {
		ustcnt |= /*RXFE_F|RXHE_F|*/ RXRE_F | /* TXEE_F|*/ TXHE_F;
	} else if (data->baudrate < 9600) {
		ustcnt |= /* RXHE_F|*/ RXRE_F | /* TXEE_F|*/ TXHE_F;
	} else {
		ustcnt |= RXRE_F|TXHE_F;
	}

	return ustcnt;
}

struct baud_rate_settings
{
	ULONG	baudrate;
	UBYTE	divider;
	UBYTE	prescaler;
};

/*
 * For a default 33.16Mhz system clock
 */
static const struct baud_rate_settings brs[] =
{
	{600,	7,	0x26},
	{1200,	6,	0x26},
	{2400,	5,	0x26},
	{4800,	4,	0x26},
	{9600,	3,	0x26},
	{14400,	4,	0x38},
	{19200,	2,	0x26},
	{28800,	3,	0x38},
	{38400,	1,	0x26},
	{57600,	2,	0x38},
	{115200,1,	0x38},
	{230400,0,	0x38},
	{~0,	~0,	~0}
};

BOOL set_baudrate(struct HIDDSerialUnitData * data, ULONG speed)
{
	/* set the speed on the UART now */
#warning Which bits to set in ubaud???
	UWORD ubaud = 0;
	int i = 0;
	int found = FALSE;
	
	if (speed < MINSPEED || speed > MAXSPEED)
		return FALSE;
	
	while (~0 != brs[i].baudrate) {
		if (speed == brs[i].baudrate) {
			ubaud |= (((UWORD)brs[i].divider) << 8) | brs[i].prescaler;
			found = TRUE;
			break;
		}
		i++;
	}
	
	if (FALSE == found) {
		UBYTE divider = 7;
		ULONG prescaler;
		ULONG best = 0;
		ULONG val;
		UBYTE _divider = 7;
		ULONG _prescaler = 0;
		/*
		 * Try to calculate the parameters for this odd baudrate.
		 * Don't know whether this is correct...
		 */
		while (divider > 0) {
			prescaler = (2073600/(1<<divider))/speed;
			/*
			 * Calculate backwards and see how good this 
			 * result is (due to integer rounding of the
			 * previous divisions).
			 */
			if (0 == prescaler)
				break;
				
			if (prescaler < 65) {
				val = prescaler*(1<<divider);
				if (val > best) {
					best = val;
					_divider = divider;
					_prescaler = prescaler;
				}
			}
			
			divider--;
		}
		
		ubaud |= (((UWORD)_divider) << 8) | (65-_prescaler);
	}

	serial_out_w(data, O_UBAUD, ubaud);
	return TRUE;
}

/* Serial interrupts */

#undef SysBase
#define SysBase (hw->sysBase)
#define csd ((struct class_static_data *)(irq->h_Data))


static void common_serial_int_handler(HIDDT_IRQ_Handler * irq, 
                                      HIDDT_IRQ_HwInfo * hw,
                                      ULONG unitnum)
{
	UWORD code = 0;
	if (csd->units[unitnum]) {
		code = serial_in_w(csd->units[unitnum], O_URX);
	}

//D(bug("---------- IN COMMON SERIAL HANDLER!\n"));
//D(bug("URX: code=0x%x\n",code));
	if (code & (FIFO_EMPTY_F|FIFO_HALF_F|DATA_READY_F)) {
		D(bug("In %s 1\n",__FUNCTION__));
		if (csd->units[unitnum]) {
			serialunit_receive_data(csd->units[unitnum],
			                        code,
			                        SysBase);
		}
	}

	code = 0;
	if (csd->units[unitnum])
		code = serial_in_w(csd->units[unitnum], O_UTX);

//D(bug("UTX: code=0x%x\n",code));
	if (code & (FIFO_EMPTY_F|FIFO_HALF_F|TX_AVAIL_F)) {
		D(bug("In %s 2\n",__FUNCTION__));
		if (csd->units[unitnum]) {
			if (0 == serialunit_write_more_data(csd->units[unitnum], 
			                                    NULL, 
			                                    SysBase)) {
				
			}
		}
	}
}

void serial_int_uart1(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
	common_serial_int_handler(irq, hw, 0);
}

void serial_int_uart2(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
	common_serial_int_handler(irq, hw, 1);
}

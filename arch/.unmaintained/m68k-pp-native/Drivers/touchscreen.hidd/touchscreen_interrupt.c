#include "touchscreen.h"
#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <asm/registers.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#define DEBUG 0
#include <aros/debug.h>

#undef SysBase
/*
 * Interrupt handler for the touchscreen
 */
void touchscreen_int(HIDDT_IRQ_Handler * irq, HIDDT_IRQ_HwInfo *hw)
{
	struct ExecBase * SysBase =     *(struct ExecBase **)0x04;
	struct mouse_data               *data =(struct mouse_data *)irq->h_Data;
	struct pHidd_Mouse_Event        *e = &data->me;
	/*
	 * Try to read the coordinates
	 */
	WREG_B(PFDATA)   = (RREG_B(PFDATA) & 0xf0) | 0x06;
	WREG_W(SPICONT2) = (SPI_ENABLE_F|SPI_XCH_F|SPI_IRQEN_F);
	
	e->x = RREG_W(SPIDATA2);
	
	WREG_B(PFDATA)   = (RREG_B(PFDATA) & 0xf0) | 0x09;
	WREG_W(SPICONT2) = (SPI_ENABLE_F|SPI_XCH_F|SPI_IRQEN_F);

	e->y = RREG_W(SPIDATA2);

	e->x = -(e->x/2-0xff);
	e->y = -(e->y/2-0xff);

	if (STATE_IDLE == data->state) {
		e->button = vHidd_Mouse_NoButton;
		e->type   = vHidd_Mouse_Motion;
		D(bug("---------------- Motion! callback=%p\n",data->mouse_callback));
		data->mouse_callback(data->callbackdata, e);

		e->button = vHidd_Mouse_Button1;
		e->type   = vHidd_Mouse_Press;
		D(bug("------------- Press!\n"));
		data->mouse_callback(data->callbackdata, e);
	} else {
		if ((e->x != data->lastx) &&
		    (e->y != data->lasty)) {
			e->button = vHidd_Mouse_Button1;
			e->type   = vHidd_Mouse_Motion;
			D(bug("---------------- Motion! callback=%p\n",data->mouse_callback));
			data->mouse_callback(data->callbackdata, e);
		}
	}

	data->lastx = e->x;
	data->lasty = e->y;
	data->state = STATE_PEN_DOWN;


	data->idlectr = 0;
}


AROS_UFH4(ULONG, tsVBlank,
     AROS_UFHA(ULONG, dummy, A0),
     AROS_UFHA(void *, _data, A1),
     AROS_UFHA(ULONG, dummy2, A5),
     AROS_UFHA(struct ExecBase *, SysBase, A6))
{
	AROS_USERFUNC_INIT
	struct mouse_data *data =(struct mouse_data *)_data;
	if (STATE_PEN_DOWN == data->state) {
		data->idlectr++;
		if (2 == data->idlectr) {
			struct pHidd_Mouse_Event  *e = &data->me;
			e->x = 0;
			e->y = 0;
			e->button = vHidd_Mouse_Button1;
			e->type   = vHidd_Mouse_Release;
			data->mouse_callback(data->callbackdata, e);
			data->state = STATE_IDLE;
		}
	}
	return 0;
	AROS_USERFUNC_EXIT
}

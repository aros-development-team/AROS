#include <proto/oop.h>
#include <hidd/irq.h>
#include "uhciclass.h"

STATIC APTR uhci_new(struct USBUHCI_staticdata *xsd) {
struct UHCIData *data;

	data = AllocMem(sizeof(struct UHCIData), MEMF_PUBLIC | MEMF_CLEAR);
	if (data != NULL)
	{
		data->irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
		if (data->irqhidd != NULL)
		{
			data->irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC | MEMF_CLEAR);
			if (data->irq)
			{
				data->irq->h_Node.ln_Name = "USB UHCI irq";
				data->irq->h_Code = UHCIInterrupt;
				data->irq->h_Data = data;
				HIDD_IRQ_AddHandler(data->irqhidd, data->irq, 
			}
		}
	}
	return NULL;
}

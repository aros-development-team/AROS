#ifndef ROM_USB_CLASSES_CDCETH_H
#define ROM_USB_CLASSES_CDCETH_H

#include <exec/types.h>
#include <stdint.h>

#define CDC_CLASS_COMMUNICATION       0x02
#define CDC_CLASS_DATA                0x0a

#define CDC_SUBCLASS_ETHERNET         0x06
#define CDC_SUBCLASS_EEM              0x0c
#define CDC_SUBCLASS_NCM              0x0d

#define CDC_DESC_CS_INTERFACE         0x24

#define CDC_SUBTYPE_ETHERNET          0x0f
#define CDC_SUBTYPE_UNION             0x06
#define CDC_SUBTYPE_HEADER            0x00

#define ETHER_ADDR_SIZE 6

BOOL cdceth_is_cdc_ethernet(uint8_t cls, uint8_t subcls, uint8_t proto);


#endif

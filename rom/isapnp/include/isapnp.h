#ifndef	RESOURCES_ISAPNP_H
#define RESOURCES_ISAPNP_H

/*
**	$VER: isapnp.h 1.1 (10.5.2001)
**
**	isapnp.resource definitions.
**
**	(C) Copyright 2001 Martin Blom
**	All Rights Reserved.
**
*/

#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/semaphores.h>

/*** The name of the exec resource *******************************************/

#define ISAPNPNAME "isapnp.resource"


/*** A macro usable for ISAPNP_FindCard() and ISAPNP_FindDevice() ************/

#define ISAPNP_MAKE_ID(a,b,c) \
 ( (LONG) (a) << 24 | (LONG) (b) << 16 | (LONG) (c) << 8 )


/*** Flags for ISAPNP_LockCardsA() and ISAPNP_LockDevicesA() *****************/

#define ISAPNP_LOCKF_NONE           0x00

#define ISAPNP_LOCKF_NONBLOCKING    0x01
#define ISAPNP_LOCKB_NONBLOCKING    0

/*** Node types **************************************************************/

#define ISAPNP_NT_CARD            ( NT_USER - 1 )
#define ISAPNP_NT_DEVICE          ( NT_USER - 2 )
#define ISAPNP_NT_RESOURCE_GROUP  ( NT_USER - 3 )
#define ISAPNP_NT_IRQ_RESOURCE    ( NT_USER - 4 )
#define ISAPNP_NT_DMA_RESOURCE    ( NT_USER - 5 )
#define ISAPNP_NT_IO_RESOURCE     ( NT_USER - 6 )
#define ISAPNP_NT_MEMORY_RESOURCE ( NT_USER - 7 )

/*** A unique identifier for a card or logical device ************************/

struct ISAPNP_Identifier
{
  struct MinNode isapnpid_MinNode;
  char           isapnpid_Vendor[ 4 ];
  UWORD          isapnpid_ProductID;
  UBYTE	         isapnpid_Revision;
  UBYTE          isapnpid_Pad;
};


/*** A PNP ISA card **********************************************************/

struct ISAPNP_Card
{
  struct Node              isapnpc_Node;

  BOOL                     isapnpc_Disabled;

  struct List              isapnpc_Devices;
  UWORD                    isapnpc_Pad1;

  struct SignalSemaphore   isapnpc_Lock;
  UWORD                    isapnpc_Pad2;

  struct ISAPNP_Identifier isapnpc_ID;
  ULONG                    isapnpc_SerialNumber;

  UBYTE                    isapnpc_CSN;

  UBYTE                    isapnpc_MajorPnPVersion;
  UBYTE                    isapnpc_MinorPnPVersion;
  UBYTE                    isapnpc_VendorPnPVersion;
};


/*** A logical device on an ISA card *****************************************/

struct ISAPNP_ResourceGroup;

struct ISAPNP_Device
{
  struct Node                  isapnpd_Node;

  BOOL       	               isapnpd_Disabled;

  struct ISAPNP_Card*          isapnpd_Card;

  struct SignalSemaphore       isapnpd_Lock;
  UWORD                        isapnpd_Pad1;
  
  struct MinList               isapnpd_IDs;
  struct ISAPNP_ResourceGroup* isapnpd_Options;
  struct MinList               isapnpd_Resources;

  UWORD                        isapnpd_SupportedCommands;
  UWORD                        isapnpd_DeviceNumber;
};

/* Flags for isapnpd_SupportedCommands */

#define ISAPNP_DEVICE_SCF_BOOTABLE    0x01
#define ISAPNP_DEVICE_SCF_RANGE_CHECK 0x02
#define ISAPNP_DEVICE_SCB_BOOTABLE    0
#define ISAPNP_DEVICE_SCB_RANGE_CHECK 1


/*** A resource group ********************************************************/

struct ISAPNP_ResourceGroup
{
  struct MinNode isapnprg_MinNode;
  UBYTE          isapnprg_Type;
  UBYTE          isapnprg_Pri;

  UWORD          isapnprg_Pad;

  struct MinList isapnprg_Resources;
  struct MinList isapnprg_ResourceGroups;
};

/* Priorities for isapnprg_Pri and PNPISA_AllocResourceGroup() */

#define ISAPNP_RG_PRI_GOOD        64
#define ISAPNP_RG_PRI_ACCEPTABLE  0
#define ISAPNP_RG_PRI_SUBOPTIMAL  -64


/*** An resource (the "base class") ******************************************/

struct ISAPNP_Resource
{
  struct MinNode isapnpr_MinNode;
  UBYTE          isapnpr_Type;
};


/*** An IRQ resource *********************************************************/

struct ISAPNP_IRQResource
{
  struct MinNode isapnpirqr_MinNode;
  UBYTE          isapnpirqr_Type;
  
  UBYTE          isapnpirqr_IRQType;
  UWORD          isapnpirqr_IRQMask;
};

/* Flags for isapnpirqr_IRQType */

#define ISAPNP_IRQRESOURCE_ITF_HIGH_EDGE  0x01
#define ISAPNP_IRQRESOURCE_ITF_LOW_EDGE   0x02
#define ISAPNP_IRQRESOURCE_ITF_HIGH_LEVEL 0x04
#define ISAPNP_IRQRESOURCE_ITF_LOW_LEVEL  0x08
#define ISAPNP_IRQRESOURCE_ITB_HIGH_EDGE  0
#define ISAPNP_IRQRESOURCE_ITB_LOW_EDGE   1
#define ISAPNP_IRQRESOURCE_ITB_HIGH_LEVEL 2
#define ISAPNP_IRQRESOURCE_ITB_LOW_LEVEL  3


/*** A DMA resource **********************************************************/

struct ISAPNP_DMAResource
{
  struct MinNode isapnpdmar_MinNode;
  UBYTE          isapnpdmar_Type;

  UBYTE          isapnpdmar_ChannelMask;
  UBYTE          isapnpdmar_Flags;
};

/* Flags for isapnpdmar_Flags */

#define ISAPNP_DMARESOURCE_F_TRANSFER_MASK  0x03
#define ISAPNP_DMARESOURCE_F_TRANSFER_8BIT  0x00
#define ISAPNP_DMARESOURCE_F_TRANSFER_BOTH  0x01
#define ISAPNP_DMARESOURCE_F_TRANSFER_16BIT 0x02

#define ISAPNP_DMARESOURCE_FF_BUS_MASTER    0x04
#define ISAPNP_DMARESOURCE_FF_BYTE_MODE     0x08
#define ISAPNP_DMARESOURCE_FF_WORD_MODE     0x10
#define ISAPNP_DMARESOURCE_FB_BUS_MASTER    2
#define ISAPNP_DMARESOURCE_FB_BYTE_MODE     3
#define ISAPNP_DMARESOURCE_FB_WORD_MODE     4

#define ISAPNP_DMARESOURCE_F_SPEED_MASK       0x60
#define ISAPNP_DMARESOURCE_F_SPEED_COMPATIBLE 0x00
#define ISAPNP_DMARESOURCE_F_SPEED_TYPE_A     0x20
#define ISAPNP_DMARESOURCE_F_SPEED_TYPE_B     0x40
#define ISAPNP_DMARESOURCE_F_SPEED_TYPE_F     0x60


/*** An IO resource **********************************************************/

struct ISAPNP_IOResource
{
  struct MinNode isapnpior_MinNode;
  UBYTE          isapnpior_Type;

  UBYTE          isapnpior_Flags;

  UBYTE          isapnpior_Alignment;
  UBYTE          isapnpior_Length;

  UWORD          isapnpior_MinBase;
  UWORD          isapnpior_MaxBase;
};

/* Flags for isapnpior_Flags */

#define ISAPNP_IORESOURCE_FF_FULL_DECODE 0x01
#define ISAPNP_IORESOURCE_FB_FULL_DECODE 0

#endif /* RESOURCES_ISAPNP_H */

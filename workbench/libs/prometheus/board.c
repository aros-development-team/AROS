/*
    Copyright (C) 2005 Neil Cafferkey
    $Id$
*/


#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/prometheus.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "prometheus_intern.h"


/* Private prototypes */

static VOID WrapperIRQ(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw_info);


static const struct TagItem map_tag_list[] =
{
   {PRM_Vendor,         aoHidd_PCIDevice_VendorID},
   {PRM_Device,         aoHidd_PCIDevice_ProductID},
   {PRM_Revision,       aoHidd_PCIDevice_RevisionID},
   {PRM_Class,          aoHidd_PCIDevice_Class},
   {PRM_SubClass,       aoHidd_PCIDevice_SubClass},
   {PRM_MemoryAddr0,    aoHidd_PCIDevice_Base0},
   {PRM_MemoryAddr1,    aoHidd_PCIDevice_Base1},
   {PRM_MemoryAddr2,    aoHidd_PCIDevice_Base2},
   {PRM_MemoryAddr3,    aoHidd_PCIDevice_Base3},
   {PRM_MemoryAddr4,    aoHidd_PCIDevice_Base4},
   {PRM_MemoryAddr5,    aoHidd_PCIDevice_Base5},
   {PRM_ROM_Address,    aoHidd_PCIDevice_RomBase},
   {PRM_MemorySize0,    aoHidd_PCIDevice_Size0},
   {PRM_MemorySize1,    aoHidd_PCIDevice_Size1},
   {PRM_MemorySize2,    aoHidd_PCIDevice_Size2},
   {PRM_MemorySize3,    aoHidd_PCIDevice_Size3},
   {PRM_MemorySize4,    aoHidd_PCIDevice_Size4},
   {PRM_MemorySize5,    aoHidd_PCIDevice_Size5},
   {PRM_ROM_Size,       aoHidd_PCIDevice_RomSize},
   {PRM_SlotNumber,     aoHidd_PCIDevice_Dev},
   {PRM_FunctionNumber, aoHidd_PCIDevice_Sub},
   {TAG_END, 0}
};


/***************************************************************************

    NAME */
	AROS_LH2(PCIBoard *, Prm_FindBoardTagList,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, previous, A0),
	AROS_LHA(struct TagItem *, tag_list, A1),

/*  LOCATION */
	struct Library *, PrometheusBase, 5, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   ULONG count = 0;
   Tag tag, aros_tag;
   struct TagItem *tag_item, *temp_tag_list;
   struct PCIBoard *board, *tail;
   BOOL success = FALSE;
   UPINT board_data;

   if(previous != NULL)
      board = (APTR)previous->node.mln_Succ;
   else
      board = (APTR)base->boards.mlh_Head;
   tail = (APTR)&base->boards.mlh_Tail;

   while(board != tail && !success)
   {
      /* Check if the current board matches all requirements */

      temp_tag_list = tag_list;
      success = TRUE;
      while((tag_item = NextTagItem(&temp_tag_list)) != NULL)
      {
         tag = tag_item->ti_Tag;
         aros_tag = GetTagData(tag, TAG_DONE, map_tag_list);
         if(aros_tag != TAG_DONE)
         {
            OOP_GetAttr(board->aros_board,
               base->pcidevice_attr_base + aros_tag, &board_data);
         }
         else if(tag == PRM_BoardOwner)
            board_data = (UPINT)board->owner;

         if(board_data != tag_item->ti_Data)
            success = FALSE;
      }

      if(!success)
         board = (APTR)board->node.mln_Succ;
   }

   if(!success)
      board = NULL;

   return board;

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH2(ULONG, Prm_GetBoardAttrsTagList,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, board, A0),
	AROS_LHA(struct TagItem *, tag_list, A1),

/*  LOCATION */
	struct Library *, PrometheusBase, 6, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   ULONG count = 0;
   Tag aros_tag;
   UPINT *tag_data_ptr;
   struct TagItem *tag_item;

   while((tag_item = NextTagItem(&tag_list)) != NULL)
   {
      if(tag_item->ti_Tag == PRM_BoardOwner)
      {
         tag_data_ptr = (UPINT *)tag_item->ti_Data;
         *tag_data_ptr = (UPINT)board->owner;
         count++;
      }
      else
      {
         aros_tag = GetTagData(tag_item->ti_Tag, TAG_DONE, map_tag_list);
         if(aros_tag != TAG_DONE)
         {
            OOP_GetAttr(board->aros_board,
               base->pcidevice_attr_base + aros_tag,
               (UPINT *)tag_item->ti_Data);
            count++;
         }
      }
   }

   return count;

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH2(ULONG, Prm_SetBoardAttrsTagList,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, board, A0),
	AROS_LHA(struct TagItem *, tag_list, A1),

/*  LOCATION */
	struct Library *, PrometheusBase, 13, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   ULONG count = 0;
   APTR new_owner;
   struct TagItem *tag_item;

   tag_item = FindTagItem(PRM_BoardOwner, tag_list);
   if(tag_item != NULL)
   {
      new_owner = (APTR)tag_item->ti_Data;
      if(new_owner != NULL && board->owner == NULL
         || new_owner == NULL && board->owner != NULL)
      {
         board->owner = new_owner;
         count++;
      }
   }

   return count;

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH2(UBYTE, Prm_ReadConfigByte,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, board, A0),
	AROS_LHA(UBYTE, offset, D0),

/*  LOCATION */
	struct Library *, PrometheusBase, 9, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   struct pHidd_PCIDevice_ReadConfigByte message;

   message.mID =
      OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
   message.reg = offset;

   return (UBYTE)OOP_DoMethod(board->aros_board, (OOP_Msg)&message);

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH3(VOID, Prm_WriteConfigByte,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, board, A0),
	AROS_LHA(UBYTE, data, D0),
	AROS_LHA(UBYTE, offset, D1),

/*  LOCATION */
	struct Library *, PrometheusBase, 12, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   struct pHidd_PCIDevice_WriteConfigByte message;

   message.mID =
      OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
   message.reg = offset;
   message.val = data;

   OOP_DoMethod(board->aros_board, (OOP_Msg)&message);

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH2(UWORD, Prm_ReadConfigWord,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, board, A0),
	AROS_LHA(UBYTE, offset, D0),

/*  LOCATION */
	struct Library *, PrometheusBase, 8, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   struct pHidd_PCIDevice_ReadConfigWord message;

   message.mID =
      OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
   message.reg = offset & ~0x1;

   return (UWORD)OOP_DoMethod(board->aros_board, (OOP_Msg)&message);

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH3(VOID, Prm_WriteConfigWord,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, board, A0),
	AROS_LHA(UWORD, data, D0),
	AROS_LHA(UBYTE, offset, D1),

/*  LOCATION */
	struct Library *, PrometheusBase, 11, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   struct pHidd_PCIDevice_WriteConfigWord message;

   message.mID =
      OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);
   message.reg = offset & ~0x1;
   message.val = data;

   OOP_DoMethod(board->aros_board, (OOP_Msg)&message);

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH2(ULONG, Prm_ReadConfigLong,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, board, A0),
	AROS_LHA(UBYTE, offset, D0),

/*  LOCATION */
	struct Library *, PrometheusBase, 7, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   struct pHidd_PCIDevice_ReadConfigLong message;

   message.mID =
      OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong);
   message.reg = offset & ~0x3;

   return (ULONG)OOP_DoMethod(board->aros_board, (OOP_Msg)&message);

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH3(VOID, Prm_WriteConfigLong,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, board, A0),
	AROS_LHA(ULONG, data, D0),
	AROS_LHA(UBYTE, offset, D1),

/*  LOCATION */
	struct Library *, PrometheusBase, 10, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   struct pHidd_PCIDevice_WriteConfigLong message;

   message.mID =
      OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigLong);
   message.reg = offset & ~0x3;
   message.val = data;

   OOP_DoMethod(board->aros_board, (OOP_Msg)&message);

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH2(BOOL, Prm_AddIntServer,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, board, A0),
	AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
	struct Library *, PrometheusBase, 14, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   BOOL success = TRUE;
   HIDDT_IRQ_Handler *aros_irq;
   UPINT int_no;

   /* Allocate AROS int structure */

   if(board == NULL || board->aros_irq != NULL)
      success = FALSE;

   if(success)
   {
      aros_irq =
         AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC | MEMF_CLEAR);
      if(aros_irq == NULL)
         success = FALSE;
   }

   /* Add AROS int to system */

   if(success)
   {
      OOP_GetAttr(board->aros_board,
         base->pcidevice_attr_base + aoHidd_PCIDevice_INTLine, &int_no);
      board->aros_irq = aros_irq;
      aros_irq->h_Node.ln_Name = interrupt->is_Node.ln_Name;
      aros_irq->h_Code = WrapperIRQ;
      aros_irq->h_Data = interrupt;

      success = HIDD_IRQ_AddHandler(base->irq_hidd, aros_irq, int_no);
   }

   return success;

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH2(VOID, Prm_RemIntServer,

/*  SYNOPSIS */
	AROS_LHA(PCIBoard *, board, A0),
	AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
	struct Library *, PrometheusBase, 15, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;

   if(board != NULL)
   {
      HIDD_IRQ_RemHandler(base->irq_hidd, board->aros_irq);
      FreeMem(board->aros_irq, sizeof(HIDDT_IRQ_Handler));
      board->aros_irq = NULL;
   }

   return;

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH1(APTR, Prm_AllocDMABuffer,

/*  SYNOPSIS */
	AROS_LHA(ULONG, size, D0),

/*  LOCATION */
	struct Library *, PrometheusBase, 16, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   APTR buffer;

   if(size != 0)
      buffer = AllocMem(size, MEMF_PUBLIC);
   else
      buffer = NULL;

   return buffer;

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH2(VOID, Prm_FreeDMABuffer,

/*  SYNOPSIS */
	AROS_LHA(APTR, buffer, A0),
	AROS_LHA(ULONG, size, D0),

/*  LOCATION */
	struct Library *, PrometheusBase, 17, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;

   if(buffer != NULL && size != 0)
      FreeMem(buffer, size);

   return;

   AROS_LIBFUNC_EXIT
}



/***************************************************************************

    NAME */
	AROS_LH1(APTR, Prm_GetPhysicalAddr,

/*  SYNOPSIS */
	AROS_LHA(APTR, address, D0),

/*  LOCATION */
	struct Library *, PrometheusBase, 18, Prometheus)

/*  FUNCTION

    INPUTS

    RESULT

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   return address;

   AROS_LIBFUNC_EXIT
}



static VOID WrapperIRQ(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw_info)
{
   struct Interrupt *interrupt;

   interrupt = (struct Interrupt *)irq->h_Data;
   AROS_UFC2(BOOL, interrupt->is_Code,
      AROS_UFCA(APTR, interrupt->is_Data, A1),
      AROS_UFCA(APTR, interrupt->is_Code, A5));

   return;
}




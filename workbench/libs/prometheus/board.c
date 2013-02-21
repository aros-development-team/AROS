/*
    Copyright © 2005-2013, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/prometheus.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "prometheus_intern.h"

#define KernelBase (base->kernelBase)

/* Private prototypes */

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

static const struct Node foreign_owner =
{
    .ln_Name = "AROS"
};

static UPINT GetOwner(struct LibBase *base, PCIBoard *board)
{
    const struct Node *owner = board->owner;

    if (!owner)
    {
        /*
         * The board is not owned via prometheus.library API.
         * But it could be owned via some other API. In this case
         * we supply own struct Node.
         */
        IPTR aros_owner;

        OOP_GetAttr(board->aros_board, aHidd_PCIDevice_Owner, &aros_owner);
        if (aros_owner)
            owner = &foreign_owner;
    }

    return (UPINT)owner;
}

/***************************************************************************

    NAME */
        AROS_LH2(PCIBoard *, Prm_FindBoardTagList,

/*  SYNOPSIS */
        AROS_LHA(PCIBoard *, previous, A0),
        AROS_LHA(struct TagItem *, tag_list, A1),

/*  LOCATION */
        struct Library *, PrometheusBase, 5, Prometheus)

/*  FUNCTION
        Find the board whose properties match the given set
        of attributes.

    INPUTS
        previous - an opaque pointer to previously found board,
                   or NULL to start the search from the beginning
        tag_list - a pointer to a taglist specifying attributes to
                   match against. If NULL, then all boards will be
                   considered matching.

    RESULT
        A pointer to next matching board object or NULL if the search
        has ended and there is no more match.
    
    NOTES
        You can search for boards with some specific owner using
        PRM_BoardOwner tag. However note that in AROS prometheus.library
        is a wrapper on top of native object-oriented framework. This
        framework uses different concept of device ownership, and
        prometheus.library cannot determine correct owner value for devices
        locked using those APIs. Those devices are treated as having the
        same owner named "AROS", however in reality their owners will be
        different.

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   struct LibBase *base = (APTR)PrometheusBase;
   Tag tag, aros_tag;
   struct TagItem *temp_tag_list;
   struct TagItem *tag_item;
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
         
         switch (tag)
         {
         case PRM_BoardOwner:
            board_data = GetOwner(base, board);
            break;
            
         default:
            aros_tag = GetTagData(tag, TAG_DONE, (const struct TagItem *)map_tag_list);
            if (aros_tag != TAG_DONE)
               OOP_GetAttr(board->aros_board, base->pcidevice_attr_base + aros_tag,
                           &board_data);
            else
               success = FALSE;
            break;
         }

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
        Returns information about the board according to the
        specified taglist.

    INPUTS
        board    - an opaque pointer to board object to query
        tag_list - a list of attributes to query. ti_Data for
                   every tag should be a pointer to IPTR storage
                   where the data will be written. For unrecognized
                   tags a value of 0 will be returned. Tags with
                   ti_Data set to NULL will be skipped.

    RESULT
        Number of succesfully processed tags.

    NOTES
        AROS implementation of prometheus.library is a wrapper on top of
        object-oriented driver stack. Software can use either
        prometheus.library, or some other wrapper API (like openpci.library)
        or HIDD object-oriented API directly. Concept of device ownership
        is different across different APIs, so this method returns correct
        device owner only if the device was locked using prometheus.library's
        Prm_SetBoardAttrsTagList() function. If device's owner uses another
        API, prometheus.library will specify "AROS" default name.

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
      if (!tag_item->ti_Data)
         continue;

      if(tag_item->ti_Tag == PRM_BoardOwner)
      {
         tag_data_ptr = (UPINT *)tag_item->ti_Data;
         *tag_data_ptr = GetOwner(base, board);
         count++;
      }
      else
      {
         aros_tag = GetTagData(tag_item->ti_Tag, TAG_DONE, map_tag_list);
         if(aros_tag != TAG_DONE)
         {
            OOP_GetAttr(board->aros_board,
               base->pcidevice_attr_base + aros_tag,
               (IPTR *)tag_item->ti_Data);
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

   struct LibBase *base = (struct LibBase *)PrometheusBase;
   ULONG count = 0;
   struct Node *new_owner;
   struct TagItem *tag_item;

   tag_item = FindTagItem(PRM_BoardOwner, tag_list);
   if(tag_item != NULL)
   {
      new_owner = (APTR)tag_item->ti_Data;

      if (new_owner)
      {
         CONST_STRPTR new_str = new_owner->ln_Name;
         CONST_STRPTR old_str;

         /* Just in case. We cannot specify NULL owner to PCIDevice::Obtain() */
         if (!new_str)
            new_str = base->lib_header.lib_Node.ln_Name;
         
         old_str = HIDD_PCIDevice_Obtain(board->aros_board, new_str);
         if (!old_str)
         {
            board->owner = new_owner;
            count++;
         }
      }
      else if (board->owner)
      {
         HIDD_PCIDevice_Release(board->aros_board);
         board->owner = NULL;
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

   struct LibBase *base = (struct LibBase *)PrometheusBase;

   return HIDD_PCIDevice_ReadConfigByte(board->aros_board, offset);

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

   struct LibBase *base = (struct LibBase *)PrometheusBase;

   HIDD_PCIDevice_WriteConfigByte(board->aros_board, offset, data);

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

   struct LibBase *base = (struct LibBase *)PrometheusBase;

   return HIDD_PCIDevice_ReadConfigWord(board->aros_board, offset & ~0x1);

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

   struct LibBase *base = (struct LibBase *)PrometheusBase;

   HIDD_PCIDevice_WriteConfigWord(board->aros_board, offset & ~0x1, data);

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

   struct LibBase *base = (struct LibBase *)PrometheusBase;

   return HIDD_PCIDevice_ReadConfigLong(board->aros_board, offset & ~0x3);

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

   struct LibBase *base = (struct LibBase *)PrometheusBase;

   HIDD_PCIDevice_WriteConfigLong(board->aros_board, offset & ~0x3, data);

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
   BOOL success = FALSE;

   /* Add AROS int to system */
   if (board)
      success = HIDD_PCIDevice_AddInterrupt(board->aros_board, interrupt);

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
      HIDD_PCIDevice_RemoveInterrupt(board->aros_board, interrupt);

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
        Allocate memory region accessible by PCI DMA.

    INPUTS
        size - Size of the region to allocate. NULL is safe
               input, in this case the function fails.

    RESULT
        A pointer to allocated region or NULL upon failure.
        The region will always be LONG-aligned.

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   /*
    * This should be OK for any "direct" PCI bus.
    * Note that this also relies on AllocMem()'s ability to return
    * NULL when zero size is given.
    */
   return AllocMem(size, MEMF_31BIT);

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
        Free memory buffer allocated by Prm_AllocDMABuffer().

    INPUTS
        buffer - a pointer to a buffer to free. NULL is a safe value,
                 in this case the function does nothing.
        size   - size of the buffer. Zero is a safe value, in this case
                 the function does nothing.

    RESULT
        None.

***************************************************************************/
{
   AROS_LIBFUNC_INIT

   /*
    * Note that this relies on FreeMem()'s ability to ignore
    * zero buffer or size.
    */
   FreeMem(buffer, size);

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

   struct LibBase *base = (struct LibBase *)PrometheusBase;

   /* This should be OK for any "direct" PCI bus */
   return KrnVirtualToPhysical(address);

   AROS_LIBFUNC_EXIT
}

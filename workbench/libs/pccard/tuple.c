/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

/*

File: tuple.c
Copyright (C) 1999 David A. Hinds
Copyright (C) 2001,2002 Neil Cafferkey

Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1

*/


#include <exec/memory.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/pccard.h>

#include "pccard_intern.h"

#define MAX_TUPLE PCCARD_TPL_ORG

#define MAX_POWER_COUNT 3
#define POWER_PARAM_COUNT 7
#define TIMING_COUNT 3

#define MAX_REGION_COUNT 4


static struct TagItem device_tag_list[]=
{
   {PCCARD_RegionCount,0},
   {PCCARD_RegionLists,0},
   {TAG_END}
};


static struct TagItem longlinkcb_tag_list[]=
{
   {TAG_END}
};


static struct TagItem configcb_tag_list[]=
{
   {TAG_END}
};


static struct TagItem cftableentrycb_tag_list[]=
{
   {TAG_END}
};


static struct TagItem longlinkmfc_tag_list[]=
{
   {TAG_END}
};


static struct TagItem bar_tag_list[]=
{
   {TAG_END}
};


static struct TagItem checksum_tag_list[]=
{
   {TAG_END}
};


static struct TagItem longlinka_tag_list[]=
{
   {TAG_END}
};


static struct TagItem longlinkc_tag_list[]=
{
   {TAG_END}
};


static struct TagItem linktarget_tag_list[]=
{
   {TAG_END}
};


static struct TagItem nolink_tag_list[]=
{
   {TAG_END}
};


static struct TagItem vers1_tag_list[]=
{
   {PCCARD_MajorVersion,0},
   {PCCARD_MinorVersion,0},
   {PCCARD_InfoStringCount,0},
   {PCCARD_InfoStrings,0},
   {TAG_END}
};


static struct TagItem altstr_tag_list[]=
{
   {TAG_END}
};


static struct TagItem jedecc_tag_list[]=
{
   {TAG_END}
};


static struct TagItem jedeca_tag_list[]=
{
   {TAG_END}
};


static struct TagItem config_tag_list[]=
{
   {PCCARD_RegisterBase,0},
   {PCCARD_ModeCount,0},
   {TAG_END}
};


static struct TagItem cftableentry_tag_list[]=
{
   {PCCARD_ModeNo,0},
   {PCCARD_Flags,0},
   {PCCARD_VCCPowerTags,0},
   {PCCARD_VPP1PowerTags,0},
   {PCCARD_VPP2PowerTags,0},
   {PCCARD_WaitTimingTags,0},
   {PCCARD_ReadyTimingTags,0},
   {PCCARD_ReservedTimingTags,0},
   {PCCARD_IOFlags,0},
   {PCCARD_IOLineCount,0},
   {PCCARD_IOWinCount,0},
   {PCCARD_IOWinBases,0},
   {PCCARD_IOWinLengths,0},
   {PCCARD_IRQFlags,0},
   {PCCARD_IRQMask,0},
   {PCCARD_MemWinCount,0},
   {PCCARD_MemWinBases,0},
   {PCCARD_MemWinHostBases,0},
   {PCCARD_MemWinLengths,0},
   {TAG_END}
};


static struct TagItem deviceoc_tag_list[]=
{
   {TAG_END}
};


static struct TagItem deviceoa_tag_list[]=
{
   {TAG_END}
};


static struct TagItem devicegeo_tag_list[]=
{
   {TAG_END}
};


static struct TagItem devicegeoa_tag_list[]=
{
   {TAG_END}
};


static struct TagItem manfid_tag_list[]=
{
   {PCCARD_Maker,0},
   {PCCARD_Product,0},
   {TAG_END}
};


static struct TagItem funcid_tag_list[]=
{
   {PCCARD_Type,0},
   {PCCARD_Flags,0},
   {TAG_END}
};


static struct TagItem funce_tag_list[]=
{
   {TAG_END}
};


static struct TagItem swil_tag_list[]=
{
   {TAG_END}
};


static struct TagItem vers2_tag_list[]=
{
   {TAG_END}
};


static struct TagItem format_tag_list[]=
{
   {TAG_END}
};


static struct TagItem geometry_tag_list[]=
{
   {TAG_END}
};


static struct TagItem byteorder_tag_list[]=
{
   {TAG_END}
};


static struct TagItem date_tag_list[]=
{
   {TAG_END}
};


static struct TagItem battery_tag_list[]=
{
   {TAG_END}
};


static struct TagItem org_tag_list[]=
{
   {TAG_END}
};


static struct TagItem *const tag_lists[]=
{
   NULL,
   device_tag_list,
   longlinkcb_tag_list,
   NULL,
   configcb_tag_list,
   cftableentrycb_tag_list,
   longlinkmfc_tag_list,
   bar_tag_list,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   checksum_tag_list,
   longlinka_tag_list,
   longlinkc_tag_list,
   linktarget_tag_list,
   nolink_tag_list,
   vers1_tag_list,
   altstr_tag_list,
   device_tag_list,
   jedecc_tag_list,
   jedeca_tag_list,
   config_tag_list,
   cftableentry_tag_list,
   deviceoc_tag_list,
   deviceoa_tag_list,
   devicegeo_tag_list,
   devicegeoa_tag_list,
   manfid_tag_list,
   funcid_tag_list,
   funce_tag_list,
   swil_tag_list,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   vers2_tag_list,
   format_tag_list,
   geometry_tag_list,
   byteorder_tag_list,
   date_tag_list,
   battery_tag_list,
   org_tag_list
};


static struct TagItem power_tag_list[]=
{
   {PCCARD_NominalVoltage,0},
   {PCCARD_MinVoltage,0},
   {PCCARD_MaxVoltage,0},
   {PCCARD_StaticCurrent,0},
   {PCCARD_AverageCurrent,0},
   {PCCARD_PeakCurrent,0},
   {PCCARD_DownCurrent,0},
   {PCCARD_Flags,0},
   {TAG_END}
};


static struct TagItem timing_tag_list[]=
{
   {PCCARD_Value,0},
   {PCCARD_Scale,0},
   {TAG_END}
};


static struct TagItem region_tag_list[]=
{
   {PCCARD_Flags,0},
   {PCCARD_Type,0},
   {PCCARD_Speed,0},
   {PCCARD_Base,0},
   {PCCARD_Length,0},
   {TAG_END}
};


static const Tag sublist_tags[]=
{
   PCCARD_VCCPowerTags,
   PCCARD_VPP1PowerTags,
   PCCARD_VPP2PowerTags,
   PCCARD_WaitTimingTags,
   PCCARD_ReadyTimingTags,
   PCCARD_ReservedTimingTags,
   TAG_END
};


static const Tag array_tags[]=
{
   PCCARD_IOWinBases,
   PCCARD_IOWinLengths,
   PCCARD_MemWinBases,
   PCCARD_MemWinHostBases,
   PCCARD_MemWinLengths,
   TAG_END
};


static const Tag string_array_tags[]=
{
   PCCARD_InfoStrings,
   TAG_END
};


static const Tag sublist_array_tags[]=
{
   PCCARD_RegionLists,
   TAG_END
};


static const UBYTE region_speeds[]=
{
   0,250,200,150,100
};


static const UBYTE mantissas[]=
{
   10,12,13,15,20,25,30,35,40,45,50,55,60,70,80,90
};


static const ULONG exponents[]=
{
   1,10,100,1000,10000,100000,1000000,10000000
};


/* Convert an extended speed byte to a time in nanoseconds */
#define SPEED_CVT(v) \
   (mantissas[(((v)>>3)&0xf)-1]*exponents[(v)&0x7]/10)


/* Convert a power byte to a current in 0.1 microamps */
#define POWER_CVT(v) \
   (mantissas[((v)>>3)&0xf]*exponents[(v)&0x7]/10)


#define POWER_SCALE(v) (exponents[(v)&0x7])



static BOOL ParseDevice(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base);
static BOOL ParseVers1(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base);
static BOOL ParseConfig(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base);
static BOOL ParseCfTableEntry(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base);
static BOOL ParseManfID(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base);
static BOOL ParseFuncID(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base);
static BOOL ParseStrings(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base);
static ULONG StrLen(const TEXT *s);



/****** pccard.library/PCCard_GetTupleInfo *********************************
*
*   NAME
*	PCCard_GetTupleInfo -- Parse a CIS tuple.
*
*   SYNOPSIS
*	tag_list = PCCard_GetTupleInfo(tuple)
*	D0                             A0
*
*	struct TagItem *PCCard_GetTupleInfo(UBYTE *);
*
*   FUNCTION
*	Reads the information held within a CIS tuple and presents it in the
*	form of a tag list. The returned tag list is read-only and must be
*	relinquished via PCCard_FreeTupleInfo().
*
*	The tags available for each tuple type are described below.
*
*	PCCARD_TPL_DEVICE/PCCARD_TPL_DEVICEA:
*	    PCCARD_RegionCount - The number of memory regions in either
*	        common memory (PCCARD_TPL_DEVICE) or attribute memory
*	        (PCCARD_TPL_DEVICEA).
*	    PCCARD_RegionLists - An array of tag lists, one for each memory
*	        region. Each sub-list may have the following tags:
*	        PCCARD_Flags - Currently only PCCARD_REGIONF_WP (write
*	            protection) is defined.
*	        PCCARD_Type - Region's memory type.
*	        PCCARD_Speed - Speed in nanoseconds.
*	        PCCARD_Base - Offset of start of region.
*	        PCCARD_Length - Length of region.
*
*	PCCARD_TPL_VERS1:
*	    PCCARD_MajorVersion - ?
*	    PCCARD_MinorVersion - ?
*	    PCCARD_InfoStringCount - Number of strings available.
*	    PCCARD_InfoStrings - An array of card information strings.
*
*	PCCARD_TPL_CONFIG:
*	    PCCARD_RegisterBase - Offset within attribute memory of
*	        configuration registers.
*	    PCCARD_ModeCount - Number of operating modes.
*
*	PCCARD_TPL_CFTABLEENTRY:
*	    PCCARD_ModeNo - Value to write to the COR to select this mode.
*	    PCCARD_Flags - See pccard.h for details.
*	    PCCARD_VCCPowerTags - Tag list for VCC power. The following tags
*	        may be present (voltages in 10-uV units, currents in 100-nA
*	        units):
*	        PCCARD_NominalVoltage - Nominal supply voltage.
*	        PCCARD_MinVoltage - Maximum supply voltage.
*	        PCCARD_MaxVoltage - Minimum supply voltage.
*	        PCCARD_StaticCurrent - Continuous supply current required.
*	        PCCARD_AverageCurrent - Maximum current averaged over one
*	            second.
*	        PCCARD_PeakCurrent - Maximum current averaged over 10 ms.
*	        PCCARD_DownCurrent - Current required in power-down mode.
*	        PCCARD_Flags - See pccard.h for details.
*	    PCCARD_VPP1PowerTags - Tag list for VPP1 power. Uses the same
*	        tags as PCCARD_VCCPowerTags.
*	    PCCARD_VPP2PowerTags - Tag list for VPP2 power. Uses the same
*	        tags as PCCARD_VCCPowerTags.
*	    PCCARD_WaitTimingTags - Tag list for wait timing. The following
*	        tags may be present:
*	        PCCARD_Value - Base value/mantissa (in nanoseconds).
*	        PCCARD_Scale - Multiplier/exponent to be applied to
*	            PCCARD_Value.
*	    PCCARD_ReadyTimingTags - Tag list for ready timing. Uses the
*	        same tags as PCCARD_WaitTimingTags.
*	    PCCARD_ReservedTimingTags - Tag list for reserved timing. Uses
*	        the same tags as PCCARD_WaitTimingTags.
*	    PCCARD_IOFlags - See pccard.h for details.
*	    PCCARD_IOLineCount - Number of IO lines decoded by the card.
*	    PCCARD_IOWinCount - Number of IO windows.
*	    PCCARD_IOWinBases - An array of ULONGs giving the window bases
*	        as offsets into the card's IO space.
*	    PCCARD_IOWinLengths - An array of ULONGs giving the lengths of
*	        the IO windows.
*	    PCCARD_IRQFlags - See pccard.h for details.
*	    PCCARD_IRQMask - Interrupt numbers that may be used.
*	    PCCARD_MemWinCount - Number of memory windows.
*	    PCCARD_MemWinBases - An array of ULONGs giving the window bases
*	        as offsets into the card's memory space.
*	    PCCARD_MemWinHostBases - An array of ULONGs giving the window
*	        bases as offsets into the host memory space.
*	    PCCARD_MemWinLengths - An array of ULONGs giving the lengths of
*	        the memory windows.
*
*	PCCARD_TPL_MANFID:
*	    PCCARD_Maker - The card's manufacturer ID.
*	    PCCARD_Product - The card's product ID relative to its
*	        manufacturer.
*
*	PCCARD_TPL_FUNCID:
*	    PCCARD_Type - The card's function, eg. network or memory.
*	    PCCARD_Flags - Card initialisation flags.
*
*   INPUTS
*	tuple - Pointer to the tuple.
*
*   RESULT
*	tag_list - A tag list containing the tuple's properties.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*	PCCard_FreeTupleInfo()
*
****************************************************************************
*
*/

AROS_LH1(struct TagItem *, PCCard_GetTupleInfo,
    AROS_LHA(const UBYTE*, tuple, A0),
    struct PCCardBase *, PCCardBase, 5, PCCard)
{
   AROS_LIBFUNC_INIT

   BOOL success=TRUE;
   struct TagItem *tag_list=NULL;
   UBYTE tuple_type;

   tuple_type=tuple[0];
   if(tuple_type<=MAX_TUPLE)
      tag_list=CloneTagItems(tag_lists[tuple_type]);
   if(tag_list==NULL)
      success=FALSE;

   if(success)
   {
      switch(tuple_type)
      {
      case PCCARD_TPL_DEVICE:
      case PCCARD_TPL_DEVICEA:
         ParseDevice(tuple,tag_list,PCCardBase);
         break;
      case PCCARD_TPL_VERS1:
         ParseVers1(tuple,tag_list,PCCardBase);
         break;
      case PCCARD_TPL_CONFIG:
         ParseConfig(tuple,tag_list,PCCardBase);
         break;
      case PCCARD_TPL_CFTABLEENTRY:
         ParseCfTableEntry(tuple,tag_list,PCCardBase);
         break;
      case PCCARD_TPL_MANFID:
         ParseManfID(tuple,tag_list,PCCardBase);
         break;
      case PCCARD_TPL_FUNCID:
         ParseFuncID(tuple,tag_list,PCCardBase);
         break;
      default:
         break;
      }
   }

   return tag_list;

   AROS_LIBFUNC_EXIT
}



static BOOL ParseDevice(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base)
{
   BOOL success=TRUE;
   struct TagItem **lists,*sub_tag_list,*sub_tag;
   ULONG flags=0,speed=0,region_base=0,region_length;
   UBYTE region_count;

   /* Allocate tag-list array */

   tuple+=2;

   lists=AllocVec(sizeof(struct TagItem *)*(MAX_REGION_COUNT+1),
      MEMF_PUBLIC|MEMF_CLEAR);
   if(lists==NULL)
      success=FALSE;

   /* Get region info */

   if(success)
   {

      for(region_count=0;*tuple!=0xff;region_count++)
      {
         sub_tag_list=CloneTagItems(region_tag_list);
         if(sub_tag_list==NULL)
            success=FALSE;

         if(success)
         {
            lists[region_count]=sub_tag_list;

            /* Get flags and region type */

            if((*tuple&0x8)!=0)
               flags=PCCARD_REGIONF_WP;
            NextTagItem(&sub_tag_list)->ti_Data=flags;
            NextTagItem(&sub_tag_list)->ti_Data=*tuple>>4;

            /* Get speed */

            sub_tag=NextTagItem(&sub_tag_list);
            switch(*tuple&0x7)
            {
            case 0:
               sub_tag->ti_Tag=TAG_IGNORE;
               break;
            case 7:
               tuple++;
               speed=SPEED_CVT(*tuple);
               break;
            default:
               speed=region_speeds[*tuple&0x7];
            }
            sub_tag->ti_Data=speed;
            tuple++;

            /* Get base and length */

            NextTagItem(&sub_tag_list)->ti_Data=region_base;
            region_length=((*tuple>>3)+1)*(512<<((*tuple&0x7)*2));
            NextTagItem(&sub_tag_list)->ti_Data=region_length;
            region_base+=region_length;
            tuple++;
         }
      }

      NextTagItem(&tag_list)->ti_Data=region_count;
   }

   return success;
}



static BOOL ParseVers1(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base)
{
   /* Get version no. */

   tuple+=2;
   NextTagItem(&tag_list)->ti_Data=*tuple++;
   NextTagItem(&tag_list)->ti_Data=*tuple++;

   /* Get info strings */

   return ParseStrings(tuple,tag_list,base);
}



static BOOL ParseConfig(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base)
{
   ULONG reg_base=0;
   UBYTE base_size;
   const UBYTE *p,*q;

   p=tuple+2;
   base_size=(*p++)&0x3;

   FindTagItem(PCCARD_ModeCount,tag_list)->ti_Data=*(p++);

   for(q=p+base_size;q>=p;q--)
   {
      reg_base<<=8;
      reg_base|=*q;
   }

   FindTagItem(PCCARD_RegisterBase,tag_list)->ti_Data=reg_base;

   return TRUE;
}



static BOOL ParseCfTableEntry(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base)
{
   ULONG n,flags=0,scale,*win_bases = NULL,*host_bases = NULL,*win_lengths = NULL;
   UBYTE base_size,length_size,i=0,j,present,power_count,features,
      timing_scales[TIMING_COUNT],win_count=0,sub_flags=0,sizes=0,mask=0;
   const UBYTE *p,*q;
   struct TagItem *tag,*temp_tag=tag_list,*temp_sub_tag,*sub_tag,
      *sub_tag_list;
   BOOL success=TRUE;

   /* Get mode no. */

   p=tuple+2;
   NextTagItem(&temp_tag)->ti_Data=*p&0x3f;

   if(*p&0x40)
      flags|=PCCARD_CFTABLEF_DEFAULT;
   if(*p&0x80)
      p++;

   NextTagItem(&temp_tag)->ti_Data=flags;

   features=*(++p);
   p++;

   /* Parse power data */

   for(power_count=features&0x3;i<MAX_POWER_COUNT;i++)
   {
      tag=NextTagItem(&temp_tag);

      if(i<power_count)
      {
         temp_sub_tag=sub_tag_list=CloneTagItems(power_tag_list);
         if(sub_tag_list==NULL)
            success=FALSE;

         if(success)
         {
            present=*(p++);
            flags=0;
            for(j=0;j<POWER_PARAM_COUNT;j++)
            {
               sub_tag=NextTagItem(&temp_sub_tag);

               if((present&1)!=0)
               {
                  n=POWER_CVT(*p);
                  scale=POWER_SCALE(*p);
                  while((*(p++)&0x80)!=0)
                  {
                     if((*p&0x7f)<100)
                        n+=(*p&0x7f)*scale/100;
                     else if(*p==0x7d)
                        flags|=PCCARD_POWERF_HIGHZOK;
                     else if(*p==0x7e)
                        n=0;
                     else if(*p==0x7f)
                        flags|=PCCARD_POWERF_HIGHZREQ;
                  }
                  sub_tag->ti_Data=n;
               }
               else
                  sub_tag->ti_Tag=TAG_IGNORE;

               present>>=1;
            }
            NextTagItem(&temp_sub_tag)->ti_Data=flags;
         }

         tag->ti_Data=(UPINT)sub_tag_list;
      }
      else
         tag->ti_Tag=TAG_IGNORE;
   }

   /* Parse timing data */

   if((features&0x4)!=0)
   {
      scale=*(p++);
      scale<<=1;
      scale|=0x1;
      for(i=0;i<TIMING_COUNT;i++)
      {
         timing_scales[i]=scale&0x7;
         scale>>=3;
      }
      if(timing_scales[0]!=7)
         timing_scales[0]>>=1;

      for(i=0;i<TIMING_COUNT;i++)
      {
         tag=NextTagItem(&temp_tag);

         if(timing_scales[i]!=7)
         {
            temp_sub_tag=sub_tag_list=CloneTagItems(timing_tag_list);
            if(sub_tag_list==NULL)
               success=FALSE;

            if(success)
            {
               p++;
               NextTagItem(&temp_sub_tag)->ti_Data=SPEED_CVT(*p);
               NextTagItem(&temp_sub_tag)->ti_Data=
                  exponents[timing_scales[i]];
            }

            tag->ti_Data=(UPINT)sub_tag_list;
         }
         else
            tag->ti_Tag=TAG_IGNORE;
      }

      p++;
   }
   else
   {
      tag=NextTagItem(&temp_tag);
      tag->ti_Tag=TAG_SKIP;
      tag->ti_Data=TIMING_COUNT-1;
      temp_tag=tag;
   }

   /* Parse IO windows data */

   if((features&0x8)!=0)
   {
      sub_flags=*p;

      if((sub_flags&0x80)!=0)
      {
         p++;
         sizes=*(p++);
         win_count=(sizes&0xf)+1;
         sizes>>=4;
         base_size=1<<((sizes&0x3)-1);
         sizes>>=2;
         length_size=1<<((sizes&0x3)-1);
      }
      else
      {
         win_count=1;
      }

      win_bases=AllocVec(win_count*sizeof(ULONG),MEMF_PUBLIC);
      win_lengths=AllocVec(win_count*sizeof(ULONG),MEMF_PUBLIC);
      if((win_bases==NULL)||(win_lengths==NULL))
         success=FALSE;

      if(success)
      {
         if((sub_flags&0x80)!=0)
         {
            for(i=0;i<win_count;i++)
            {
               for(n=0,q=p+base_size;q>p;n|=*(--q))
                  n<<=8;
               win_bases[i]=n;
               p+=base_size;

               for(n=0,q=p+length_size;q>p;n|=*(--q))
                  n<<=8;
               win_lengths[i]=n+1;
               p+=length_size;
            }
         }
         else
         {
            *win_bases=0;
            *win_lengths=1<<(sub_flags&0x1f);
         }
      }
   }

   tag=NextTagItem(&temp_tag);
   if((features&0x8)==0)
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=sub_flags&(PCCARD_IOF_8BIT|PCCARD_IOF_16BIT);

   tag=NextTagItem(&temp_tag);
   if((features&0x8)==0)
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=sub_flags&0x1f;

   tag=NextTagItem(&temp_tag);
   if((features&0x8)==0)
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=win_count;

   tag=NextTagItem(&temp_tag);
   if((features&0x8)==0)
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=(UPINT)win_bases;

   tag=NextTagItem(&temp_tag);
   if((features&0x8)==0)
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=(UPINT)win_lengths;

   /* Parse IRQ data */

   if((features&0x10)!=0)
   {
      sub_flags=*(p++);
      if((sub_flags&0x10)!=0)
      {
         mask=*(p++);
      }
      else
      {
         mask=1<<(sub_flags&0xf);   /* TO DO: check if single interrupt is "desired" or "allowed" */
         sub_flags&=0xe0;
      }
   }

   tag=NextTagItem(&temp_tag);
   if((features&0x10)==0)
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=sub_flags;

   tag=NextTagItem(&temp_tag);
   if((features&0x10)==0)
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=mask;

   /* Parse memory windows data */

   if((features&0x60)!=0)
   {
      if((features&0x60)==0x60)
      {
         sizes=*(p++);
         win_count=(sizes&0x7)+1;
         sizes>>=3;
         length_size=sizes&0x3;
         sizes>>=2;
         base_size=sizes&0x3;
         sizes&=0x4;
      }
      else
      {
         sizes=0;
         win_count=1;
      }

      win_bases=AllocVec(win_count*sizeof(ULONG),MEMF_PUBLIC);
      win_lengths=AllocVec(win_count*sizeof(ULONG),MEMF_PUBLIC);
      if((win_bases==NULL)||(win_lengths==NULL))
         success=FALSE;

      if(sizes!=0)
      {
         host_bases=AllocVec(win_count*sizeof(ULONG),MEMF_PUBLIC);
         if(host_bases==NULL)
            success=FALSE;
      }

      if(success)
      {
         if((features&0x60)==0x60)
         {
            for(i=0;i<win_count;i++)
            {
               for(n=0,q=p+length_size;q>p;n<<=8)
                  n|=*(--q);
               win_lengths[i]=n;
               p+=length_size;

               for(n=0,q=p+base_size;q>p;n<<=8)
                  n|=*(--q);
               win_bases[i]=n;
               p+=base_size;

               if(sizes!=0)
               {
                  for(n=0,q=p+base_size;q>p;n<<=8)
                     n|=*(--q);
                  host_bases[i]=n;
                  p+=base_size;
               }
            }
         }
         else
         {
            *win_lengths=(*p<<8)+(*(p+1)<<16);
            p+=2;

            if((features&0x40)!=0)
            {
               *win_bases=(*p<<8)+(*(p+1)<<16);
               p+=2;
            }
            else
               *win_bases=0;
         }
      }
   }

   tag=NextTagItem(&temp_tag);
   if((features&0x60)==0)
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=win_count;

   tag=NextTagItem(&temp_tag);
   if((features&0x60)==0)
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=(UPINT)win_bases;

   tag=NextTagItem(&temp_tag);
   if(((features&0x60)==0)||(sizes==0))
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=(UPINT)host_bases;

   tag=NextTagItem(&temp_tag);
   if((features&0x60)==0)
      tag->ti_Tag=TAG_IGNORE;
   tag->ti_Data=(UPINT)win_lengths;

   /* Parse extra flags */

   if((features&0x80)!=0)
      flags|=*(p++);

   /* TO DO: parse extra flags and subtuples */


   /* Return */

   return success;
}



static BOOL ParseManfID(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base)
{
   /* Get manufacturer and product IDs */

   tuple+=2;
   NextTagItem(&tag_list)->ti_Data=LEWord(tuple);
   tuple+=2;
   NextTagItem(&tag_list)->ti_Data=LEWord(tuple);

   return TRUE;
}



static BOOL ParseFuncID(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base)
{
   /* Get function and intialisation info */

   tuple+=2;
   NextTagItem(&tag_list)->ti_Data=*tuple++;
   NextTagItem(&tag_list)->ti_Data=*tuple;

   return TRUE;
}



/****** pccard.library/PCCard_FreeTupleInfo ********************************
*
*   NAME
*	PCCard_FreeTupleInfo -- Relinquish a CIS tuple tag list.
*
*   SYNOPSIS
*	PCCard_FreeTupleInfo(tag_list)
*	                     A0
*
*	VOID PCCard_FreeTupleInfo(struct TagItem *);
*
*   FUNCTION
*	Frees a tag list obtained via PCCard_GetTupleInfo().
*
*   INPUTS
*	tag_list - Tuple tag list (may be NULL).
*
*   RESULT
*	None.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*	PCCard_GetTupleInfo()
*
****************************************************************************
*
*/

AROS_LH1(void, PCCard_FreeTupleInfo,
   AROS_LHA(struct TagItem*, tag_list, A0),
   struct PCCardBase *, PCCardBase, 6, PCCard)
{
   AROS_LIBFUNC_INIT

   struct TagItem *tag,*temp_tag;
   TEXT **p;

   /* Free sub-tag lists and arrays */

   if(tag_list!=NULL)
   {
      temp_tag=tag_list;
      while((tag=NextTagItem(&temp_tag))!=NULL)
      {
         if(TagInArray(tag->ti_Tag,(Tag*)sublist_tags)&&(tag->ti_Data!=0))
            PCCard_FreeTupleInfo((APTR)tag->ti_Data);
         else if(TagInArray(tag->ti_Tag,(Tag*)array_tags))
            FreeVec((APTR)tag->ti_Data);
         else if(TagInArray(tag->ti_Tag,(Tag*)string_array_tags))
         {
            for(p=(APTR)tag->ti_Data;*p!=NULL;p++)
               FreeVec(*p);
         }
      }
   }

   /* Free main list and return */

   FreeTagItems(tag_list);
   return;

   AROS_LIBFUNC_EXIT
}



static BOOL ParseStrings(const UBYTE *tuple,struct TagItem *tag_list,
   struct PCCardBase *base)
{
   UBYTE i=0,string_count=0,length;
   const UBYTE *p;
   BOOL success=TRUE;
   TEXT **strings;

   /* Count strings */

   for(p=tuple;*p!=0xff;p++)
      if(*p=='\0')
         string_count++;

   /* Allocate memory for and copy strings */

   strings=AllocVec((string_count+1)*sizeof(APTR),MEMF_PUBLIC|MEMF_CLEAR);
   if(strings==NULL)
      success=FALSE;

   if(success)
   {
      for(p=tuple;i<string_count;i++)
      {
         length=StrLen(p)+1;
         strings[i]=AllocVec(length,MEMF_PUBLIC);
         if(strings[i]!=NULL)
            CopyMem(p,strings[i],length);
         else
            success=FALSE;
         p+=length;
      }
   }

   NextTagItem(&tag_list)->ti_Data=string_count;
   NextTagItem(&tag_list)->ti_Data=(UPINT)strings;

   /* Return */

   return success;
}



static ULONG StrLen(const TEXT *s)
{
   const TEXT *p;

   for(p=s;*p!='\0';p++);
   return p-s;
}

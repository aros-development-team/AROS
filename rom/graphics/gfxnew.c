/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/monitor.h>
#include <graphics/view.h>
#include <graphics/gfxnodes.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH1(struct ExtendedNode *, GfxNew,

/*  SYNOPSIS */

      AROS_LHA( ULONG, node_type, D0),

/*  LOCATION */

      struct GfxBase *, GfxBase, 110, Graphics)

/*  FUNCTION
      Allocate a special graphics extended data structure. The type of
      structure to be allocated is passed in the node_type identifier.

    INPUTS
      node_type = the type of garphics extended data structure to allocate.
                  (see gfxnodes.h for identifier definitions.)

    RESULT
      A pointer to the allocated graphics node or NULL if the allocation
      failed

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
      graphics/gfxnodes.h GfxFree(), GfxAssociate(), GfxLookUp()

    INTERNALS

    HISTORY

******************************************************************************/
{
  AROS_LIBFUNC_INIT

  struct ExtendedNode * Result;
  const ULONG GfxNew_memsizes[] = { 0,
                                    sizeof(struct ViewExtra),
                                    sizeof(struct ViewPortExtra),
                                    sizeof(struct SpecialMonitor),
                                    sizeof(struct MonitorSpec)
                                  };


  if (node_type >= VIEW_EXTRA_TYPE && node_type <= MONITOR_SPEC_TYPE)
  {
    Result = (struct ExtendedNode *) AllocMem(GfxNew_memsizes[node_type],
                                              MEMF_CLEAR | MEMF_PUBLIC);
    if (Result)
    {
      /* do some initialisation they all have in common */
      Result->xln_Type = NT_GRAPHICS;
      Result->xln_Subsystem = SS_GRAPHICS;
      Result->xln_Subtype = (BYTE)node_type;
      /* Result->xln_Library = (APTR)GfxBase; */

      /* the following pointer has to point to some unknown routine */
      /* Result->xln_Init = ???; */

      /* lets get more specific now !*/
      switch(node_type)
      {
        case VIEW_EXTRA_TYPE:
          ((struct ViewExtra *)Result)->Monitor = (struct MonitorSpec *)
             FindName((struct List *)(&(GfxBase -> MonitorList)), DEFAULT_MONITOR_NAME);
          break;
        case VIEWPORT_EXTRA_TYPE:
          break;
        case SPECIAL_MONITOR_TYPE:
          /* ((struct SpecialMonitor *)Result).do_monitor = */
        break;
        case MONITOR_SPEC_TYPE:
          /* ((struct MonitorSpec *)Result)->ms_transform = */
          /* ((struct MonitorSpec *)Result)->ms_translate = */
          /* ((struct MonitorSpec *)Result)->ms_scale = */
          /* ((struct MonitorSpec *)Result)->ms_xoffset = */
          /* ((struct MonitorSpec *)Result)->ms_yoffset = */
          /* ((struct MonitorSpec *)Result)->ms_maxoscan = */
          /* ((struct MonitorSpec *)Result)->ms_videoscan = */
          /* ((struct MonitorSpec *)Result)->ms_reserved00 = */
          /* ((struct MonitorSpec *)Result)->ms_reserved01 = */
          break;
      }
    return Result;
    } /* could allocate requested memory */
  } /* node_type is valid */
  return NULL;

  AROS_LIBFUNC_EXIT
} /* GfxNew */

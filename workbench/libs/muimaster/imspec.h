/* 
    Copyright  1999, David Le Corfec.
    Copyright  2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_IMSPEC_H
#define _MUI_IMSPEC_H

/* API change on Feb-2003 by dlc :
 * MUIM_Setup
   {
      imspec = zune_imspec_setup(spec, mri);
      MUIM_Show
      {
          zune_imspec_show(imspec, obj);
          MUIM_Draw
          {
              zune_imspec_draw(imspec, ...)
          }
          zune_imspec_hide(imspec);
      }
      MUIM_Hide
      zune_imspec_cleanup(imspec);
      imspec = NULL;
   }
   MUIM_Cleanup
 *
 *
 * zune_imspec_setup() (called in MUIM_Setup) will create and return an internal
 * structure from an external specification.
 * zune_imspec_cleanup() (called in MUIM_Cleanup) will free an internal specification.
 */

struct MUI_ImageSpec_intern *zune_imspec_setup(IPTR s, struct MUI_RenderInfo *mri);
struct MUI_ImageSpec_intern *zune_imspec_setup_dummy(IPTR s);
void zune_imspec_cleanup(struct MUI_ImageSpec_intern *spec);
BOOL zune_imspec_askminmax(struct MUI_ImageSpec_intern *spec, struct MUI_MinMax *minmax);
void zune_imspec_show(struct MUI_ImageSpec_intern *spec, Object *obj);
void zune_imspec_hide(struct MUI_ImageSpec_intern *spec);
void zune_imspec_draw (struct MUI_ImageSpec_intern *img, struct MUI_RenderInfo *mri,
		 LONG left, LONG top, LONG width, LONG height,
		 LONG xoffset, LONG yoffset, LONG state);
void zune_imspec_drawbuffered (struct MUI_ImageSpec_intern *spec, struct RastPort *rp, struct MUI_RenderInfo *mri,
		 LONG left, LONG top, LONG width, LONG height,
		 LONG xoffset, LONG yoffset, LONG state, LONG dx, LONG dy, WORD mode, LONG abs_l, LONG abs_t, LONG abs_r, LONG abs_b);

/*  const char *zune_imspec_to_string(struct MUI_ImageSpec_intern *spec); */
STRPTR zune_image_spec_duplicate(IPTR in);
void zune_image_spec_free(CONST_STRPTR spec);

#endif

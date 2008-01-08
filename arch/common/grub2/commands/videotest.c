/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/machine/memory.h>
#include <grub/video.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/arg.h>
#include <grub/mm.h>
#include <grub/font.h>
#include <grub/term.h>

static grub_err_t
grub_cmd_videotest (struct grub_arg_list *state __attribute__ ((unused)),
                    int argc __attribute__ ((unused)),
                    char **args __attribute__ ((unused)))
{
  if (grub_video_setup (1024, 768,
                        GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != GRUB_ERR_NONE)
    return grub_errno;

  grub_getkey ();

  grub_video_color_t color;
  unsigned int x;
  unsigned int y;
  unsigned int width;
  unsigned int height;
  int i;
  struct grub_font_glyph glyph;
  struct grub_video_render_target *text_layer;
  grub_video_color_t palette[16];

  grub_video_get_viewport (&x, &y, &width, &height);

  grub_video_create_render_target (&text_layer, width, height,
                                   GRUB_VIDEO_MODE_TYPE_RGB
                                   | GRUB_VIDEO_MODE_TYPE_ALPHA);

  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);

  color = grub_video_map_rgb (0, 0, 0);
  grub_video_fill_rect (color, 0, 0, width, height);

  color = grub_video_map_rgb (255, 0, 0);
  grub_video_fill_rect (color, 0, 0, 100, 100);

  color = grub_video_map_rgb (0, 255, 255);
  grub_video_fill_rect (color, 100, 100, 100, 100);

  grub_font_get_glyph ('*', &glyph);  
  grub_video_blit_glyph (&glyph, color, 200 ,0);

  grub_video_set_viewport (x + 150, y + 150,
                           width - 150 * 2, height - 150 * 2);
  color = grub_video_map_rgb (77, 33, 77);
  grub_video_fill_rect (color, 0, 0, width, height);

  grub_video_set_active_render_target (text_layer);

  color = grub_video_map_rgb (255, 255, 255);

  grub_font_get_glyph ('A', &glyph);
  grub_video_blit_glyph (&glyph, color, 16, 16);
  grub_font_get_glyph ('B', &glyph);
  grub_video_blit_glyph (&glyph, color, 16 * 2, 16);

  grub_font_get_glyph ('*', &glyph);

  for (i = 0; i < 16; i++)
    {
      color = grub_video_map_color (i);
      palette[i] = color;
      grub_video_blit_glyph (&glyph, color, 16 + i * 16, 32);
    }

  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);

  for (i = 0; i < 255; i++)
    {
      color = grub_video_map_rgb (i, 33, 77);
      grub_video_fill_rect (color, 0, 0, width, height);
      grub_video_blit_render_target (text_layer, GRUB_VIDEO_BLIT_BLEND, 0, 0,
                                     0, 0, width, height);
    }

  grub_getkey ();

  grub_video_delete_render_target (text_layer);

  grub_video_restore ();

  for (i = 0; i < 16; i++)
    grub_printf("color %d: %08x\n", i, palette[i]);

  grub_errno = GRUB_ERR_NONE;
  return grub_errno;
}

GRUB_MOD_INIT(videotest)
{
  grub_register_command ("videotest",
                         grub_cmd_videotest,
                         GRUB_COMMAND_FLAG_BOTH,
                         "videotest",
                         "Test video subsystem",
                         0);
}

GRUB_MOD_FINI(videotest)
{
  grub_unregister_command ("videotest");
}

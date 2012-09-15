/* view.c - Graphical menu interface MVC view. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#include <grub/types.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/normal.h>
#include <grub/video.h>
#include <grub/gfxterm.h>
#include <grub/bitmap.h>
#include <grub/bitmap_scale.h>
#include <grub/term.h>
#include <grub/gfxwidgets.h>
#include <grub/time.h>
#include <grub/menu.h>
#include <grub/menu_viewer.h>
#include <grub/gfxmenu_view.h>
#include <grub/gui_string_util.h>
#include <grub/icon_manager.h>
#include <grub/i18n.h>

static void
init_terminal (grub_gfxmenu_view_t view);
static grub_video_rect_t term_rect;
static grub_gfxmenu_view_t term_view;

/* Create a new view object, loading the theme specified by THEME_PATH and
   associating MODEL with the view.  */
grub_gfxmenu_view_t
grub_gfxmenu_view_new (const char *theme_path,
		       int width, int height)
{
  grub_gfxmenu_view_t view;
  grub_font_t default_font;
  grub_video_rgba_color_t default_fg_color;
  grub_video_rgba_color_t default_bg_color;

  view = grub_malloc (sizeof (*view));
  if (! view)
    return 0;

  view->screen.x = 0;
  view->screen.y = 0;
  view->screen.width = width;
  view->screen.height = height;

  default_font = grub_font_get ("Unknown Regular 16");
  default_fg_color = grub_video_rgba_color_rgb (0, 0, 0);
  default_bg_color = grub_video_rgba_color_rgb (255, 255, 255);

  view->canvas = 0;

  view->title_font = default_font;
  view->message_font = default_font;
  view->terminal_font_name = grub_strdup ("Fixed 10");
  view->title_color = default_fg_color;
  view->message_color = default_bg_color;
  view->message_bg_color = default_fg_color;
  view->desktop_image = 0;
  view->desktop_color = default_bg_color;
  view->terminal_box = grub_gfxmenu_create_box (0, 0);
  view->title_text = grub_strdup (_("GRUB Boot Menu"));
  view->progress_message_text = 0;
  view->theme_path = 0;

  /* Set the timeout bar's frame.  */
  view->progress_message_frame.width = view->screen.width * 4 / 5;
  view->progress_message_frame.height = 50;
  view->progress_message_frame.x = view->screen.x
    + (view->screen.width - view->progress_message_frame.width) / 2;
  view->progress_message_frame.y = view->screen.y
    + view->screen.height - 90 - 20 - view->progress_message_frame.height;

  if (grub_gfxmenu_view_load_theme (view, theme_path) != 0)
    {
      grub_gfxmenu_view_destroy (view);
      return 0;
    }

  return view;
}

/* Destroy the view object.  All used memory is freed.  */
void
grub_gfxmenu_view_destroy (grub_gfxmenu_view_t view)
{
  if (!view)
    return;
  grub_video_bitmap_destroy (view->desktop_image);
  if (view->terminal_box)
    view->terminal_box->destroy (view->terminal_box);
  grub_free (view->terminal_font_name);
  grub_free (view->title_text);
  grub_free (view->progress_message_text);
  grub_free (view->theme_path);
  if (view->canvas)
    view->canvas->component.ops->destroy (view->canvas);
  grub_free (view);
}

static void
redraw_background (grub_gfxmenu_view_t view,
		   const grub_video_rect_t *bounds)
{
  if (view->desktop_image)
    {
      struct grub_video_bitmap *img = view->desktop_image;
      grub_video_blit_bitmap (img, GRUB_VIDEO_BLIT_REPLACE,
                              bounds->x, bounds->y,
			      bounds->x - view->screen.x,
			      bounds->y - view->screen.y,
			      bounds->width, bounds->height);
    }
  else
    {
      grub_video_fill_rect (grub_video_map_rgba_color (view->desktop_color),
                            bounds->x, bounds->y,
                            bounds->width, bounds->height);
    }
}

static void
draw_title (grub_gfxmenu_view_t view)
{
  if (! view->title_text)
    return;

  /* Center the title. */
  int title_width = grub_font_get_string_width (view->title_font,
                                                view->title_text);
  int x = (view->screen.width - title_width) / 2;
  int y = 40 + grub_font_get_ascent (view->title_font);
  grub_font_draw_string (view->title_text,
                         view->title_font,
                         grub_video_map_rgba_color (view->title_color),
                         x, y);
}

struct progress_value_data
{
  int visible;
  int start;
  int end;
  int value;
};

struct grub_gfxmenu_timeout_notify *grub_gfxmenu_timeout_notifications;

static void
update_timeouts (int visible, int start, int value, int end)
{
  struct grub_gfxmenu_timeout_notify *cur;

  for (cur = grub_gfxmenu_timeout_notifications; cur; cur = cur->next)
    cur->set_state (cur->self, visible, start, value, end);
}

static void
redraw_timeouts (struct grub_gfxmenu_view *view)
{
  struct grub_gfxmenu_timeout_notify *cur;

  for (cur = grub_gfxmenu_timeout_notifications; cur; cur = cur->next)
    {
      grub_video_rect_t bounds;
      cur->self->ops->get_bounds (cur->self, &bounds);
      grub_gfxmenu_view_redraw (view, &bounds);
    }
}

void 
grub_gfxmenu_print_timeout (int timeout, void *data)
{
  struct grub_gfxmenu_view *view = data;

  if (view->first_timeout == -1)
    view->first_timeout = timeout;

  update_timeouts (1, -(view->first_timeout + 1), -timeout, 0);
  redraw_timeouts (view);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    redraw_timeouts (view);
}

void 
grub_gfxmenu_clear_timeout (void *data)
{
  struct grub_gfxmenu_view *view = data;

  update_timeouts (0, 1, 0, 0);
  redraw_timeouts (view);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    redraw_timeouts (view);
}

static void
update_menu_visit (grub_gui_component_t component,
                   void *userdata)
{
  grub_gfxmenu_view_t view;
  view = userdata;
  if (component->ops->is_instance (component, "list"))
    {
      grub_gui_list_t list = (grub_gui_list_t) component;
      list->ops->set_view_info (list, view);
    }
}

/* Update any boot menu components with the current menu model and
   theme path.  */
static void
update_menu_components (grub_gfxmenu_view_t view)
{
  grub_gui_iterate_recursively ((grub_gui_component_t) view->canvas,
                                update_menu_visit, view);
}

static void
draw_message (grub_gfxmenu_view_t view)
{
  char *text = view->progress_message_text;
  grub_video_rect_t f = view->progress_message_frame;
  if (! text)
    return;

  grub_font_t font = view->message_font;
  grub_video_color_t color = grub_video_map_rgba_color (view->message_color);

  /* Border.  */
  grub_video_fill_rect (color,
                        f.x-1, f.y-1, f.width+2, f.height+2);
  /* Fill.  */
  grub_video_fill_rect (grub_video_map_rgba_color (view->message_bg_color),
                        f.x, f.y, f.width, f.height);

  /* Center the text. */
  int text_width = grub_font_get_string_width (font, text);
  int x = f.x + (f.width - text_width) / 2;
  int y = (f.y + (f.height - grub_font_get_descent (font)) / 2
           + grub_font_get_ascent (font) / 2);
  grub_font_draw_string (text, font, color, x, y);
}

void
grub_gfxmenu_view_redraw (grub_gfxmenu_view_t view,
			  const grub_video_rect_t *region)
{
  if (grub_video_have_common_points (&term_rect, region))
    grub_gfxterm_schedule_repaint ();

  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);

  redraw_background (view, region);
  if (view->canvas)
    view->canvas->component.ops->paint (view->canvas, region);
  draw_title (view);
  if (grub_video_have_common_points (&view->progress_message_frame, region))
    draw_message (view);
}

void
grub_gfxmenu_view_draw (grub_gfxmenu_view_t view)
{
  init_terminal (view);

  /* Clear the screen; there may be garbage left over in video memory. */
  grub_video_fill_rect (grub_video_map_rgb (0, 0, 0),
                        view->screen.x, view->screen.y,
                        view->screen.width, view->screen.height);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    grub_video_fill_rect (grub_video_map_rgb (0, 0, 0),
			  view->screen.x, view->screen.y,
			  view->screen.width, view->screen.height);

  update_menu_components (view);

  grub_gfxmenu_view_redraw (view, &view->screen);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    grub_gfxmenu_view_redraw (view, &view->screen);
}

static void
redraw_menu_visit (grub_gui_component_t component,
                   void *userdata)
{
  grub_gfxmenu_view_t view;
  view = userdata;
  if (component->ops->is_instance (component, "list"))
    {
      grub_video_rect_t bounds;

      component->ops->get_bounds (component, &bounds);
      grub_gfxmenu_view_redraw (view, &bounds);
    }
}

void
grub_gfxmenu_redraw_menu (grub_gfxmenu_view_t view)
{
  update_menu_components (view);

  grub_gui_iterate_recursively ((grub_gui_component_t) view->canvas,
                                redraw_menu_visit, view);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    {
      grub_gui_iterate_recursively ((grub_gui_component_t) view->canvas,
				    redraw_menu_visit, view);
    }
}

void 
grub_gfxmenu_set_chosen_entry (int entry, void *data)
{
  grub_gfxmenu_view_t view = data;

  view->selected = entry;
  grub_gfxmenu_redraw_menu (view);
}

static void
grub_gfxmenu_draw_terminal_box (void)
{
  grub_gfxmenu_box_t term_box;

  term_box = term_view->terminal_box;
  if (!term_box)
    return;
  
  term_box->set_content_size (term_box, term_rect.width,
			      term_rect.height);
  
  term_box->draw (term_box,
		  term_rect.x - term_box->get_left_pad (term_box),
		  term_rect.y - term_box->get_top_pad (term_box));
}

static void
init_terminal (grub_gfxmenu_view_t view)
{
  term_rect.width = view->screen.width * 7 / 10;
  term_rect.height = view->screen.height * 7 / 10;

  term_rect.x = view->screen.x + view->screen.width * (10 - 7) / 10 / 2;
  term_rect.y = view->screen.y + view->screen.height * (10 - 7) / 10 / 2;

  term_view = view;

  /* Note: currently there is no API for changing the gfxterm font
     on the fly, so whatever font the initially loaded theme specifies
     will be permanent.  */
  grub_gfxterm_set_window (GRUB_VIDEO_RENDER_TARGET_DISPLAY, term_rect.x,
			   term_rect.y,
			   term_rect.width, term_rect.height,
			   view->double_repaint, view->terminal_font_name, 3);
  grub_gfxterm_decorator_hook = grub_gfxmenu_draw_terminal_box;
}

/* FIXME: previously notifications were displayed in special case.
   Is it necessary?
 */
#if 0
/* Sets MESSAGE as the progress message for the view.
   MESSAGE can be 0, in which case no message is displayed.  */
static void
set_progress_message (grub_gfxmenu_view_t view, const char *message)
{
  grub_free (view->progress_message_text);
  if (message)
    view->progress_message_text = grub_strdup (message);
  else
    view->progress_message_text = 0;
}

static void
notify_booting (grub_menu_entry_t entry, void *userdata)
{
  grub_gfxmenu_view_t view = (grub_gfxmenu_view_t) userdata;

  char *s = grub_malloc (100 + grub_strlen (entry->title));
  if (!s)
    return;

  grub_sprintf (s, "Booting '%s'", entry->title);
  set_progress_message (view, s);
  grub_free (s);
  grub_gfxmenu_view_redraw (view, &view->progress_message_frame);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    grub_gfxmenu_view_redraw (view, &view->progress_message_frame);
}

static void
notify_fallback (grub_menu_entry_t entry, void *userdata)
{
  grub_gfxmenu_view_t view = (grub_gfxmenu_view_t) userdata;

  char *s = grub_malloc (100 + grub_strlen (entry->title));
  if (!s)
    return;

  grub_sprintf (s, "Falling back to '%s'", entry->title);
  set_progress_message (view, s);
  grub_free (s);
  grub_gfxmenu_view_redraw (view, &view->progress_message_frame);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    grub_gfxmenu_view_redraw (view, &view->progress_message_frame);
}

static void
notify_execution_failure (void *userdata __attribute__ ((unused)))
{
}


static struct grub_menu_execute_callback execute_callback =
{
  .notify_booting = notify_booting,
  .notify_fallback = notify_fallback,
  .notify_failure = notify_execution_failure
};

#endif

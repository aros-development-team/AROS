#ifndef AROS_GUI_GLUE_H
#define AROS_GUI_GLUE_H

/*
    Intuition windows, input and signals for Rust on AROS.
    The Rust half is ../src/lib.rs; keep the structures in step with it.
*/

#include <stdint.h>

#define AROS_GUI_WINDOW_RESIZABLE  (1u << 0)
#define AROS_GUI_WINDOW_BORDERLESS (1u << 1)
#define AROS_GUI_WINDOW_ACTIVATE   (1u << 2)

struct aros_gui_window_attrs {
    int32_t left, top;              /* < 0: let the glue choose */
    int32_t inner_width, inner_height;
    int32_t min_width, min_height;  /* 0: no limit */
    uint32_t flags;
    const char *title;
};

struct aros_gui_window_info {
    uintptr_t handle;
    int32_t left, top;
    int32_t width, height;          /* including the borders */
    int32_t inner_width, inner_height;
    int32_t border_left, border_top;
    uint32_t sigmask;
};

/* One IntuiMessage, copied out and already replied. */
struct aros_gui_message {
    uint32_t class;
    uint16_t code;
    uint16_t qualifier;
    int32_t mouse_x, mouse_y;       /* relative to the window interior */
};

int aros_gui_screen_size(int32_t *width, int32_t *height);
/* The Workbench screen's first `count` pens as 0xRRGGBB. Returns how many. */
int aros_gui_screen_palette(uint32_t *rgb, int count);

int aros_gui_window_open(const struct aros_gui_window_attrs *attrs,
    struct aros_gui_window_info *out);
void aros_gui_window_close(uintptr_t window);
void aros_gui_window_info(uintptr_t window, struct aros_gui_window_info *out);
int aros_gui_window_set_title(uintptr_t window, const char *title);
void aros_gui_window_set_inner_size(uintptr_t window, int32_t width, int32_t height);
void aros_gui_window_set_position(uintptr_t window, int32_t left, int32_t top);
void aros_gui_window_set_min_size(uintptr_t window, int32_t width, int32_t height);
void aros_gui_window_set_pointer_visible(uintptr_t window, int visible);
void aros_gui_window_focus(uintptr_t window);
int aros_gui_window_is_active(uintptr_t window);
int aros_gui_window_next_message(uintptr_t window, struct aros_gui_message *out);
/* Relative mouse mode (IDCMP_DELTAMOVE): mouse_x/mouse_y of IDCMP_MOUSEMOVE
   messages become the movement since the last one, and the pointer may sit at
   a screen edge. */
void aros_gui_window_set_delta_move(uintptr_t window, int on);
int aros_gui_window_delta_move(uintptr_t window);

/* Text for a raw key through the current keymap, in ISO-8859-1. */
int aros_gui_map_rawkey(uint16_t code, uint16_t qualifier, char *buf, int len);

void *aros_gui_task_self(void);
int aros_gui_signal_alloc(void);
void aros_gui_signal_free(int bit);
void aros_gui_signal_send(void *task, uint32_t mask);
/* Waits for any of `mask`; `timeout_us` < 0 waits forever. */
uint32_t aros_gui_wait(uint32_t mask, int64_t timeout_us, int *timed_out);
/* Closes the timer behind timed waits, in the task that waited. */
void aros_gui_wait_cleanup(void);

/* --- The clipboard: unit 0, IFF FTXT in ISO-8859-1 (clip.c) --- */

/* Returns 0 on success. */
int aros_gui_clip_write(const uint8_t *text, int32_t len);
/* The clipboard text, or NULL; free it with aros_gui_clip_free(). */
uint8_t *aros_gui_clip_read(int32_t *len);
void aros_gui_clip_free(uint8_t *text);

/* --- OpenGL through gl.library's GLA context (gl.c) --- */

/* A context bound to the window's interior. Returns 0 on failure. */
uintptr_t aros_gui_gl_create(uintptr_t window, int alpha, int depth, int stencil);
/* The same with a profile (core != 0: core profile) and the lowest GL version
   the context must offer (major 0: no minimum). Needs a gl.library with the
   GLA_ContextProfile tags; older ones ignore them (and give compatibility). */
uintptr_t aros_gui_gl_create_profile(uintptr_t window, int alpha, int depth, int stencil,
    int core, int major, int minor);
void aros_gui_gl_destroy(uintptr_t ctx);
/* 0 detaches the current context. */
void aros_gui_gl_make_current(uintptr_t ctx);
uintptr_t aros_gui_gl_current(void);
void aros_gui_gl_swap_buffers(uintptr_t ctx);
/* Points the context at the window's current interior, after a resize. */
void aros_gui_gl_set_window(uintptr_t ctx, uintptr_t window);
void *aros_gui_gl_proc(const char *name);

/* An ASL file requester; flags AROS_GUI_FR_*. Returns the chosen path, to free
   with aros_gui_string_free, or NULL when cancelled. `window` may be 0. */
#define AROS_GUI_FR_SAVE    1
#define AROS_GUI_FR_DRAWERS 2
char *aros_gui_file_request(uintptr_t window, const char *title, const char *drawer,
                            const char *file, const char *pattern, int flags);
/* An EasyRequest with "|"-separated buttons: 1.. from the left, the last is 0. */
int aros_gui_message(uintptr_t window, const char *title, const char *text,
                     const char *buttons);
void aros_gui_string_free(char *text);

#endif /* AROS_GUI_GLUE_H */

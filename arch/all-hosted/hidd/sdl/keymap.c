/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright (c) 2010 The AROS Development Team. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include <exec/types.h>

#include <devices/rawkeycodes.h>

#ifdef __THROW
#undef __THROW
#endif
#ifdef __CONCAT
#undef __CONCAT
#endif

#include "sdl_intern.h"

#define DEBUG 0
#include <aros/debug.h>

struct keymap {
    SDLKey  sdl;
    UBYTE   aros;
};

/* the SDLK_* names are taken from SDL/SDL_keysym.h. I've only included the
 * ones that actually have an equal in AROS. If you have a keyboard with a key
 * that isn't covered here, find its SDL key name in SDL/SDL_keysym.h, choose
 * an appropriate AROS key to map it to from devices/rawkeycodes.h, and add a
 * row to the table. The SDL "checkkeys" test program can help you to choose a
 * SDL key */

static const struct keymap keymap[] = {
    { SDLK_BACKSPACE, RAWKEY_BACKSPACE },
    { SDLK_TAB,       RAWKEY_TAB       },
    { SDLK_RETURN,    RAWKEY_RETURN    },
    { SDLK_PAUSE,     RAWKEY_PAUSE     },
    { SDLK_ESCAPE,    RAWKEY_ESCAPE    },
    { SDLK_SPACE,     RAWKEY_SPACE     },
    { SDLK_COMMA,     RAWKEY_COMMA     },
    { SDLK_MINUS,     RAWKEY_MINUS     },
    { SDLK_PERIOD,    RAWKEY_PERIOD    },
    { SDLK_SLASH,     RAWKEY_SLASH     },
    { SDLK_0,         RAWKEY_0         },
    { SDLK_1,         RAWKEY_1         },
    { SDLK_2,         RAWKEY_2         },
    { SDLK_3,         RAWKEY_3         },
    { SDLK_4,         RAWKEY_4         },
    { SDLK_5,         RAWKEY_5         },
    { SDLK_6,         RAWKEY_6         },
    { SDLK_7,         RAWKEY_7         },
    { SDLK_8,         RAWKEY_8         },
    { SDLK_9,         RAWKEY_9         },
    { SDLK_SEMICOLON, RAWKEY_SEMICOLON },
    { SDLK_EQUALS,    RAWKEY_EQUAL     },
    { SDLK_BACKSLASH, RAWKEY_BACKSLASH },
    { SDLK_BACKQUOTE, RAWKEY_TILDE     },
    { SDLK_a,         RAWKEY_A         },
    { SDLK_b,         RAWKEY_B         },
    { SDLK_c,         RAWKEY_C         },
    { SDLK_d,         RAWKEY_D         },
    { SDLK_e,         RAWKEY_E         },
    { SDLK_f,         RAWKEY_F         },
    { SDLK_g,         RAWKEY_G         },
    { SDLK_h,         RAWKEY_H         },
    { SDLK_i,         RAWKEY_I         },
    { SDLK_j,         RAWKEY_J         },
    { SDLK_k,         RAWKEY_K         },
    { SDLK_l,         RAWKEY_L         },
    { SDLK_m,         RAWKEY_M         },
    { SDLK_n,         RAWKEY_N         },
    { SDLK_o,         RAWKEY_O         },
    { SDLK_p,         RAWKEY_P         },
    { SDLK_q,         RAWKEY_Q         },
    { SDLK_r,         RAWKEY_R         },
    { SDLK_s,         RAWKEY_S         },
    { SDLK_t,         RAWKEY_T         },
    { SDLK_u,         RAWKEY_U         },
    { SDLK_v,         RAWKEY_V         },
    { SDLK_w,         RAWKEY_W         },
    { SDLK_x,         RAWKEY_X         },
    { SDLK_y,         RAWKEY_Y         },
    { SDLK_z,         RAWKEY_Z         },
    { SDLK_DELETE,    RAWKEY_DELETE    },
    { SDLK_KP0,       RAWKEY_KP_0      },
    { SDLK_KP1,       RAWKEY_KP_1      },
    { SDLK_KP2,       RAWKEY_KP_2      },
    { SDLK_KP3,       RAWKEY_KP_3      },
    { SDLK_KP4,       RAWKEY_KP_4      },
    { SDLK_KP5,       RAWKEY_KP_5      },
    { SDLK_KP6,       RAWKEY_KP_6      },
    { SDLK_KP7,       RAWKEY_KP_7      },
    { SDLK_KP8,       RAWKEY_KP_8      },
    { SDLK_KP9,       RAWKEY_KP_9      },
    { SDLK_KP_PERIOD, RAWKEY_KP_DECIMAL},
    { SDLK_KP_PLUS,   RAWKEY_KP_PLUS   },
    { SDLK_KP_ENTER,  RAWKEY_KP_ENTER  },
    { SDLK_UP,        RAWKEY_UP        },
    { SDLK_DOWN,      RAWKEY_DOWN      },
    { SDLK_RIGHT,     RAWKEY_RIGHT     },
    { SDLK_LEFT,      RAWKEY_LEFT      },
    { SDLK_INSERT,    RAWKEY_INSERT    },
    { SDLK_HOME,      RAWKEY_HOME      },
    { SDLK_END,       RAWKEY_END       },
    { SDLK_PAGEUP,    RAWKEY_PAGEUP    },
    { SDLK_PAGEDOWN,  RAWKEY_PAGEDOWN  },
    { SDLK_F1,        RAWKEY_F1        },
    { SDLK_F2,        RAWKEY_F2        },
    { SDLK_F3,        RAWKEY_F3        },
    { SDLK_F4,        RAWKEY_F4        },
    { SDLK_F5,        RAWKEY_F5        },
    { SDLK_F6,        RAWKEY_F6        },
    { SDLK_F7,        RAWKEY_F7        },
    { SDLK_F8,        RAWKEY_F8        },
    { SDLK_F9,        RAWKEY_F9        },
    { SDLK_F10,       RAWKEY_F10       },
    { SDLK_F11,       RAWKEY_F11       },
    { SDLK_F12,       RAWKEY_F12       },
    { SDLK_CAPSLOCK,  RAWKEY_CAPSLOCK  },
    { SDLK_RSHIFT,    RAWKEY_RSHIFT    },
    { SDLK_LSHIFT,    RAWKEY_LSHIFT    },
    { SDLK_RCTRL,     RAWKEY_CONTROL   },
    { SDLK_LCTRL,     RAWKEY_LCONTROL  },
    { SDLK_RALT,      RAWKEY_RALT      },
    { SDLK_LALT,      RAWKEY_LALT      },
    { SDLK_RMETA,     RAWKEY_RAMIGA    },
    { SDLK_LMETA,     RAWKEY_LAMIGA    },
    { SDLK_LSUPER,    RAWKEY_LAMIGA    },
    { SDLK_RSUPER,    RAWKEY_RAMIGA    },
    { SDLK_HELP,      RAWKEY_HELP      },
    { 0xff,           0xff             }
};

void sdl_keymap_init(LIBBASETYPEPTR LIBBASE) {
    int i;
    const struct keymap *pair;

    D(bug("[sdl] sdl_keymap_init\n"));

    for (i = 0; i < SDLK_LAST; i++)
        LIBBASE->keycode[i] = 0xff;

    for (pair = keymap; pair->sdl != 0xff && pair->aros != 0xff; pair++)
        LIBBASE->keycode[pair->sdl] = pair->aros;
}

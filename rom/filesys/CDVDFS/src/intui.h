#ifndef INTUI_H
#define INTUI_H
/* intui.h: */

#include "globals.h"

void Init_Intui(struct CDVDBase *global);
void Close_Intui(struct CDVDBase *global);
void Display_Error_Tags(struct CDVDBase *global, char *, APTR);
#define Display_Error(_p_msg, ...)	\
({					\
    IPTR _tags[] = { __VA_ARGS__ };	\
    Display_Error_Tags(global, _p_msg, (CONST APTR )_tags); \
})
void Show_CDDA_Icon(struct CDVDBase *global);
void Hide_CDDA_Icon(struct CDVDBase *global);

#endif

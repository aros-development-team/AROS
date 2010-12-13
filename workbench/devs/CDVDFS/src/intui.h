#ifndef INTUI_H
#define INTUI_H
/* intui.h: */

void Init_Intui(void);
void Close_Intui(void);
void Display_Error_Tags(char *, APTR);
#define Display_Error(_p_msg, ...)	\
({					\
    IPTR _tags[] = { __VA_ARGS__ };	\
    Display_Error_Tags(_p_msg, (CONST APTR )_tags); \
})
void Show_CDDA_Icon(void);
void Hide_CDDA_Icon(void);

extern struct MsgPort *g_app_port;
extern ULONG g_app_sigbit;
extern int g_retry_show_cdda_icon;

#endif

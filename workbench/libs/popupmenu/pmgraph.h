//
// pmgraph.h
//
// PopupMenu Library - Graphics routines
//
// Copyright (C)2000 Henrik Isaksson <henrik@boing.nu>
// All Rights Reserved.
//

#ifndef PM_GRAPH_H
#define PM_GRAPH_H

struct CGXHook;
struct PM_Root;

void PM_DrawBg(struct PM_Window *pw, int xa, int ya, int xb, int yb);
void PM_Ghost(struct PM_Window *w, int x, int y, int xb, int yb, int pen);
void PM_Shadow(struct PM_Window *w, int x, int y, int xb, int yb, int pen, struct CGXHook *ch);
void ColourBox(struct PM_Window *w, int xa, int ya, int xb, int yb, int pen, int sh, int sd, BOOL selected);
void PM_DI_SetTextPen(struct PM_Window *a, struct PopupMenu *pm);
ULONG PM_RenderCheckMark(struct PM_Window *a, struct PopupMenu *pm, BOOL Selected);
void PM_Separator(struct PM_Window *w, int x, int y, int x2, int shine, int shadow);
void PM_DrawBox(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow);
int PM_NewDrawItem(struct PM_Window *a, struct PopupMenu *pm, BOOL UsePen, BOOL disabled);
int PM_DrawItemHoriz(struct PM_Window *a, struct PopupMenu *pm, BOOL Selected);
void PM_XENSeparator(struct PM_Window *w, int x, int y, int x2, int shine, int shadow, int bgplus, int bgminus);
void PM_NewSeparator(struct PM_Window *w, int x, int y, int x2, int shine, int shadow);
void PM_OldSeparator(struct PM_Window *w, int x, int y, int x2, int shine, int shadow);
void PM_DrawXENBox(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow, int bgplus, int bgminus);
void PM_DrawDBLBox(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow);
void PM_DrawDropBox(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow);
void PM_DrawPrefBox(struct PM_Root *p, struct PM_Window *w, int x1, int y1, int x2, int y2);
void PM_WideSeparator(struct PM_Window *a, struct PopupMenu *pm);
void PM_ShortSeparator(struct PM_Window *a, struct PopupMenu *pm);
void PM_DrawBoxMM2(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow, int halfshine);
void PM_RectFill(struct PM_Window *pw, int xa, int ya, int xb, int yb);

#endif /* PM_GRAPH_H */

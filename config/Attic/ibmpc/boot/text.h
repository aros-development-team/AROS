#ifndef _TEXT_H
#define _TEXT_H

#define KERNEL_DATA ((unsigned char *)0x900)
#define MAXROW 30
extern int c_row, c_col, c_atr;

/* All printing functions to vga text display do auto-wrap and scrolling */

/* Puts one char to vga text display */
void putc(char c);
/* Puts one char to vga text display without changing background information */
void putc_fg(char c);
/* Puts a string to vga text display */
void puts(char *s);
/* Puts a string to vga text display without changing background information */
void puts_fg(char *s);
/* Puts an integer to vga text display */
void puti(unsigned int i);
/* Puts an integer to vga text display without changing background information */
void puti_fg(unsigned int i);
/* Puts a hex number to vga text display */
void putx(unsigned int i);
/* Puts a hex number to vga text display without changing background information */
void putx_fg(unsigned int i);
/* Clear vga text display */
void cls(void);
/* Set Cursor positon to [x,y] ([0,0] is top-left corner) */
/* x>=80 will be (x % 80) and y>=MAXROW will be (y % MAXROW) */
void gotoxy(int x, int y);
/* Set foreground color 0..15 */
void setfg(char c);
/* Set background color 0..7 */
void setbg(char c);
/* Set blink tag 0=off else=on */
void setblink(char c);
/* Calculate length of string not including trailing 0 */
unsigned int strlen(char *s);

#endif /* !_TEXT_H */


#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ep_info.h"
#include "6x10.h"

#if 1 
#define mybug(col,var) __asm__ __volatile__("move.l #"#col",0xf9002000+"#var"" :: );
#else
#define mybug(col,var)
#endif

extern char tab[127];

#ifdef rkprintf
# undef rkprintf
#endif
#define rkprintf(x...)  scr_RawPutChars(tab, snprintf(tab,126, x))

extern unsigned long _end;
extern unsigned long _edata;
extern struct mac68k_init_stuff init_stuff;

void vputc(char c);
void fontinit(void);
void setcursor(unsigned char col, unsigned char row);
void clearscreen(void);
void screen_init(void);
void scr_RawPutChars(char *chr, int lim);

void screen_init(void)
{
#if 0
	init_stuff.vidaddr = 0xf9000e00;
	init_stuff.vidrow = 832;
	init_stuff.vidwidth = 832;
	init_stuff.vidheight = 624;
	init_stuff.viddepth = 8;
#endif

	if(init_stuff.vidaddr)
	{
		unsigned long i,j,k;

		init_stuff.chrrows = init_stuff.vidheight / 10 - 1;
		init_stuff.chrcols = init_stuff.vidwidth / 8 - 1;
		init_stuff.currow = 0;
		init_stuff.curcol = 0;

		clearscreen();

		for(j=0;j<=init_stuff.vidheight;j++)
		{
			k=j*init_stuff.vidrow;
			for(i=0;i<=init_stuff.vidrow;i++)
			{
				*(volatile unsigned char *)(init_stuff.vidaddr+i+k) = (unsigned char)i;
			}
		}
		fontinit();
	}
}

void fontinit(void)
{
	unsigned char i,j,k,m;

	for(j=0;j<128;j++)
	{
		for(i=0;i<10;i++)
		{
			switch(init_stuff.viddepth)
			{
				case 1:
					init_stuff.fontbyte[j][i][0] = ~(Font6x10[j*10+i]);
					break;
				case 2:
				case 4:
				case 8:
				case 16:
				case 24:
				case 32:
				default:
					for(k=0,m=7;k<8;k++,m--)
					{
						if((Font6x10[j*10+i]>>k) & 0x1)
						{
							init_stuff.fontbyte[j][i][m] = 0;
						}
						else
						{
							init_stuff.fontbyte[j][i][m] = 0xff;
						}
					}
					break;
			}
		}
	}
}

void setcursor(unsigned char col, unsigned char row)
{
	init_stuff.curcol = col;
	init_stuff.currow = row;
}

void vputc(char c)
{
	unsigned long caddr;
	unsigned long i;

	if(c == '\n')
	{
		init_stuff.currow++;
		init_stuff.curcol = 0;
	}

	caddr = init_stuff.vidaddr + (init_stuff.curcol*init_stuff.viddepth) + (init_stuff.currow*10*init_stuff.vidrow);

	for(i=0;i<10;i++)
	{
		switch(init_stuff.viddepth)
		{
			case 1:
				*(volatile unsigned char *)(caddr++) = init_stuff.fontbyte[c][i][0];
				caddr += init_stuff.vidrow - 1;
				break;
			case 2:
				*(volatile unsigned short *)(caddr++) = (unsigned short)init_stuff.fontbyte[c][i][0];
				break;
			case 4:
			case 8:
			case 16:
			case 24:
			case 32:
			default:
				memcpy((unsigned long *)(caddr), &(init_stuff.fontbyte[c][i][0]),8);
				caddr += init_stuff.vidrow;
				break;
		}
	}

	if(init_stuff.curcol < init_stuff.chrcols)
	{
		init_stuff.curcol++;
	}
	else
	{
		init_stuff.curcol = 0;
		init_stuff.currow++;
	}
}

void clearscreen(void)
{
	unsigned long i,j,k;

	for(j=0;j<=init_stuff.vidheight;j++)
	{
		k=j*init_stuff.vidrow;
		for(i=0;i<=init_stuff.vidrow;i+=4)
		{
			*(volatile unsigned long *)(init_stuff.vidaddr+i+k) = (unsigned long)0xffffffff;
		}
	}
}

void scr_RawPutChars(char *chr, int lim)
{
    int i;

    for (i=0; i<lim; i++)
        vputc(*chr++);
}

#define mac_scc_cha_b_ctrl_offset	0x0
#define mac_scc_cha_a_ctrl_offset	0x2
#define mac_scc_cha_b_data_offset	0x4
#define mac_scc_cha_a_data_offset	0x6

void sputc(char c)
{
	unsigned char *sccbase = (unsigned char *)0x50f0c020;

	while(0x4 & sccbase[mac_scc_cha_b_ctrl_offset])
	{
		__asm__ __volatile__ ("move.l %%sp@,0xf9003000" :: );
	}

	sccbase[mac_scc_cha_b_data_offset] = c;
}

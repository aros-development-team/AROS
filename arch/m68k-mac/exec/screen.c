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
	unsigned char font_lut2[16] = {
	        0x00, 0x03, 0x0c, 0x0f,
	        0x30, 0x33, 0x3c, 0x3f,
	        0xc0, 0xc3, 0xcc, 0xcf,
	        0xf0, 0xf3, 0xfc, 0xff
	};
	unsigned char font_lut4[4] = {
		0x00, 0x0f, 0xf0, 0xff
	};
	unsigned char i,j,k,m,temp;

	for(j=0;j<128;j++)
	{
		for(i=0;i<10;i++)
		{
			temp = ~(Font6x10[j*10+i]);

			switch(init_stuff.viddepth)
			{
				case 1:
					init_stuff.fontbyte[j][i][0] = temp;
					break;
				case 2:
					init_stuff.fontbyte[j][i][1] = font_lut2[temp&0x0f];
					temp = temp >> 4;
					init_stuff.fontbyte[j][i][0] = font_lut2[temp&0x0f];
					break;
				case 4:
					init_stuff.fontbyte[j][i][3] = font_lut4[temp&0x03];
					temp = temp >> 2;
					init_stuff.fontbyte[j][i][2] = font_lut4[temp&0x03];
					temp = temp >> 2;
					init_stuff.fontbyte[j][i][1] = font_lut4[temp&0x03];
					temp = temp >> 2;
					init_stuff.fontbyte[j][i][0] = font_lut4[temp&0x03];
					break;
				default:
					for(k=0,m=7;k<8;k++,m--)
					{
						if((temp>>k) & 0x1)
						{
							init_stuff.fontbyte[j][i][m] = 0xff;
						}
						else
						{
							init_stuff.fontbyte[j][i][m] = 0x00;
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
	unsigned long caddr,pixel32;
	unsigned long i,j,m;
	unsigned short pixel16;
	unsigned char temp;

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
			case 2:
			case 4:
			case 8:
				memcpy((unsigned long *)(caddr), &(init_stuff.fontbyte[c][i][0]),init_stuff.viddepth);
				break;
			case 16:
				for(m=0;m<=8;m++)
				{
					temp = init_stuff.fontbyte[c][i][m];
					pixel16 = ~((0x01<<15)|(temp<<11)|(temp<<6)|((0xf1&temp)>>3));
					*(volatile unsigned short *)(caddr+m*2) = pixel16;
				}
				break;
			case 24:
			case 32:
				for(m=0;m<=8;m++)
				{
					temp = init_stuff.fontbyte[c][i][m];
					pixel32 = ~(0xff<<24|(temp << 16)|(temp << 8)|temp);
	
					*(volatile unsigned long *)(caddr+m*4) = pixel32;
				}
				break;
			default:
				break;
		}
		caddr += init_stuff.vidrow;
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

#define scc_b_ctrl	0x0
#define scc_a_ctrl	0x2
#define scc_b_data	0x4
#define scc_a_data	0x6

void sputc(char c)
{
	unsigned char *sccbase = (unsigned char *)0x50f0c020;

	while(0x4 & sccbase[scc_b_ctrl]);

	sccbase[scc_b_data] = c;
}

#include "bootinfo.h"
#include "6x10.h"

extern unsigned short _end;

struct vidstuff {
	unsigned long vidaddr;
	unsigned long viddepth;
	unsigned long vidrow;
	unsigned long vidwidth;
	unsigned long vidheight;
	unsigned char chrrows;
	unsigned char chrcols;
	unsigned char curcol;
	unsigned char currow;
	char fontbyte[128][10][8];
};

void vputc(unsigned char c, struct vidstuff *vs);
void fontinit(struct vidstuff *vs);
void setcursor(unsigned char col, unsigned char row, struct vidstuff *vs);
void clearscreen(struct vidstuff *vs);

int main(void)
{
	unsigned char *mytest = (unsigned char*)&_end;
	struct vidstuff vs;

	while(((struct bi_record *)mytest)->tag != BI_LAST)
	{
		struct bi_record *mach_info = (struct bi_record *)mytest;

		switch(mach_info->tag)
		{
			case BI_MAC_VADDR:
				vs.vidaddr = mach_info->data[0];
				break;
			case BI_MAC_VDEPTH:
				vs.viddepth = mach_info->data[0];
				break;
			case BI_MAC_VROW:
				vs.vidrow = mach_info->data[0];
				break;
			case BI_MAC_VDIM:
				vs.vidwidth = mach_info->data[0] & 0xffff;
				vs.vidheight = mach_info->data[0] >> 16;
				break;
			default:
				break;
		}
		mytest += mach_info->size;
	}
	if(vs.vidaddr)
	{
		unsigned long i,j,k;

		vs.chrrows = vs.vidheight / 10 - 1;
		vs.chrcols = vs.vidwidth / 8 - 1;
		vs.currow = 0;
		vs.curcol = 0;

		clearscreen(&vs);

		for(j=0;j<=vs.vidheight;j++)
		{
			k=j*vs.vidrow;
			for(i=0;i<=vs.vidrow;i++)
			{
				*(volatile unsigned char *)(vs.vidaddr+i+k) = (unsigned char)i;
			}
		}
		fontinit(&vs);

		for(j=0;j<128;j++)
		{
			vputc(j,&vs);
		}
		setcursor(10,5,&vs);
		vputc('A',&vs);
		vputc('R',&vs);
		vputc('O',&vs);
		vputc('S',&vs);
	}
}

void fontinit(struct vidstuff *vs)
{
	unsigned char i,j,k,m;

	for(j=0;j<128;j++)
	{
		for(i=0;i<10;i++)
		{
			for(k=0,m=7;k<8;k++,m--)
			{
				if((Font6x10[j*10+i]>>k) & 0x1)
				{
					vs->fontbyte[j][i][m] = 0;
				}
				else
				{
					vs->fontbyte[j][i][m] = 0xff;
				}
			}
		}
	}
}

void setcursor(unsigned char col, unsigned char row, struct vidstuff *vs)
{
	vs->curcol = col;
	vs->currow = row;
}

void vputc(unsigned char c, struct vidstuff *vs)
{
	unsigned long bytesperpixel = vs->vidrow / vs->vidwidth; 
	unsigned long caddr;
	unsigned long i,k;

	if(c == '\n')
	{
		vs->currow++;
		vs->curcol = 0;
	}

	caddr = vs->vidaddr + (vs->curcol*8) + (vs->currow*10*vs->vidrow);

	for(i=0;i<10;i++)
	{
		for(k=0;k<8;k++)
		{
			 *(volatile unsigned char *)(caddr++) = vs->fontbyte[c][i][k];
		}
		caddr += vs->vidrow - 8;
	}

	if(vs->curcol < vs->chrcols)
	{
		vs->curcol++;
	}
	else
	{
		vs->curcol = 0;
		vs->currow++;
	}
}

void clearscreen(struct vidstuff *vs)
{
	unsigned long i,j,k;

	for(j=0;j<=vs->vidheight;j++)
	{
		k=j*vs->vidrow;
		for(i=0;i<=vs->vidrow;i++)
		{
			*(volatile unsigned char *)(vs->vidaddr+i+k) = (unsigned char)0xff;
		}
	}
}

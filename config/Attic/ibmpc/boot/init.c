/*
    (C) 1997-98 AROS - The Amiga Research OS
    $Id$
    
    Desc: Begining of AROS kernel
    Lang: English
*/

/*****************************************************************************

    FUNCTION
	This is the main file in Native PC AROS. The main function is called
	by head.S file. All this code is in flat 32-bit mode.
	
	This file will make up the exec base, memory and other stuff. We don't
	need any malloc functions, because there is no OS yet. To be honest:
	while main is running Native PC AROS is the only OS on our PC.

	Be careful. You cant use any stdio. Only your own functions are
	acceptable at this moment!!

*****************************************************************************/

#include "text.h"
#include "logo.h"


void show_status(void)
{
unsigned char *p;
int d,i;

  p = KERNEL_DATA;
  puts_fg("\nAROS detected Hardware\nProcessortype: 80");
  d = p[0]-1;
  puti_fg(d);
  puts_fg("86\nAvailable Memory: ");
  d = (p[3]<<8) + p[2];
  puti_fg(d);
  puts_fg("kB\n");
  puts_fg("Video: (");
  d = p[21];
  puti_fg(d);
  puts_fg("x");
  d = p[22];
  puti_fg(d);
  puts_fg(")\n");
  puts_fg("Pointing device: ");
  if(p[0x1ff] == 0xaa)
    puts_fg("installed\n");
  else
    puts_fg("not installed\n");
  puts_fg("HD Drive tables:\nhd0(0x80): 0x");
  for(i=0;i<0x10;i++)
  {
    d = p[0x80+i];
    if( d < 16 )
      putc_fg('0');
    putx_fg(d);
  }
  puts_fg("\nhd1(0x90): 0x");
  for(i=0;i<0x10;i++)
  {
    d = p[0x90+i];
    if( d < 16 )
      putc_fg('0');
    putx_fg(d);
  }
  putc_fg('\n');

}

int main()
{
  char text[] = "Now booting AROS - The Amiga Research OS\n";

  showlogo();
  gotoxy(0,0);
  puts_fg(text);
  show_status();

return 0;
}

#define C_BLINK 0x80
#define C_BGCOLOR 0x70
#define C_FGCOLOR 0x0f
#define VGA_BASE (void *)0xb8000
#define KERNEL_DATA ((unsigned char *)0x900)
#define MAXROW 30

int c_row = 0, c_col = 0, c_atr = 0x0f;
void Set_cursor(int pos);

void putc_fg(char c)
{
unsigned char *p;

  if(c != '\n')
  {
    p = VGA_BASE + 80*2*c_row + c_col*2;
    p[0] = c;
    p[1] &= C_BGCOLOR;
    p[1] |= (c_atr & (C_FGCOLOR|C_BLINK));
    c_col++;
  }
  else
  {
    c_col = 0;
    c_row++;
  }
  if(c_col == 80)
  {
    c_row++;
    c_col = 0;
  }
  if(c_row == MAXROW)
  {
  int i,j;
    for(j=0;j<(MAXROW-1);j++)
    {
      p = VGA_BASE + 80*2*j;
      for(i=0;i<80;i++)
      {
        p[i*2] = p[i*2+160];
        p[i*2+1] &= C_BGCOLOR;
        p[i*2+1] |= (p[i*2+161] & (C_FGCOLOR|C_BLINK));
      }
    }
    p = VGA_BASE + 80*2*(MAXROW-1);
    for(i=0;i<80;i++)
      p[i*2] &= C_BGCOLOR;
    c_col = 0;
    c_row--;
  }
  Set_cursor(c_col+c_row*80);
}

void putc(char c)
{
unsigned char *p;

  if(c != '\n')
  {
    p = VGA_BASE + 80*2*c_row + c_col*2;
    p[0] = c;
    p[1] = c_atr;
    c_col++;
  }
  else
  {
    c_col = 0;
    c_row++;
  }
  if(c_col == 80)
  {
    c_row++;
    c_col = 0;
  }
  if(c_row == MAXROW)
  {
  int i,j;
    for(j=0;j<(MAXROW-1);j++)
    {
      p = VGA_BASE + 80*2*j;
      for(i=0;i<160;i++)
        p[i] = p[i+160];
    }
    p = VGA_BASE + 80*2*(MAXROW-1);
    for(i=0;i<160;i++)
      p[i] = 0;
    c_col = 0;
    c_row--;
  }
  Set_cursor(c_col+c_row*80);
}

void puts_fg(char *s)
{
  while(*s)putc_fg(*s++);
}

void puts(char *s)
{
  while(*s)putc(*s++);
}

void _puti_fg(unsigned int i)
{
  if(i!=0)
  {
    _puti_fg((int)(i/10));
    putc_fg('0'+(int)(i%10));
  }
}

void puti_fg(unsigned int i)
{
  if(i!=0)
    _puti_fg((int)(i/10));
  putc_fg('0'+(int)(i%10));
}

void _puti(unsigned int i)
{
  if(i!=0)
  {
    _puti(i/10);
    putc('0'+i%10);
  }
}

void puti(unsigned int i)
{
  if(i!=0)
    _puti(i/10);
  putc('0'+i%10);
}

void _putx_fg(unsigned int i)
{
unsigned char v;
  if(i!=0)
  {
    _putx_fg(i/16);
    v = i%16;
    if(v<10)
      putc_fg('0'+v);
    else
      putc_fg('a'+v-10);
  }
}

void putx_fg(unsigned int i)
{
unsigned char v;
  if(i!=0)
    _putx_fg(i/16);
  v = i%16;
  if(v<10)
    putc_fg('0'+v);
  else
    putc_fg('a'+v-10);
}

void _putx(unsigned int i)
{
unsigned char v;
  if(i!=0)
  {
    _putx(i/16);
    v = i%16;
    if(v<10)
      putc('0'+v);
    else
      putc('a'+v-10);
  }
}

void putx(unsigned int i)
{
unsigned char v;
  if(i!=0)
    _putx(i/16);
  v = i%16;
  if(v<10)
    putc('0'+v);
  else
    putc('a'+v-10);
}

void cls(void)
{
unsigned char *p;
int i,j;

  p = VGA_BASE;
  for(j=0;j<MAXROW;j++)
    for(i=0;i<80;i++)
      p[2*i+160*j] = ' ';
      p[2*i+160*j+1] = c_atr;

  c_row = 0;
  c_col = 0;
  Set_cursor(c_col+c_row*80);
}

void gotoxy(int x, int y)
{
  c_col = x%80;
  c_row = y%MAXROW;
  Set_cursor(c_col+c_row*80);
}

void setfg(char c)
{
  c_atr &= C_BLINK | C_BGCOLOR;
  c_atr |= (c & C_FGCOLOR);
}

void setbg(char c)
{
  c_atr &= C_BLINK | C_FGCOLOR;
  c_atr |= (c & 0x07)<<4;
}

void setblink(char c)
{
  c_atr &= C_FGCOLOR | C_BGCOLOR;
  if(c)
    c_atr |= C_BLINK;
}

unsigned int strlen(char *s)
{
unsigned int i=0;
  while(s[i++]);
  --i;
return i;
}

from)
{
  DTFTYPE to;
  unsigned char *fp=((unsigned char *)&from)+7,*tp=(unsigned char *)&to;
  *tp++=*fp--;
  *tp++=*fp--;
  *tp++=*fp--;
  *tp++=*fp--;
  *tp++=*fp--;
  *tp++=*fp--;
  *tp++=*fp--;
  *tp++=*fp--;
  return to;
}

  	


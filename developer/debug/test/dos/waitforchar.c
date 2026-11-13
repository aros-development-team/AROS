/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>

int main()
{
  TEXT ch;
  BPTR in = Input();
  BPTR out = Output();

  Printf("Enter a character within 9 seconds:\n");

  SetMode(in, 1);

  if (WaitForChar(in, 9000000))
  {
    FPuts(out, "WaitForChar: char arrived\n");
    Read(in, &ch, 1); /* Flush the character */
  }
  else
    FPuts(out, "WaitForChar: timeout\n");

  SetMode(in, 0);

  return 0;
}

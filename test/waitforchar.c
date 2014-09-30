/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>

int main()
{
  BPTR in = Input();
  BPTR out = Output();

  SetMode(in, 1);

  Delay(25);

  if (WaitForChar(in, 9000000))
    FPuts(out, "WaitForChar: char arrived\n");
  else
    FPuts(out, "WaitForChar: timeout\n");

  Flush(in);
  Flush(out);

  SetMode(in, 0);

  return 0;
}

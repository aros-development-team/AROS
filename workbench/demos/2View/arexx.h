#ifndef _AREXX_H
#define _AREXX_H

#include "minrexx.h"

long initRexxPort(void);
void determinePortName(char *portName);
int disp(struct RexxMsg *msg,struct rexxCommandList *dat,char *p);

void setRexxFree(void);

#if 0
#pragma libcall RexxSysBase CreateArgstring 7E 0802
#pragma libcall RexxSysBase DeleteArgstring 84 801
#pragma libcall RexxSysBase LengthArgstring 8A 801
#pragma libcall RexxSysBase CreateRexxMsg 90 09803
#pragma libcall RexxSysBase DeleteRexxMsg 96 801
#pragma libcall RexxSysBase ClearRexxMsg 9C 0802
#pragma libcall RexxSysBase FillRexxMsg A2 10803
#pragma libcall RexxSysBase IsRexxMsg A8 801

#pragma libcall RexxSysBase LockRexxBase 1C2 001
#pragma libcall RexxSysBase UnlockRexxBase 1C8 001
#endif


#endif /* _AREXX_H */

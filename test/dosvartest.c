#include <string.h>
#include <stdio.h>

#include "proto/dos.h"
#include "proto/exec.h"
#include "proto/utility.h"



#define DefCheckVar(VarName, Flags,Expected) \
  CopyMem("!!!", Buffer, 4); \
  printf("Checking Variable %s with Flags %x \n",VarName,Flags); \
  Bytes = GetVar(VarName,Buffer,0x17,Flags); \
  printf("  GetVar: res=0x%x  %s  (IOErr=%i, len=%i)\n", Bytes, Buffer,IoErr(),strlen(Buffer)); \
  printf("expected: %s\n",Expected); \
  printf("---\n");

#define DefSetVar(VarName,Value,Flags) \
  printf("Setting Variable %s:=`%s` with Flags %x \n",VarName,Value,Flags); \
  SetVar(VarName,Value,-1,Flags);

#define DefDeleteVar(VarName, Flags) \
  printf("Deleting Variable %s  Flags %x\n",VarName,Flags); \
  DeleteVar(VarName, Flags); \
  printf("---\n");

void main(void)
{
  int Bytes;
  char * Buffer = AllocMem(0x1000,0x0);

  DefCheckVar("s:startup-sequence", GVF_GLOBAL_ONLY, "first line of startup-sequence");
  DefCheckVar("s:startup-sequence", GVF_GLOBAL_ONLY|GVF_BINARY_VAR, "whole startup-sequence");

  DefSetVar("AROS","Amiga R. OS\n1995-1997\n",GVF_LOCAL_ONLY);
  DefCheckVar("AROS", GVF_LOCAL_ONLY, "Amiga R. OS");
  DefCheckVar("AROS", GVF_LOCAL_ONLY|GVF_BINARY_VAR, "Amiga R. OS\n1995-1997\n");
  DefCheckVar("AROS", GVF_LOCAL_ONLY|GVF_BINARY_VAR|GVF_DONT_NULL_TERM, "Amiga R. OS\n1995-1997\n(+garbage)\n");
  DefDeleteVar("AROS", GVF_LOCAL_ONLY);
  DefCheckVar("AROS", GVF_LOCAL_ONLY, "no output as var is deleted");

  DefSetVar("AROS","Amiga R. OS\n1995-1997\n",GVF_LOCAL_ONLY|LVF_IGNORE);
  DefCheckVar("aros", GVF_LOCAL_ONLY, "no output");
  DefCheckVar("aros", GVF_LOCAL_ONLY|LVF_IGNORE, "Amiga R. OS");
  DefCheckVar("aRoS", GVF_LOCAL_ONLY|LVF_IGNORE|GVF_BINARY_VAR, "Amiga R. OS\n1995-1997\n");
  DefCheckVar("aRoS", GVF_LOCAL_ONLY|LVF_IGNORE|GVF_BINARY_VAR|GVF_DONT_NULL_TERM, "Amiga R. OS\n1995-1997\n (+garbage)\n");
  DefDeleteVar("AROS", GVF_LOCAL_ONLY);
  DefCheckVar("aros", GVF_LOCAL_ONLY|LVF_IGNORE, "Amiga R. OS");
  DefDeleteVar("AROS", GVF_LOCAL_ONLY|LVF_IGNORE);
  DefCheckVar("aros", GVF_LOCAL_ONLY|LVF_IGNORE, "no output as var is deleted");

  DefSetVar("AROS","Amiga R. OS\n1995-1997\n",GVF_LOCAL_ONLY|LVF_IGNORE|LV_ALIAS);
  DefCheckVar("aros", GVF_LOCAL_ONLY, "no output");
  DefCheckVar("aros", GVF_LOCAL_ONLY|LVF_IGNORE, "no output");
  DefCheckVar("aros", GVF_LOCAL_ONLY|LV_ALIAS, "no output");
  DefCheckVar("aros", GVF_LOCAL_ONLY|LVF_IGNORE|LV_ALIAS, "Amiga R. OS");
  DefCheckVar("aRoS", GVF_LOCAL_ONLY|LVF_IGNORE|GVF_BINARY_VAR|LV_ALIAS, "Amiga R. OS\n1995-1997\n");
  DefDeleteVar("AROS", GVF_LOCAL_ONLY);
  DefCheckVar("aros", GVF_LOCAL_ONLY|LVF_IGNORE|LV_ALIAS, "Amiga R. OS");
  DefDeleteVar("AROS", GVF_LOCAL_ONLY|LVF_IGNORE);
  DefCheckVar("aros", GVF_LOCAL_ONLY|LVF_IGNORE|LV_ALIAS, "Amiga R. OS");
  DefDeleteVar("AROS", GVF_LOCAL_ONLY|LVF_IGNORE);
  DefCheckVar("aros", GVF_LOCAL_ONLY|LVF_IGNORE|LV_ALIAS, "Amiga R. OS");
  DefDeleteVar("AROS", GVF_LOCAL_ONLY|LVF_IGNORE|LV_ALIAS);
  DefCheckVar("aros", GVF_LOCAL_ONLY|LVF_IGNORE|LV_ALIAS, "no output as var is deleted");


  DefSetVar("AROS","Amiga R. OS\n1995-1997\n",GVF_GLOBAL_ONLY);
  DefCheckVar("aros", GVF_LOCAL_ONLY, "no output");
  DefCheckVar("aros", GVF_GLOBAL_ONLY, "Amiga R. OS");
  DefDeleteVar("AROS", GVF_LOCAL_ONLY);
  DefCheckVar("aros", GVF_GLOBAL_ONLY, "Amiga R. OS");
  DefDeleteVar("aROS", GVF_GLOBAL_ONLY);
  DefCheckVar("AROS", GVF_GLOBAL_ONLY, "no output as var is deleted");

  DefSetVar("AROS","Amiga R. OS\n1995-1997\n",GVF_GLOBAL_ONLY|GVF_SAVE_VAR);
  DefCheckVar("aros", GVF_LOCAL_ONLY, "no output");
  DefCheckVar("aros_", GVF_GLOBAL_ONLY, "no output");
  DefCheckVar("aros", GVF_GLOBAL_ONLY, "Amiga R. OS");
  DefCheckVar("aRoS", GVF_GLOBAL_ONLY|GVF_BINARY_VAR, "Amiga R. OS\n1995-1997\n");
  DefCheckVar("aRoS", GVF_GLOBAL_ONLY|GVF_BINARY_VAR|GVF_DONT_NULL_TERM, "Amiga R. OS\n1995-1997\n");
  DefDeleteVar("AROS", GVF_LOCAL_ONLY);
  DefCheckVar("aros", GVF_GLOBAL_ONLY, "Amiga R. OS");
  DefDeleteVar("AROS", GVF_GLOBAL_ONLY);
  DefCheckVar("aros", GVF_GLOBAL_ONLY, "no output as var is deleted");


  FreeMem(Buffer,0x1000);
}

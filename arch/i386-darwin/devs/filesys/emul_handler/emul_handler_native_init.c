/*
 *  emul_handler_native_init.c.c
 *  AROS
 *
 *  Created by Daniel Oberhoff on 06.04.08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#define NATIVE
#include <aros/kernel.h>
#include "emul_handler_intern.h"

struct TagItem * emul_handler_native_hooks = 0;

struct TagItem * emul_handler_get_native_hooks()
{
  if (emul_handler_native_hooks == 0)
  {
	BeginHookList(emul_handler_native_hooks,28);
	Add2HookList(EHND,Emul,Stat);
	Add2HookList(EHND,Emul,LStat);
	Add2HookList(EHND,Emul,CloseDir);
	Add2HookList(EHND,Emul,Close);
	Add2HookList(EHND,Emul,OpenDir);
	Add2HookList(EHND,Emul,Open);
	Add2HookList(EHND,Emul,DirName);
	Add2HookList(EHND,Emul,TellDir);
	Add2HookList(EHND,Emul,SeekDir);
	Add2HookList(EHND,Emul,RewindDir);
	Add2HookList(EHND,Emul,Delete);
	Add2HookList(EHND,Emul,Rename);
	Add2HookList(EHND,Emul,GetEnv);
	Add2HookList(EHND,Emul,GetCWD);
	Add2HookList(EHND,Emul,GetHome);
	Add2HookList(EHND,Emul,ClosePW);
	Add2HookList(EHND,Emul,SymLink);
	Add2HookList(EHND,Emul,StatFS);
	Add2HookList(EHND,Emul,ChDir);
	Add2HookList(EHND,Emul,Isatty);
	Add2HookList(EHND,Emul,Link);
	Add2HookList(EHND,Emul,LSeek);
	Add2HookList(EHND,Emul,Chmod);
	Add2HookList(EHND,Emul,SymLink);
	Add2HookList(EHND,Emul,MKDir);
	Add2HookList(EHND,Emul,Read);
	Add2HookList(EHND,Emul,ReadLink);
	Add2HookList(EHND,Emul,Write);
	EndHookList;
  }
  return emul_handler_native_hooks;
}

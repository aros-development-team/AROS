/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: Test program for RealTime.library
    Lang: English
*/


#include <aros/debug.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include <libraries/realtime.h>
#include <proto/realtime.h>

#include <stdio.h>
#include <stdlib.h>


AROS_UFP3(ULONG, myFunc,
	  AROS_UFPA(struct Hook *  , hook   , A0),
	  AROS_UFPA(struct pmTime *, message, A1),
	  AROS_UFPA(struct Player *, player , A2));


int main(int argc, char* argv[])
{
    struct Library *RealTimeBase = OpenLibrary("realtime.library", 40);
    struct Hook myHook;

    struct TagItem tags[] = { { PLAYER_Name     , (IPTR)"Test player" },
			      { PLAYER_Hook     , (IPTR)&myHook },
			      { PLAYER_Conductor, (IPTR)"Test conductor" },
			      { TAG_DONE        , (IPTR)NULL } };

    struct Player *player;

    myHook.h_Entry = (HOOKFUNC)myFunc;
    myHook.h_SubEntry = NULL;
    myHook.h_Data  = NULL;

    if (RealTimeBase == NULL)
    {
	printf("Couldn't open realtime.library\n");
	exit(1);
    }

    player = CreatePlayerA(tags);

    if (player == NULL)
    {
	printf("Couldn't create player\n");
	CloseLibrary(RealTimeBase);
	exit(1);
    }

    SetConductorState(player, CONDSTATE_RUNNING, 0);

    {
	struct TagItem tags[] = { { PLAYER_Ready, TRUE },
				  { TAG_DONE,     NULL } };

	SetPlayerAttrsA(player, tags);
    }

    Wait(SIGBREAKF_CTRL_C);

    DeletePlayer(player);

    CloseLibrary(RealTimeBase);

    return 0;
}


AROS_UFH3(ULONG, myFunc,
	  AROS_UFHA(struct Hook *  , hook   , A0),
	  AROS_UFHA(struct pmTime *, message, A1),
	  AROS_UFHA(struct Player *, player , A2))
{
    AROS_USERFUNC_INIT

    switch (message->pmt_Method)
    {
    case PM_TICK:
	kprintf("Tick at clock %u\n", message->pmt_Time);
	break;

    case PM_POSITION:
	kprintf("Position change: Clock %u\n", message->pmt_Time);
	break;

    case PM_SHUTTLE:
	kprintf("Shuttling into clock %u\n", message->pmt_Time);
	break;

    case PM_STATE:
	kprintf("State change... old state = %u\n",
		((struct pmState *)message)->pms_OldState);

    default:
	kprintf("Error: Bogus message with method %u\n", message->pmt_Method);
	break;
    }

    return 0;

    AROS_USERFUNC_EXIT
}

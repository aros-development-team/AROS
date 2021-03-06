/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.
*/
# define  DEBUG 0
# include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/realtime.h>
#include <exec/lists.h>
#include "realtime_intern.h"

struct Conductor *createConductor(BOOL private, LONG *error,STRPTR name,
                                  struct Library *RealTimeBase);

/*****************************************************************************

    NAME */

#include <utility/tagitem.h>
#include <libraries/realtime.h>

    AROS_LH2(BOOL, SetPlayerAttrsA,

/*  SYNOPSIS */

        AROS_LHA(struct Player  *, player , A0),
        AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */

        struct Library *, RealTimeBase, 9, RealTime)

/*  FUNCTION

    Sets the attributes of a player. An attribute not specified in the array
    of tags is unchanged after this procedure.

    INPUTS

    player   --  The player the attributes of which to set.
    tagList  --  Pointer to an array of tags describing the player's
                 attributes or NULL.

    TAGS

    The same tags as for CreatePlayerA().

    RESULT

    Success/failure indicator. If failure, then, in case the PLAYER_ErrorCode
    is provided, more information can be obtained via that pointer.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    DeletePlayer(), GetPlayerAttrsA()

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    LONG           *error = NULL;
    struct TagItem *tl = tagList;
    struct TagItem *tag;
    APTR            lock;

    error = (LONG *)GetTagData(PLAYER_ErrorCode, (IPTR)NULL, tl);

    while ((tag = NextTagItem(&tl)) != NULL)
    {
        switch(tag->ti_Tag)
        {
        case PLAYER_Name:
            player->pl_Link.ln_Name = (APTR)tag->ti_Data;
            break;

        case PLAYER_Hook:
            player->pl_Hook = (struct Hook *)tag->ti_Data;
            break;

        case PLAYER_Priority:
            player->pl_Link.ln_Pri = (BYTE)tag->ti_Data;

            if (player->pl_Link.ln_Succ != NULL)
            {
                /* If this node has been (is) inserted before, then remove it
                   and put it in the right place. */

                /* Is this player attached to a conductor? */
                if (player->pl_Source != NULL)
                {
                    lock = LockRealTime(RT_CONDUCTORS);
                    Remove((struct Node *)player);
                    Enqueue((struct List *)&player->pl_Source->cdt_Players,
                            (struct Node *)player);
                    UnlockRealTime(lock);
                }
                else
                {
                    if(error != NULL)
                    {
                        *error = RTE_NOCONDUCTOR;
                    }

                    return FALSE;
                }
            }

            break;

        case PLAYER_Conductor:

            D(bug("Found PLAYER_Conductor tag\n"));

            if ((IPTR)tag->ti_Data == -1UL)
            {
                player->pl_Source = createConductor(TRUE, error,
                                                    (STRPTR)tag->ti_Data,
                                                    RealTimeBase);
            }
            else
            {
                struct Conductor *cd = FindConductor((STRPTR)tag->ti_Data);

                if (cd == NULL)
                {
                    D(bug("Trying to create a public conductor.\n"));
                    player->pl_Source = createConductor(FALSE, error,
                                                        (STRPTR)tag->ti_Data,
                                                        RealTimeBase);
                }
                else
                {
                    player->pl_Source = cd;
                }
            }

            if (player->pl_Source != NULL)
            {
                APTR lock;

                lock = LockRealTime(RT_CONDUCTORS);
                
                /* Enqueue the player to the conductor list */
                Enqueue((struct List *)&player->pl_Source->cdt_Players,
                        (struct Node *)player);
                
                UnlockRealTime(lock);
            }
            else
            {
                if (error != NULL)
                {
                    *error = RTE_NOCONDUCTOR;
                    return FALSE;
                }
            }
            break;

        case PLAYER_Ready:
            if ((BOOL)tag->ti_Data)
            {
                struct Conductor *conductor = player->pl_Source;

                player->pl_Flags |= PLAYERF_READY;

                if (conductor != NULL)
                {
                    ObtainSemaphoreShared(&conductor->cdt_Lock);

                    if (conductor->cdt_Barrier != NULL)
                    {
                        Signal(conductor->cdt_Barrier, SIGF_SINGLE);
                    }

                    ReleaseSemaphore(&conductor->cdt_Lock);
                }
            }
            else
            {
                player->pl_Flags &= ~PLAYERF_READY;
            }
                
            break;

        case PLAYER_AlarmTime:
            player->pl_Flags |= PLAYERF_ALARMSET;
            player->pl_AlarmTime = (LONG)tag->ti_Data;
            break;

        case PLAYER_Alarm:
            if ((BOOL)tag->ti_Data)
            {
                player->pl_Flags |= PLAYERF_ALARMSET;
            }
            else
            {
                player->pl_Flags &= ~PLAYERF_ALARMSET;
            }

            break;

        case PLAYER_AlarmSigTask:
            if ((struct Task *)tag->ti_Data == NULL)
            {
                player->pl_Flags &= ~PLAYERF_ALARMSET;
            }
                
            player->pl_Task = (struct Task *)tag->ti_Data;
            break;

        case PLAYER_AlarmSigBit:
            if ((BYTE)tag->ti_Data == -1)
            {
                player->pl_Flags &= ~PLAYERF_ALARMSET;
            }

            /* We could use player->pl_Link.ln_Type here */
            player->pl_Reserved0 = (BYTE)tag->ti_Data;      /* NOTE! */
            break;

        case PLAYER_Quiet:
            if ((BOOL)tag->ti_Data)
            {
                player->pl_Flags |= PLAYERF_QUIET;
            }
            else
            {
                player->pl_Flags &= ~PLAYERF_QUIET;
            }
                
            break;

        case PLAYER_UserData:
            player->pl_UserData = (APTR)tag->ti_Data;
            break;

        case PLAYER_ID:
            player->pl_PlayerID = (UWORD)tag->ti_Data;
            break;

        case PLAYER_Conducted:
            if ((BOOL)tag->ti_Data)
            {
                player->pl_Flags |= PLAYERF_CONDUCTED;
            }
            else
            {
                player->pl_Flags &= ~PLAYERF_CONDUCTED;
            }

            break;

        case PLAYER_ExtSync:
            lock = LockRealTime(RT_CONDUCTORS);

            if ((BOOL)tag->ti_Data)
            {
                if (player->pl_Source->cdt_Flags & CONDUCTF_EXTERNAL)
                {
                    /* Only one external synchronizer at a time, please */
                    UnlockRealTime(lock);

                    return FALSE;
                }

                player->pl_Source->cdt_Flags |= CONDUCTF_EXTERNAL;
                player->pl_Flags |= PLAYERF_EXTSYNC;
            }
            else
            {
                /* If this player was the external synchronizer, we
                   clean up */
                if (player->pl_Flags & PLAYERF_EXTSYNC)
                {
                    player->pl_Source->cdt_Flags &= ~CONDUCTF_EXTERNAL;
                    player->pl_Source->cdt_Flags &= ~CONDUCTF_GOTTICK;
                }

                player->pl_Flags &= ~PLAYERF_EXTSYNC;
            }

            UnlockRealTime(lock);

            break;
        }
    }

    /* Consistency checks */
    if (player->pl_Task == NULL)
    {
        player->pl_Flags &= ~PLAYERF_ALARMSET;
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* SetPlayerAttrsA */

struct Conductor *createConductor(BOOL private, LONG *error, STRPTR name,
                                  struct Library *RealTimeBase)
{
    struct Conductor *cd = AllocMem(sizeof(struct Conductor),
                                    MEMF_PUBLIC | MEMF_CLEAR);

    if (cd == NULL)
    {
        if (error != NULL)
        {
            *error = RTE_NOMEMORY;
        }

        return NULL;
    }

    cd->cdt_Link.ln_Name = name;

    NEWLIST(&cd->cdt_Players);
    InitSemaphore(&cd->cdt_Lock);

    /* Initialize conductor clock */
    cd->cdt_ClockTime = GPB(RealTimeBase)->rtb_Time;
    cd->cdt_StartTime = GPB(RealTimeBase)->rtb_Time;

    /* Conductors are created in 'stopped' mode. To make the clock start
       running, call SetConductorState(player, CONDSTATE_RUNNING, _); */
    cd->cdt_State = CONDSTATE_STOPPED;

    if (private)
    {
        cd->cdt_Flags |= CONDUCTF_PRIVATE;
    }

    {
        /* Add the conductor to the realtime library conductor list */
        APTR lock;

        lock = LockRealTime(RT_CONDUCTORS);
        
        AddTail((struct List *)&GPB(RealTimeBase)->rtb_ConductorList,
                (struct Node *)cd);
        
        UnlockRealTime(lock);
    }

    return cd;
}

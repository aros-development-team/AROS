/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: The boot assign DRIVERS: must own its lock.
*/

/*
 * An assign unlocks its locks when it is removed or replaced (AssignLock()),
 * so a lock that DRIVERS: shares with another assign (it used to be the
 * second lock of LIBS:) is freed by "Assign LIBS: ..." while DRIVERS: still
 * uses it. This looks for DRIVERS:'s lock in every other directory assign,
 * main lock and multi-assign list, and changes nothing.
 *
 * Only pointer identity is checked, and only for DRIVERS:: ram-handler
 * shares one counted lock per object, so two RAM: assigns may legally hold
 * the same pointer.
 *
 * Returns 0 if no other assign holds DRIVERS:'s lock, RETURN_FAIL if one
 * does, RETURN_WARN if there is no DRIVERS: directory assign.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dosextens.h>

int main(void)
{
    struct DosList *head, *dl, *self;
    BPTR drivers = BNULL;
    LONG checked = 0, shared = 0;

    head = LockDosList(LDF_ASSIGNS | LDF_READ);

    self = FindDosEntry(head, "DRIVERS", LDF_ASSIGNS);
    if (self != NULL && self->dol_Type == DLT_DIRECTORY)
        drivers = self->dol_Lock;

    if (drivers != BNULL)
    {
        dl = head;
        while ((dl = NextDosEntry(dl, LDF_ASSIGNS)) != NULL)
        {
            struct AssignList *al;

            if (dl == self || dl->dol_Type != DLT_DIRECTORY)
                continue;

            checked++;
            if (dl->dol_Lock == drivers)
            {
                Printf("DRIVERS: lock %p is also held by %b:\n", BADDR(drivers),
                       dl->dol_Name);
                shared++;
            }
            for (al = dl->dol_misc.dol_assign.dol_List; al; al = al->al_Next)
            {
                checked++;
                if (al->al_Lock == drivers)
                {
                    Printf("DRIVERS: lock %p is also held by %b:\n", BADDR(drivers),
                           dl->dol_Name);
                    shared++;
                }
            }
        }
    }

    UnLockDosList(LDF_ASSIGNS | LDF_READ);

    if (drivers == BNULL)
    {
        PutStr("no DRIVERS: directory assign\n");
        return RETURN_WARN;
    }

    Printf("%ld locks of other assigns checked, %ld are DRIVERS:'s\n", checked, shared);

    return shared ? RETURN_FAIL : RETURN_OK;
}

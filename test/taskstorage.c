#include <proto/exec.h>
#include <proto/dos.h>

#include <assert.h>

static BPTR out;
static LONG slot1, slot2, slot3;

static void printslot1(void)
{
    FPrintf(out, "Value slot %ld: %ld\n", slot1, GetTaskStorageSlot(slot1));
}

int main(void)
{
    out = Output();

    slot1 = AllocTaskStorageSlot();
    FPrintf(out, "Got slot %ld\n", slot1);

    slot2 = AllocTaskStorageSlot();
    FPrintf(out, "Got slot %ld\n", slot2);

    FreeTaskStorageSlot(slot2);
    FPrintf(out, "Freed slot %ld\n", slot2);

    slot2 = AllocTaskStorageSlot();
    FPrintf(out, "Got slot %ld\n", slot2);

    slot3 = AllocTaskStorageSlot();
    FPrintf(out, "Got slot %ld\n", slot3);

    SetTaskStorageSlot(slot1, 69);
    FPrintf(out, "Stored value 69 in slot %ld\n", slot1);

    FPrintf(out, "Checking value in subtask\n");
    struct Process *proc = 
        CreateNewProcTags(
            NP_Entry, printslot1, NP_Name, "Check slot1",
            NP_Synchronous, TRUE, TAG_DONE
        );
    assert(proc != NULL);

    FreeTaskStorageSlot(slot1);
    FPrintf(out, "Freed slot %ld\n", slot1);

    slot1 = AllocTaskStorageSlot();
    FPrintf(out, "Got slot %ld\n", slot1);

    FreeTaskStorageSlot(slot2);
    FPrintf(out, "Freed slot %ld\n", slot2);

    FreeTaskStorageSlot(slot1);
    FPrintf(out, "Freed slot %ld\n", slot1);

    FreeTaskStorageSlot(slot3);
    FPrintf(out, "Freed slot %ld\n", slot3);

    return 0;
}


There are at least 3 different AOS SetPatch programs we need to
fake out:

WB 1.3:

   - Looks for a message port named "SetPatch-01"
   struct SetPatch_1 {
       struct MsgPort sp_MsgPort;
   };

WB 2.x:

   - Message port named "SetPatch Port"
   struct SetPatch_2 {
       struct MsgPort sp_MsgPort;
       UWORD    sp_Version_Major;
       UWORD    sp_Version_Minor;
       struct SetPatch_2_PatchTable *sp_PatchTable;
       ULONG    sp_PatchEntrySize;
       ULONG    sp_ThisIsTheValue2;
   };

   struct SetPatch_2_Entry {
       ULONG        se_Valid;       /* 0 terminates the list */
       CONST_STRPTR se_Name;
   };

WB 3.x:

    - Semaphore named "\253 SetPatch \273"

    struct SetPatch_3 {
        struct SignalSemaphore  sp_Semaphore;
        struct MinList          sp_PatchList;
        UWORD                   sp_Version_Major;
        UWORD                   sp_Version_Minor;
    };

    struct SetPatch_3_Entry {
        struct MinNode          se_Node;
        CONST_STRPTR            se_Name;
    };
    



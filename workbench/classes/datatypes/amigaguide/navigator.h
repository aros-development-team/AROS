#ifndef AG_NAVIGATOR_H
#define AG_NAVIGATOR_H

/*
** $PROJECT: amigaguide.datatype
**
** $VER: navigator.h 50.1 (07.06.03)
**
** $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
**
*/

/* --------------------------------- tags --------------------------------- */

#define NA_Dummy        (TAG_USER + 0x40000)

#define NA_Buttons      (NA_Dummy + 0x1)
#define NA_Distance     (NA_Dummy + 0x2)
#define NA_Target       (NA_Dummy + 0x3)
#define NA_Command      (NA_Dummy + 0x4)

/* ------------------------------ structures ------------------------------ */

struct NavigatorButton
{
   STRPTR nb_Text;
   ULONG nb_Command;
   ULONG nb_Flags;
};

#define NBF_DISABLED       (1<<0)
#define NBF_SELECTED       (1<<1)
#define NBF_NEEDRENDERING  (1<<2)

#define NVM_CHANGESTATUS      0x200       /* change status of a button */
#define NVM_CHANGED           0x201       /* get number of changed buttons */

struct NavigatorStatus
{
   ULONG ns_Command;
   ULONG ns_Status;
};

struct npChangeStatus
{
   ULONG MethodID;
   struct GadgetInfo *np_GInfo;
   ULONG np_NumCommands;
   struct NavigatorStatus *np_Commands;
};

#define NVS_ENABLE        0x0000
#define NVS_DISABLE       0x0001

#define NVS_MASK          0x00ff
#define NVF_RENDER        0x0100

#define NVA_Dummy              (TAG_USER + 0x1000)

#define NVA_Selected           (NVA_Dummy + 0x0)

#endif

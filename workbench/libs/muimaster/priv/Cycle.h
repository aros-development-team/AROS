/****************************************************************************/
/** Cycle                                                                  **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Cycle[];
#else
#define MUIC_Cycle "Cycle.mui"
#endif

/* Attributes */

#define MUIA_Cycle_Active                   0x80421788 /* V4  isg LONG              */
#define MUIA_Cycle_Entries                  0x80420629 /* V4  i.. STRPTR *          */

#define MUIV_Cycle_Active_Next -1
#define MUIV_Cycle_Active_Prev -2


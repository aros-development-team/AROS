#ifndef __EVENTHANDLER_H__
#define __EVENTHANDLER_H__

/************************/
/* Window Event Handler */
/************************/

struct MUI_EventHandlerNode
{
	struct MinNode ehn_Node;
	BYTE           ehn_Reserved; /* don't touch! */
	BYTE           ehn_Priority; /* event handlers are inserted according to their priority. */
	UWORD          ehn_Flags;    /* certain flags, see below for definitions. */
	Object        *ehn_Object;   /* object which should receive MUIM_HandleEvent. */
	struct IClass *ehn_Class;    /* if !=NULL, MUIM_HandleEvent is invoked on exactly this class with CoerceMethod(). */
	ULONG          ehn_Events;   /* one or more IDCMP flags this handler should react on. */
};

/* flags for ehn_Flags */
#define MUI_EHF_ALWAYSKEYS (1<<0)

/* other values reserved for future use */

/* return values for MUIM_HandleEvent (bit-masked, all other bits must be 0) */
#define MUI_EventHandlerRC_Eat (1<<0) /* stop MUI from calling other handlers */


#endif

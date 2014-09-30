/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/execbase.h>
#include <devices/rawkeycodes.h>
#include <devices/inputevent.h>

#define NUM_STDKEYS 89
#define NOKEY -1

struct Library *KeymapBase;

static WORD std_keytable[NUM_STDKEYS] =
{
    NOKEY   	    	,  /* 000 (0x00) */
    RAWKEY_ESCAPE   	,  /* 001 (0x01) K_Escape */
    RAWKEY_1	    	,  /* 002 (0x02) K_1 */
    RAWKEY_2	    	,  /* 003 (0x03) K_2 */
    RAWKEY_3	    	,  /* 004 (0x04) K_3 */
    RAWKEY_4	    	,  /* 005 (0x05) K_4 */
    RAWKEY_5	    	,  /* 006 (0x06) K_5 */
    RAWKEY_6	    	,  /* 007 (0x07) K_6 */
    RAWKEY_7	    	,  /* 008 (0x08) K_7 */
    RAWKEY_8	    	,  /* 009 (0x09) K_8 */
    RAWKEY_9	    	,  /* 010 (0x0A) K_9 */
    RAWKEY_0	    	,  /* 011 (0x0B) K_0 */
    RAWKEY_MINUS    	,  /* 012 (0x0C) K_Minus */
    RAWKEY_EQUAL    	,  /* 013 (0x0D) K_Equal */
    RAWKEY_BACKSPACE	,  /* 014 (0x0E) K_Backspace */
    RAWKEY_TAB	    	,  /* 015 (0x0F) K_Tab */
    RAWKEY_Q	    	,  /* 016 (0x10) K_Q */
    RAWKEY_W	    	,  /* 017 (0x11) K_W */
    RAWKEY_E	    	,  /* 018 (0x12) K_E */
    RAWKEY_R	    	,  /* 019 (0x13) K_R */
    RAWKEY_T	    	,  /* 020 (0x14) K_T */
    RAWKEY_Y	    	,  /* 021 (0x15) K_Y */
    RAWKEY_U	    	,  /* 022 (0x16) K_U */
    RAWKEY_I	    	,  /* 023 (0x17) K_I */
    RAWKEY_O	    	,  /* 024 (0x18) K_O */
    RAWKEY_P	    	,  /* 025 (0x19) K_P */
    RAWKEY_LBRACKET 	,  /* 026 (0x1A) K_LBracket */
    RAWKEY_RBRACKET 	,  /* 027 (0x1B) K_RBracket */
    RAWKEY_RETURN   	,  /* 028 (0x1C) K_Enter */
    RAWKEY_LCONTROL 	,  /* 029 (0x1D) K_LCtrl */
    RAWKEY_A	    	,  /* 030 (0x1E) K_A */
    RAWKEY_S	    	,  /* 031 (0x1F) K_S */
    RAWKEY_D	    	,  /* 032 (0x20) K_D */
    RAWKEY_F	    	,  /* 033 (0x21) K_F */
    RAWKEY_G	    	,  /* 034 (0x22) K_G */
    RAWKEY_H	    	,  /* 035 (0x23) K_H */
    RAWKEY_J	    	,  /* 036 (0x24) K_J */
    RAWKEY_K	    	,  /* 037 (0x25) K_K */
    RAWKEY_L	    	,  /* 038 (0x26) K_L */
    RAWKEY_SEMICOLON	,  /* 039 (0x27) K_Semicolon */
    RAWKEY_QUOTE    	,  /* 040 (0x28) K_Quote */
    RAWKEY_TILDE    	,  /* 041 (0x29) K_BackQuote */
    RAWKEY_LSHIFT   	,  /* 042 (0x2A) K_LShift */
    RAWKEY_2B	    	,  /* 043 (0x2B) K_BackSlash */
    RAWKEY_Z	    	,  /* 044 (0x2C) K_Z */
    RAWKEY_X	    	,  /* 045 (0x2D) K_X */
    RAWKEY_C	    	,  /* 046 (0x2E) K_C */
    RAWKEY_V	    	,  /* 047 (0x2F) K_V */
    RAWKEY_B	    	,  /* 048 (0x30) K_B */
    RAWKEY_N	    	,  /* 049 (0x31) K_N */
    RAWKEY_M	    	,  /* 050 (0x32) K_M */
    RAWKEY_COMMA    	,  /* 051 (0x33) K_Comma */
    RAWKEY_PERIOD   	,  /* 052 (0x34) K_Period */
    RAWKEY_SLASH    	,  /* 053 (0x35) K_Slash */
    RAWKEY_RSHIFT   	,  /* 054 (0x36) K_RShift */
    0x5c    	    	,  /* 055 (0x37) K_KP_Multiply */
    RAWKEY_LALT     	,  /* 056 (0x38) K_LAlt */
    RAWKEY_SPACE    	,  /* 057 (0x39) K_Space */
    RAWKEY_CAPSLOCK 	,  /* 058 (0x3A) K_CapsLock */
    RAWKEY_F1	    	,  /* 059 (0x3B) K_F1 */
    RAWKEY_F2	    	,  /* 060 (0x3C) K_F2 */
    RAWKEY_F3	    	,  /* 061 (0x3D) K_F3 */
    RAWKEY_F4	    	,  /* 062 (0x3E) K_F4 */
    RAWKEY_F5	    	,  /* 063 (0x3F) K_F5 */
    RAWKEY_F6	    	,  /* 064 (0x40) K_F6 */
    RAWKEY_F7	    	,  /* 065 (0x41) K_F7 */
    RAWKEY_F8	    	,  /* 066 (0x42) K_F8 */
    RAWKEY_F9	    	,  /* 067 (0x43) K_F9 */
    RAWKEY_F10	    	,  /* 068 (0x44) K_F10 */
    0x5A    	    	,  /* 069 (0x45) K_NumLock */
    NOKEY   	    	,  /* 070 (0x46) K_Scroll_Lock ???? */
    RAWKEY_KP_7     	,  /* 071 (0x47) K_KP_7 */
    RAWKEY_KP_8     	,  /* 072 (0x48) K_KP_8 */
    RAWKEY_KP_9     	,  /* 073 (0x49) K_KP_9 */
    0x5D    	    	,  /* 074 (0x4A) K_KP_Sub */
    RAWKEY_KP_4     	,  /* 075 (0x4B) K_KP_4 */
    RAWKEY_KP_5     	,  /* 076 (0x4C) K_KP_5 */
    RAWKEY_KP_6     	,  /* 077 (0x4D) K_KP_6 */
    0x5E    	    	,  /* 078 (0x4E) K_KP_Add */
    RAWKEY_KP_1     	,  /* 079 (0x4F) K_KP_1 */
    RAWKEY_KP_2     	,  /* 080 (0x50) K_KP_2 */
    RAWKEY_KP_3     	,  /* 081 (0x51) K_KP_3 */
    RAWKEY_KP_0     	,  /* 082 (0x52) K_KP_0 */
    RAWKEY_KP_DECIMAL	,  /* 083 (0x53) K_KP_Decimal */
    NOKEY   	    	,  /* 084 (0x54) */
    NOKEY   	    	,  /* 085 (0x55) */
    RAWKEY_LESSGREATER	,  /* 086 (0x56) K_LessGreater */
    RAWKEY_LAMIGA   	,  /* 087 (0x57) K_F11 - Note: we map to LAMIGA, F11 rawkey code would be 0x4B */
    RAWKEY_RAMIGA   	   /* 088 (0x58) K_F12 - Note: we map to RAMIGA, F12 rawkey code would be 0x6F */
      
};

static void dotest(void)
{
    struct Window *win;
    struct IntuiMessage *msg;
    BOOL quitme = FALSE;
    
    KeymapBase = OpenLibrary("keymap.library", 0);
    if (!KeymapBase) return;
    
    win = OpenWindowTags(NULL, WA_Left, 20,
    	    	    	       WA_Top, 20,
			       WA_Width, 300,
			       WA_Height, 20,
			       WA_Activate, TRUE,
			       WA_CloseGadget, TRUE,
			       WA_DepthGadget, TRUE,
			       WA_Title, (IPTR)"IDCMP_RAWKEY Test",
			       WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
			       TAG_DONE);
    if (!win)
    {
    	CloseLibrary(KeymapBase);
    	return;
    }
    
    while(!quitme)
    {
    	WaitPort(win->UserPort);
	while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch(msg->Class)
	    {
	    	case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		    
		case IDCMP_RAWKEY:
		    printf("RAWKEY %s: %3d (0x%02x) QUAL=0x%04x DEAD=0x%08lx ", 
		    	    (msg->Code & IECODE_UP_PREFIX) ? "UP  " : "DOWN",
			    (msg->Code & ~IECODE_UP_PREFIX),
			    (msg->Code & ~IECODE_UP_PREFIX),
			    msg->Qualifier,
			    (long)*(ULONG *)msg->IAddress);
			    
		    {
		    	struct InputEvent ie;
			char buf[20];
			WORD ret;
			
			ie.ie_Class         = IECLASS_RAWKEY;
    	    	    	ie.ie_Code          = msg->Code;
			ie.ie_Qualifier     = msg->Qualifier;
			ie.ie_EventAddress  = *((ULONG **) msg->IAddress);
			
			ret = MapRawKey(&ie, buf, sizeof(buf), NULL);
			if (ret >= 0) buf[ret] = 0;
			
    	    	    	printf("MAPPED=%3d (%s)\n", ret, (ret == -1) ? "<NONE>" : ((ret == 0) ? "<ZERO>" : buf));
		    }
		    break;
	    }
	    ReplyMsg((struct Message *)msg);
	}
    }
    
    CloseWindow(win);
    CloseLibrary(KeymapBase);
}

int main(int argc, char **argv)
{
    WORD *table;
    WORD startindex = 0;
    WORD endindex = 88;
    WORD index;
    WORD trashed = 0;
    
    if (argc == 2)
    {
    	if (strcmp(argv[1], "test") == 0)
	{
	    dotest();
    	    return 0;
	}
	
    	index = strtol(argv[1], 0, 0);
	
	if (index < 0) index = 0;
	if (index > 88) index = 88;
	
	startindex = endindex = index;
    }

#if AROS_SIZEOFULONG == AROS_SIZEOFPTR
    table = (WORD *)SysBase->ex_Reserved2[1]; /* set up in config/i386-native/Drivers/keyboard/kbdclass.c */
#else
    table = NULL;
#endif
    if (!table)
    {
    	puts("Error: table points to NULL!\n");
	return 0;
    }
    
    printf("Table address = 0x%p\n\n", table);
    
    for(index = startindex; index <= endindex; index++)
    {
    	printf("#%02d: %d (0x%x)\n", index, table[index], table[index]);
	if (table[index] != std_keytable[index]) trashed++;
    }
    
    if (trashed)
    {
    	printf("\n=============== Found %d trashed entries ============\n", trashed);
    	for(index = startindex; index <= endindex; index++)
    	{
	    if (table[index] != std_keytable[index])
	    {
    	    	printf("#%02d: %d (0x%x)  should be: %d (0x%x)\n", index, table[index], table[index], std_keytable[index], std_keytable[index]);
	    }
    	}	
    }
    
    return 0;
}


#include "global.h"

// inputLoop() does pretty much what the name suggests: Font sits quietly waiting for any
// event it's listening for (IDCMP and CTRL-C signal pattern matches). The returning BYTE
// gets fed to the quitApp() function, as found in main.c
UBYTE inputLoop()
{
    ULONG signals, class;
    UWORD code, menuNumber;
    UBYTE returnCode = RETURN_OK, fontChoice = 0; // fontChoice: Font we're editing right now
    BOOL running = TRUE;
    STRPTR fileName;
    struct MenuItem *menuItem;
    struct IntuiMessage *intuiMessage;
    struct Gadget *gadget;
    extern struct FontPrefs *fontPrefs[3];  // init.c

    while(running)
    {
	kprintf("Wait()...");
	signals = Wait(1L << appGUIData->agd_Window->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);
	kprintf(" signal received!\n");

	if(signals & (1L << appGUIData->agd_Window->UserPort->mp_SigBit))
	{
	    kprintf("Message at Window->UserPort->mp_SigBit received!\n");
	    // We got a IDCMP message, analyze it and take action

	    while(NULL != (intuiMessage = GT_GetIMsg(appGUIData->agd_Window->UserPort)))
	    {
		kprintf("GT_GetIMsg() called... ");

		class = intuiMessage->Class;
		code = intuiMessage->Code;
		menuNumber = intuiMessage->Code;
		gadget = (struct Gadget *)intuiMessage->IAddress;

		GT_ReplyIMsg(intuiMessage);
		kprintf("GT_ReplyIMsg()!\n");

		if(HandleRegisterTabInput(appGUIData->agd_RegisterTab, intuiMessage))
		{
		    processRegisterTab(); // Deal with register tab
		}
		else
		{
		    switch(class)
		    {
			case IDCMP_CLOSEWINDOW :
			    running = FALSE;
			break;

			// Don't do anything - IDCMP_MOUSEBUTTON is only used by the HandleRegisterTabInput() code!
			case IDCMP_MOUSEBUTTONS :
			    kprintf("Received IDCMP_MOUSEBUTTON!\n");
			break;

			case IDCMP_VANILLAKEY :
			    switch(intuiMessage->Code)
			    {
				case 27 : // User has pressed the escape key
				    running = FALSE;
				break;
			    }
			break;

			case IDCMP_MENUPICK :
			    // Make sure to process every single menu choice made. RKRM: Libraries p. 176
			    while(menuNumber != MENUNULL)
			    {
				menuItem = ItemAddress(appGUIData->agd_Menu, menuNumber);

				kprintf("%d chosen!\n", (UBYTE)GTMENUITEM_USERDATA(menuItem));

				// The output from GTMENUITEM_USERDATA must be casted
				switch((UBYTE)GTMENUITEM_USERDATA(menuItem))
				{
				    case MSG_MEN_PROJECT_OPEN :
					if((fileName = aslOpenPrefs()))
					{
					    kprintf("reading %s...\n", fileName);

					    if(!(readIFF(fileName, fontPrefs)))
					    {
						returnCode = RETURN_FAIL;
						running = FALSE;
					    }

					    updateGUI(fontChoice); // For the moment, always update the GUI
					}
				    break;

				    case MSG_MEN_PROJECT_SAVEAS :
					if((fileName = aslSavePrefs()))
					{
					    kprintf("saving %s...\n", fileName);

					    if(!(writeIFF(fileName, fontPrefs)))
					    {
						returnCode = RETURN_FAIL;
						running = FALSE;
					    }
					}
				    break;

				    case MSG_MEN_PROJECT_QUIT :
					kprintf("Menu quit!\n");

					running = FALSE;
				    break;

				    case MSG_MEN_EDIT_DEFAULT :
					kprintf("Menu defaults!\n");

					initDefaultPrefs(fontPrefs);

					updateGUI(fontChoice); // For the moment, always update the GUI
				    break;

				    case MSG_MEN_EDIT_LASTSAVED :
					kprintf("Last saved!\n");

					if(!(readIFF("ENVARC:sys/font.prefs", fontPrefs)))
					{
					    returnCode = RETURN_FAIL;
					    running = FALSE;
					}

					updateGUI(fontChoice); // For the moment, always update the GUI
				    break;

				    case MSG_MEN_EDIT_RESTORE :
					kprintf("Menu restore!\n");

					if(!(readIFF("ENV:sys/font.prefs", fontPrefs)))
					{
					    returnCode = RETURN_FAIL;
					    running = FALSE;
					}

					updateGUI(fontChoice); // For the moment, always update the GUI
				    break;

				    default :
					kprintf("Warning: Unknown menu item!\n");
				    break;
				}

				menuNumber = menuItem->NextSelect;
			    }
			break;

			case IDCMP_GADGETUP :
			    switch(gadget->GadgetID)
			    {
				case BUTTON_GETFONT :
				    kprintf("getfont\n");

				    if(getFont(fontPrefs[fontChoice]))
					updateGUI(fontChoice);
				break;

				case BUTTON_SAVE :
				    kprintf("save\n");

				    if(!((writeIFF("ENV:sys/font.prefs", fontPrefs)) &&
					(writeIFF("ENVARC:sys/font.prefs", fontPrefs))))
					returnCode = RETURN_FAIL;

				    running = FALSE;
				break;

				case BUTTON_USE :
				    kprintf("use\n");

				    if(!(writeIFF("ENV:sys/font.prefs", fontPrefs)))
					returnCode = RETURN_FAIL;

				    running = FALSE;
				break;

				case BUTTON_CANCEL :
				    kprintf("cancel\n");

				    running = FALSE;
				break;

				default :
				    kprintf("Warning: Unknown button pressed!\n");
				break;
			    }
			break;

			case IDCMP_GADGETDOWN : // This message applies for the "MX" gadget
			    kprintf("IDCMP_GADGETDOWN!\n");

			     switch(gadget->GadgetID)
			     {
				case MX_CHOOSEFONT :
				    if(code != fontChoice) /* No reason to update GUI if nothing has changed */
				    {
					fontChoice = code;
					updateGUI(fontChoice);
				    }
				break;

				default :
				    kprintf("Warning: Unknown kind of IDCMP_GADGETDOWN message!\n");
				break;
			    }
			break;

			case IDCMP_REFRESHWINDOW :
			    kprintf("IDCMP_REFRESHWINDOW!\n");

			    GT_BeginRefresh(appGUIData->agd_Window);
			    GT_RefreshWindow(appGUIData->agd_Window, NULL); // Do we really need to call GT_RefreshWindow() after actually openening the window? Look it up! 			GT_EndRefresh(appGUIData->agd_Window, TRUE);

			    // TODO: "Font" currently calls RenderRegisterTab() at IDCMP_REFRESHWINDOW events - OK? (petah)
			    kprintf("1: OK?\n");
			    //RenderRegisterTab(appGUIData->agd_Window->RPort, appGUIData->agd_RegisterTab, TRUE);
			    kprintf("2: OK?\n");
			break;

			default :
			    kprintf("Warning - received IDCMP class %d, unable to take action! (Unknown message!)\n", class);
			break;
		    } // switch(class)

		} // else

	    } // GetIMsg()

	} // if(signals & userport)

	if(signals & SIGBREAKF_CTRL_C)
		running = FALSE;

    } // while(running)
    
    return(returnCode);
}

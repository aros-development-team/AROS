//////////////////////////////////////////////////////////////
//                                                          //
// AROS PORT by LuKeJerry (at) gmail.com                    //
// Based on the program version with standard window        //
//  + BackFill                                              //
//                                                          //
//   -- last update: 26-04-2012                             //
//     - Fixed the notification                             //
//     - Use D(bug()) macros                                //
// -------------------------------------------------------- //
//                                                          //
//   -- translated to ENG from 26-09-2011                   //
//   -- converted TABs to 4-Spaces                          //
//                                                          //
// - FIXME:                                                 //
//                                                          //
//     . When there are more icons than the screen width    //
//       can handle, exceeding icons should appear in the   //
//       next submenu. Seems that icon space is allocated,  //
//       but icon is not appearing                          //
//       Look in: Read_Prefs( and SetWindowParameters(      //
//                                                          //
//  TO DO:                                                  //
//                                                          //
//   - Even in static mode, put BiB window                  //
//          active when mouse cursor goes to very           //
//          bottom of screen                                //
//                                                          //
//   - Add possibility to show icon labels                  //
//   - Add support for screennotify                         //
//   - Add support for d&d to add an icon to a dock         //
//   - Add commodities CX support                           //
//                                                          //
//////////////////////////////////////////////////////////////

//#define DEBUG 1
#include <aros/debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <proto/workbench.h>
#include <proto/icon.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/commodities.h>
#include <proto/alib.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <workbench/icon.h>
#include <workbench/startup.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <devices/rawkeycodes.h>
#include <dos/dos.h>

#include "locale.h"

#define SUM_ICON  200
#define ICON_ACTIVE (1<<7)
#define SELECTED_ICON (1<<6)

#define TEMPLATE "SPACE/N/K,STATIC/N/K,AUTOREMAP/S,BACKGROUND/S"
#define ARG_SPACE      0
#define ARG_STATIC     1
#define ARG_AUTOREMAP  2
#define ARG_BACKGROUND 3

#define BIB_PREFS "ENV:Iconbar.prefs"

/*****************************************************************************/

static BOOL BiB_Exit        = FALSE,
            Icon_Remap      = FALSE,
            Bar_Background  = FALSE,
            PositionMenuOK  = FALSE,
            // Image           = FALSE, // <--- FIXME: "Image" seems not used at all 
            Window_Active   = FALSE,
            Window_Open     = FALSE,
            MenuWindow_Open = FALSE,
            FirstOpening    = TRUE;

static int  Static          = 0,
            Spacing         = 5,
            CurrentLevel    = 0,
            lbm             = 0,
            rbm             = 0;

static int  WindowHeight, 
            WindowWidth, 
            ScreenHeight, 
            ScreenWidth, 
            IconWidth,
            // ImageWidth, 
            // ImageHeight, // <---  FIXME: ...also: ImageWidth, ImageHeight not used at all 
            Position,
            OldPosition,   
            IconCounter,
            LevelCounter,
            MouseIcon,
            Lenght, 
            BeginningWindow, 
            EndingWindow, 
            Window_Max_X, 
            Window_Max_Y;

const char version[] = "$VER: BoingIconBar 1.02 (26.04.2012) ©2012 Robert 'Phibrizzo' Krajcarz";

static char //Background_name[256], // <-- FIXME: Background_name not used at all,
            BufferList[20],       //     but might be interesting to implement
            MovingTable[8]  = {0, 4, 7, 9, 10, 9, 7, 4};

static BOOL notification = FALSE;

static ULONG    WindowMask, 
                MenuMask, 
                CxSigMask,
                NotifMask,
                WindowSignal;

static IPTR     args[] = {(IPTR)&Spacing, (IPTR)&Static, 0, 0};

/*****************************************************************************/

static struct DiskObject *Icon[SUM_ICON]; 

static struct Window *Window_struct, *MenuWindow_struct;    //  FIXME: really ugly _struct names...
static struct Screen *Screen_struct;                       

static struct BitMap *BMP_Buffer, *BMP_DoubleBuffer;
static struct RastPort RP_Buffer, RP_DoubleBuffer;
static CxObj *broker;

// struct of icons

static struct Icon_Struct 
{
    int    Icon_Height;     // icon height 
    int    Icon_Width;      // icon width 
    int    Icon_PositionX;  // X position on main window
    int    Icon_PositionY;  // Y position on main window
    int    Icon_Status;     // status of icon: normal or selected
    BOOL   Icon_OK;         // everything OK with icon
    char   Icon_Path[255];  // name to path of icon
} Icons_Struct[SUM_ICON];

// struct of submenu

static struct Level_Struct 
{
     char Level_Name[20];   // name submenu - level name 
     int  Beginning;        // first icon on menu
     int  WindowPos_X;      // X position main window
     int  WindowPos_Y;      // Y position main window
} Levels_Struct[11];

static struct TextAttr Topaz8 = {"topaz.font",8,0,FPF_ROMFONT};

static struct IntuiText Names = {1,0,JAM1,0,0,&Topaz8,BufferList,NULL};

//  background pictures

static Object *picture[3];
static struct BitMap *bm[3];

static struct Struct_BackgroundData 
{
    int Width;   // width
    int Height;  // height
} BackgroundData[3];

static struct NewBroker newbroker;

static struct NotifyRequest *nr;
static struct MsgPort *BIBport, *BrokerMP;
struct RDArgs *rda;
static struct DiskObject *dob;

/*****************************************************************************/
// functions
static int  ReadPrefs(void);   // load prefs
static void LoadBackground(void);  //load background pictures
static void SetWindowParameters(void);   // check window sizes
static void Decode_IDCMP(struct IntuiMessage *KomIDCMP);  //  decode IDCMP main signals
static void Change_State(int Reset);  // change icon state
static void Insert_Icon(int Tryb, int IconNumber);  // draw icon
static void Blink_Icon(int IconNumber); // blink the icon
static BOOL OpenMainWindow(void);  // open main window
static void CloseMainWindow(void);  // close main window
static void CheckMousePosition(void);  // check mouse position
static void Show_Selected_Level(void);  // change the submenu
static void OpenMenuWindow(void); // open menu window
static void CloseMenuWindow(void);  // close menu window
static void Decode_IDCMP2(struct IntuiMessage *KomIDCMP);  // decode IDCMP menu signals
static void Launch_Program(char *Program);  // start the chosed program
static void Settings(void);  // open the prefs program
static void Reload_BiB(void); // reload the BiB
static void HandleCx(void); // handle Cx messages
static void CleanExit(CONST_STRPTR msg); // Cleanup and print a message

/*****************************************************************************/

int main(int argc, char *argv[])
{
    int outt = FALSE;

    struct IntuiMessage *KomIDCMP, KopiaIDCMP;
    BPTR   out = BNULL;

    newbroker.nb_Version = NB_VERSION;
    newbroker.nb_Name    = _(MSG_CX_NAME);     // string to identify this broker
    newbroker.nb_Title   = _(MSG_CX_TITLE);
    newbroker.nb_Descr   = _(MSG_CX_DESC);
    newbroker.nb_Unique  = NBU_UNIQUE | NBU_NOTIFY;

    if ((BrokerMP = CreateMsgPort()) != NULL)
    {
        newbroker.nb_Port = BrokerMP;
        CxSigMask = 1L << BrokerMP->mp_SigBit;
        broker = CxBroker(&newbroker, NULL);
        if (broker == NULL)
        {
            D(bug("[IconBar] is already running\n"));
            CleanExit("");
        }
    }

    ActivateCxObj(broker, 1);
    D(bug("[IconBar] broker %p\n", broker));

    // Read arguments
    if (argc)
    {
        // Started from Shell
        if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
        {
            PrintFault(IoErr(), argv[0]);
            return RETURN_FAIL;
        }

        if (args[ARG_AUTOREMAP]) Icon_Remap = TRUE;

        if (args[ARG_BACKGROUND]) Bar_Background = TRUE;

        out = Output();
    } else {
        // Started from WB: Read ToolTypes
        struct WBStartup *wbs=(struct WBStartup*)argv;
        struct WBArg *wba=&wbs->sm_ArgList[wbs->sm_NumArgs-1];
        BPTR oldcd;

        if (!(*wba->wa_Name))
        {
            return RETURN_FAIL;
        }

        oldcd=CurrentDir(wba->wa_Lock);
        if ((dob=GetDiskObjectNew(wba->wa_Name)))
        {
            char *str;

            if ((str=FindToolType(dob->do_ToolTypes, "SPACE")))
                *(ULONG*)args[ARG_SPACE]=(ULONG)atoi(str);

            if ((str=FindToolType(dob->do_ToolTypes, "STATIC")))
                *(ULONG*)args[ARG_STATIC]=(ULONG)atoi(str);

            if ((str=FindToolType(dob->do_ToolTypes, "AUTOREMAP")))
                Icon_Remap = TRUE;
            if ((str=FindToolType(dob->do_ToolTypes, "BACKGROUND")))
                Bar_Background = TRUE;
        }

        CurrentDir(oldcd);
        outt = TRUE;
    }

    if (out || outt)
    {
        Spacing = *(LONG*)args[ARG_SPACE];
        Static  = *(LONG*)args[ARG_STATIC];
    }

    D(bug("[IconBar] Specified options: SPACE=%d, STATIC=%d, AUTOREMAP=%d\n", Spacing, Static, Icon_Remap));

    // Read the prefs file, fail if it doesn't exist
    if (!ReadPrefs())
    {
        CleanExit("Can't read preferences\n");
    }

    // Set notification of prefs change
    if ((BIBport = CreateMsgPort()))
    {
        if ((nr = AllocMem(sizeof(struct NotifyRequest), MEMF_CLEAR)))
        {
            nr->nr_Name = BIB_PREFS;

            D(bug("[IconBar] prefs filename: %s\n",nr->nr_Name));

            nr->nr_Flags = NRF_SEND_MESSAGE;
            nr->nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
            nr->nr_stuff.nr_Msg.nr_Port = BIBport;

            if (StartNotify(nr) != DOSFALSE)
            {
                notification = TRUE;
                NotifMask = 1L << BIBport->mp_SigBit;
                D(bug("[IconBar] Notification started\n"));
            }
        }
    }
    if (Bar_Background)
        LoadBackground();

    if(Static)
    {
        Delay(Static * 50);
        OpenMainWindow();
        FirstOpening = FALSE;
    }

    // ---- main loop

    while (BiB_Exit==FALSE)
    {
        if (Window_Open || MenuWindow_Open)
        {
            WindowSignal = Wait(WindowMask | MenuMask | CxSigMask | NotifMask);
        }
        else
        {
            Delay(50);
            CheckMousePosition();
            WindowSignal = 0;
        }

        if(WindowSignal & WindowMask)
        {
            while((KomIDCMP = GT_GetIMsg(Window_struct->UserPort)))
            {
                CopyMem(KomIDCMP, &KopiaIDCMP, sizeof(struct IntuiMessage));
                GT_ReplyIMsg(KomIDCMP);
                Decode_IDCMP(&KopiaIDCMP);
            }

            if(!(Static) && Window_Active == FALSE) CloseMainWindow();
        }

        if(WindowSignal & MenuMask)
        {
            while((KomIDCMP = GT_GetIMsg(MenuWindow_struct->UserPort)))
            {
                CopyMem(KomIDCMP, &KopiaIDCMP, sizeof(struct IntuiMessage));
                GT_ReplyIMsg(KomIDCMP);
                Decode_IDCMP2(&KopiaIDCMP);
            }

            if(MenuWindow_Open == FALSE) CloseMenuWindow();
        }

        if(WindowSignal & CxSigMask)
        {
            HandleCx();
        }

        if(WindowMask & NotifMask)
        {
            if (GetMsg(BIBport) != NULL)
            {
                D(bug("[IconBar] Prefs modified!\n"));
                Reload_BiB();
            }
        }
    } // ---- end of main loop

    D(bug("[IconBar] Quitting...\n"));

    CleanExit(NULL);
    return 0;
}

/*****************************************************************************/

static int ReadPrefs(void)
{
    BPTR Prefs;
    int x, TextLenght, NumChars;

    Icons_Struct[0].Icon_OK = FALSE;
    IconCounter  = 0;
    LevelCounter = 0;
    Lenght       = 0;

    if((Prefs = Open(BIB_PREFS, MODE_OLDFILE)))
    {
        FGets(Prefs, Icons_Struct[0].Icon_Path, 255);

        if(strncmp(Icons_Struct[0].Icon_Path, "BOING_PREFS", 11) != 0)
        {
            Close(Prefs);
            //FIXME: A better error handling might be added
            D(bug("[IconBar] Prefs error\n"));
            return(0);
        }

        if((Screen_struct = LockPubScreen(NULL)) != NULL)
        {
            while (FGets(Prefs, Icons_Struct[IconCounter].Icon_Path, 255))
            {
                NumChars = strlen(Icons_Struct[IconCounter].Icon_Path);
                Icons_Struct[IconCounter].Icon_Path[NumChars-1] = '\0';
                
                if (Icons_Struct[IconCounter].Icon_Path[0] == ';')
                {
                    D(bug("[IconBar] Found a submenu! Name lenght = %d, last char = %d\n",
                    NumChars, Icons_Struct[IconCounter].Icon_Path[NumChars-1]));

                    if (NumChars > 20) NumChars = 20;
                    Icons_Struct[IconCounter].Icon_Path[19] = '\0';

                    for (x=1; x<NumChars; x++)
                    {
                        Levels_Struct[LevelCounter].Level_Name[x-1] = Icons_Struct[IconCounter].Icon_Path[x];
                    }

                    D(bug("[IconBar] x after \'for\' = %d\n", x));

                    strcpy(BufferList, Levels_Struct[LevelCounter].Level_Name);

                    D(bug("[IconBar] Level name: %s\n", Levels_Struct[LevelCounter].Level_Name));

                    TextLenght = IntuiTextLength(&Names);

                    if (TextLenght > Lenght) Lenght = TextLenght;

                    D(bug("[IconBar] IntuiTextLenght = %d\n", TextLenght));

                    D(bug("[IconBar] Submenu %d - %s, beginning %d\n",
                        LevelCounter, Levels_Struct[LevelCounter].Level_Name, Levels_Struct[LevelCounter].Beginning));

                    Levels_Struct[LevelCounter].Beginning = IconCounter;
                    LevelCounter++;
                } else {
                    D(bug("[IconBar] Loading icon #%d - name: %s\n", IconCounter, Icons_Struct[IconCounter].Icon_Path));
                    
                    if ((Icon[IconCounter] = GetIconTags(Icons_Struct[IconCounter].Icon_Path,
                        ICONGETA_RemapIcon, FALSE,
                        TAG_DONE)))
                    {
                        IconControl(Icon[IconCounter],
                            ICONCTRLA_GetWidth, &Icons_Struct[IconCounter].Icon_Width,
                            ICONCTRLA_GetHeight, &Icons_Struct[IconCounter].Icon_Height,
                            ICONCTRLA_SetNewIconsSupport, TRUE,
                            TAG_END);
                                        
                        if (Icons_Struct[IconCounter].Icon_Width == 0)
                        {
                            Icons_Struct[IconCounter].Icon_Width = Icon[IconCounter]->do_Gadget.Width;
                        }

                        if (Icons_Struct[IconCounter].Icon_Height == 0)
                        {
                            Icons_Struct[IconCounter].Icon_Height = Icon[IconCounter]->do_Gadget.Height;
                        }

                        LayoutIcon(Icon[IconCounter], Screen_struct, NULL);

                        D(bug("[IconBar] Dimension icon #%d: Widht=%d Height=%d\n",
                            IconCounter, Icons_Struct[IconCounter].Icon_Width, Icons_Struct[IconCounter].Icon_Height));

                        D(bug("[IconBar] Icon %d loaded\n", IconCounter));

                        Icons_Struct[IconCounter].Icon_OK = TRUE;
                        IconCounter++;

                        if (IconCounter == SUM_ICON) break;
                    } else {
                        //FIXME: A better error handling might be added
                        D(bug("[IconBar] Not found icon %s.\n", Icons_Struct[IconCounter].Icon_Path));
                    }
                }
            }
            Close(Prefs);

            ScreenWidth = Screen_struct->Width;
            ScreenHeight  = Screen_struct->Height;

            UnlockPubScreen(NULL, Screen_struct);

            Levels_Struct[LevelCounter].Beginning = IconCounter;

            D(bug("[IconBar] Screen size: Width=%d, Height=%d\n", ScreenWidth, ScreenHeight));

            SetWindowParameters();

            IconCounter  = Levels_Struct[1].Beginning;
            WindowWidth  = Levels_Struct[0].WindowPos_X;
            WindowHeight = Levels_Struct[0].WindowPos_Y;

            BeginningWindow = (ScreenWidth>>1) - (WindowWidth>>1);
            EndingWindow    = (ScreenWidth>>1) + (WindowWidth>>1);

            CurrentLevel = 0;

            sprintf(Levels_Struct[LevelCounter].Level_Name, _(MSG_MENU_SETTINGS));
            sprintf(Names.IText, "%s", Levels_Struct[LevelCounter].Level_Name);

            TextLenght = IntuiTextLength(&Names);

            if (TextLenght > Lenght) Lenght = TextLenght;

            LevelCounter++;

            Lenght = Lenght + 10;
        } else {
            //FIXME: A better error handling might be added
            D(bug("[IconBar] Error reading screen parameters\n"));
        }
    }

    return Icons_Struct[0].Icon_OK;
}

/*****************************************************************************/

static void LoadBackground(void)
{
    int x;
    
    STRPTR name[3] = 
    {
        "SYS:System/Images/bibgfx/left"  ,
        "SYS:System/Images/bibgfx/middle",
        "SYS:System/Images/bibgfx/right"
    };

    for (x=0; x<3; x++)
    {
        picture[x] = NewDTObject (name[x],
            DTA_GroupID,   GID_PICTURE,
            PDTA_Remap,    TRUE,
            PDTA_DestMode, PMODE_V43,
            OBP_Precision, PRECISION_IMAGE,
            PDTA_Screen,   (IPTR)Screen_struct,
            TAG_END);

        if (picture[x])
        {
            D(bug("[IconBar] Loading image n. %d - name: %s\n",x,name[x]));
            
            DoDTMethod(picture[x], NULL, NULL, DTM_PROCLAYOUT, NULL, DTSIF_NEWSIZE);

            GetDTAttrs
            (   picture[x],
                PDTA_DestBitMap,  (SIPTR)&bm[x],
                DTA_NominalHoriz, (SIPTR)&BackgroundData[x].Width,
                DTA_NominalVert,  (SIPTR)&BackgroundData[x].Height,
                TAG_END
             );
        }
    }
}

/*****************************************************************************/

static void SetWindowParameters(void)
{

    int x, y;
        
    Window_Max_X = 0;
    Window_Max_Y = 0;

    Icons_Struct[0].Icon_PositionX = 0;
        
    for (y=0; y<(LevelCounter); y++)
    {
        WindowHeight  = 0;
        WindowWidth   = 0;

        IconCounter = Levels_Struct[y+1].Beginning;

        for (x=Levels_Struct[y].Beginning; x<IconCounter; x++)
        {
            if (Icons_Struct[x].Icon_OK)
            {
                if (Icons_Struct[x].Icon_Height > WindowHeight)
                {
                    WindowHeight = Icons_Struct[x].Icon_Height;
                }
        
                if (Icons_Struct[x].Icon_Width > IconWidth)
                {
                    IconWidth = Icons_Struct[x].Icon_Width;
                }

                if ((WindowWidth + Icons_Struct[x].Icon_Width + 35) > ScreenWidth)
                {
                    D(bug("[IconBar] Exceeding the size of the screen, can't display all icons!\n"));
                    
                    IconCounter = x;
                    Levels_Struct[y+1].Beginning = x;
                    break;
                }

                WindowWidth = WindowWidth + Icons_Struct[x].Icon_Width + Spacing;

                Icons_Struct[x].Icon_PositionY = 10;
                
                if ((x+1) != IconCounter)
                {
                    Icons_Struct[x+1].Icon_PositionX = WindowWidth;
                }
            }
            else break;
        }

        for (x=Levels_Struct[y].Beginning; x<IconCounter; x++)
        {
            if (Icons_Struct[x].Icon_OK)
            {
                Icons_Struct[x].Icon_PositionX = Icons_Struct[x].Icon_PositionX + 15; 
                Icons_Struct[x].Icon_PositionY = (WindowHeight>>1) - (Icons_Struct[x].Icon_Height>>1) + 15;
            }
        }

        Levels_Struct[y].WindowPos_X = WindowWidth  + 30;
        Levels_Struct[y].WindowPos_Y = WindowHeight + 20;

        if (Levels_Struct[y].WindowPos_X > ScreenWidth) 
            Levels_Struct[y].WindowPos_X = WindowWidth;

        if (Window_Max_X < Levels_Struct[y].WindowPos_X) 
            Window_Max_X = Levels_Struct[y].WindowPos_X;
        
        if (Window_Max_Y < Levels_Struct[y].WindowPos_Y) 
            Window_Max_Y = Levels_Struct[y].WindowPos_Y;

        D(bug("[IconBar] Window size %d: S=%d, W=%d\n", y, Levels_Struct[y].WindowPos_X, Levels_Struct[y].WindowPos_Y));
    }

    D(bug("[IconBar] Maximum window size: S=%d, W=%d\n", Window_Max_X, Window_Max_Y));
    D(bug("[IconBar] Icon width: %d\n", IconWidth));

    if((Screen_struct=LockPubScreen(NULL)) != NULL)
    {
        BMP_Buffer = AllocBitMap
        (
            Window_Max_X,
            Window_Max_Y,
            GetBitMapAttr(Screen_struct->RastPort.BitMap, BMA_DEPTH),
            BMF_MINPLANES | BMF_CLEAR,
            Screen_struct->RastPort.BitMap
        );

        InitRastPort(&RP_Buffer);
        RP_Buffer.BitMap = BMP_Buffer;
        RP_Buffer.Layer  = NULL;

        BMP_DoubleBuffer = AllocBitMap
        (
            IconWidth + 8,
            Window_Max_Y,
            GetBitMapAttr(Screen_struct->RastPort.BitMap, BMA_DEPTH),
            BMF_MINPLANES | BMF_CLEAR,
            Screen_struct->RastPort.BitMap
        );

        InitRastPort(&RP_DoubleBuffer);
        RP_DoubleBuffer.BitMap = BMP_DoubleBuffer;
        RP_DoubleBuffer.Layer  = NULL;

        UnlockPubScreen(NULL,Screen_struct);
    }
}

/*****************************************************************************/

static void Decode_IDCMP(struct IntuiMessage *KomIDCMP)
{
    int x;

    // LJ: Temporary disabled mouse cases in function Decode_IDCMP

    switch (KomIDCMP->Class)
    {
        case IDCMP_CLOSEWINDOW:
            BiB_Exit = TRUE;
            break;

        case IDCMP_MOUSEMOVE:
            for(MouseIcon=Levels_Struct[CurrentLevel].Beginning; MouseIcon<IconCounter; MouseIcon++)
            {
                if(Icons_Struct[MouseIcon].Icon_OK)
                {
                    // too noisy: D(bug("[IconBar] Mouse position in the window X=%d, Y=%d\n",
                        // KomIDCMP->MouseX, KomIDCMP->MouseY));

                    if((ScreenHeight - Screen_struct->MouseY) > WindowHeight)
                    {
                        Window_Active = FALSE;
                    }

                    if(KomIDCMP->MouseX > Icons_Struct[MouseIcon].Icon_PositionX &&
                        KomIDCMP->MouseX < Icons_Struct[MouseIcon].Icon_PositionX + Icons_Struct[MouseIcon].Icon_Width &&
                        KomIDCMP->MouseY > 0 &&
                        KomIDCMP->MouseY < WindowHeight - 5)
                    {
                        if((Icons_Struct[MouseIcon].Icon_Status & ICON_ACTIVE) != ICON_ACTIVE)
                        {
                            for(x=Levels_Struct[CurrentLevel].Beginning; x<IconCounter; x++)
                            {
                                Icons_Struct[x].Icon_Status = Icons_Struct[x].Icon_Status & 0x07;
                            }

                            Icons_Struct[MouseIcon].Icon_Status = ICON_ACTIVE | (SELECTED_ICON * lbm);
                            D(bug("[IconBar] Pointer on icon #%d\n", MouseIcon));
                        }
                        break;
                    }
                    Icons_Struct[MouseIcon].Icon_Status = Icons_Struct[MouseIcon].Icon_Status & 0x07;
                }
            }
            break;

        case IDCMP_MOUSEBUTTONS:
            switch (KomIDCMP->Code)
            {
                case SELECTDOWN:
                    Icons_Struct[MouseIcon].Icon_Status = Icons_Struct[MouseIcon].Icon_Status | SELECTED_ICON;
                    lbm = 1;

                    D(bug("[IconBar] Icon #%d\n", MouseIcon));
                    break;

                case SELECTUP:
                    lbm = 0;

                    if((Icons_Struct[MouseIcon].Icon_Status & (SELECTED_ICON | ICON_ACTIVE)) == (SELECTED_ICON | ICON_ACTIVE))
                    {
                        Blink_Icon(MouseIcon);

                        D(bug("[IconBar] Launching program function with param: %s\n",
                            Icons_Struct[MouseIcon].Icon_Path));
                        Launch_Program(Icons_Struct[MouseIcon].Icon_Path);

                        D(bug("[IconBar] Icon #%d confirmed\n", MouseIcon));
                    }

                    Icons_Struct[MouseIcon].Icon_Status = Icons_Struct[MouseIcon].Icon_Status & 0xBF; //10001111b

                    break;

                case MENUDOWN:
                    rbm = 1;
                    OpenMenuWindow();
                    break;
            }

        case IDCMP_RAWKEY:
            switch(KomIDCMP->Code)
            {
                case RAWKEY_ESCAPE:
                    if((KomIDCMP->Qualifier) & IEQUALIFIER_LSHIFT)
                    {
                        BiB_Exit = TRUE;
                    }
                    break;

                case RAWKEY_F1:
                    for(x=0; x<SUM_ICON; x++)
                    {
                        if(Icon[x] != NULL)
                        {
                            LayoutIcon(Icon[x],
                                Screen_struct,
                                NULL);
                        }
                    }

                    for(x=Levels_Struct[CurrentLevel].Beginning; x<IconCounter; x++)
                    {
                        if(Icon[x] != NULL)
                        {
                            DrawIconState(Window_struct->RPort,
                                Icon[x],
                                NULL,
                                Icons_Struct[x].Icon_PositionX,
                                Icons_Struct[x].Icon_PositionY,
                                IDS_NORMAL,
                                ICONDRAWA_Frameless, TRUE,
                                ICONDRAWA_EraseBackground, FALSE,
                                TAG_DONE);
                        }
                    }
                    break;

                case RAWKEY_F2:
                case RAWKEY_NM_WHEEL_DOWN:
                    CurrentLevel++;
                    if(CurrentLevel == (LevelCounter -1 )) CurrentLevel = 0;
                    Show_Selected_Level();
                    break;

                case RAWKEY_F3:
                case RAWKEY_NM_WHEEL_UP:
                    CurrentLevel--;
                    if(CurrentLevel < 0) CurrentLevel = LevelCounter - 2;
                    Show_Selected_Level();
                    break;

                case RAWKEY_F4:
                    Settings();
                    break;
            }
            break;

        case IDCMP_INACTIVEWINDOW:
            if(rbm == 0) Window_Active = FALSE;
            lbm = 0;

            if(Static)
            {
                for(x=Levels_Struct[CurrentLevel].Beginning; x<IconCounter; x++)
                {
                    if(Icons_Struct[x].Icon_Status != 0)
                    {
                        Icons_Struct[x].Icon_Status = 0;
                        Insert_Icon(IDS_NORMAL, x);
                    }
                }
            }

            break;

        case IDCMP_INTUITICKS:
            Change_State(0);
            break;
    }

}

static void Change_State(int Reset)
{
    int x;

    for(x=Levels_Struct[CurrentLevel].Beginning; x<IconCounter; x++)
    {
        if(Icons_Struct[x].Icon_OK)
        {
            if(Icons_Struct[x].Icon_Status != 0)
            {
                if(Reset == 0)
                {
                    if((Icons_Struct[x].Icon_Status & ICON_ACTIVE) == 0 && Icons_Struct[x].Icon_Status < 4)
                    {
                        // 11000111b
                        Icons_Struct[x].Icon_Status = (Icons_Struct[x].Icon_Status - 1) & 0xC7;
                    }
                    else
                    {
                        Icons_Struct[x].Icon_Status = (Icons_Struct[x].Icon_Status + 1) & 0xC7;
                    }
                }
                else
                {
                    Icons_Struct[x].Icon_Status = 0;
                }

                Insert_Icon((Icons_Struct[x].Icon_Status & SELECTED_ICON) ? IDS_SELECTED : IDS_NORMAL, x);
            }
        }
    }
}

static void Insert_Icon(int Tryb, int IconNumber)
{
    BltBitMapRastPort(BMP_Buffer,
        Icons_Struct[IconNumber].Icon_PositionX,
        0,
        &RP_DoubleBuffer, //Window_struct->RPort,
        0, //Icons_Struct[IconNumber].Icon_PositionX,
        0,
        Icons_Struct[IconNumber].Icon_Width + 1,
        WindowHeight,
        0xC0);

    DrawIconState(&RP_DoubleBuffer, //Window_struct->RPort,
        Icon[IconNumber],
        NULL,
        0, //Icons_Struct[IconNumber].Icon_PositionX,
        Icons_Struct[IconNumber].Icon_PositionY - MovingTable[(Icons_Struct[IconNumber].Icon_Status & 0x07)],
        Tryb,
        ICONDRAWA_Frameless, TRUE,
        ICONDRAWA_EraseBackground, FALSE,
        TAG_DONE);

    BltBitMapRastPort(BMP_DoubleBuffer,
        0,
        0,
        Window_struct->RPort,
        Icons_Struct[IconNumber].Icon_PositionX,
        0,
        Icons_Struct[IconNumber].Icon_Width + 1,
        WindowHeight,
        0xC0);
}

static void Blink_Icon(int IconNumber)
{
    int x, Tryb=IDS_SELECTED;

    for(x=0; x<8; x++)
    {
        Insert_Icon(Tryb, IconNumber);
        Delay(3);

        if(Tryb == IDS_NORMAL) Tryb = IDS_SELECTED;
        else Tryb = IDS_NORMAL;
    }
}

static BOOL OpenMainWindow(void)
{
    int x, y, a;

    if((Screen_struct=LockPubScreen(NULL)) != NULL)
    {
        BltBitMapRastPort(Screen_struct->RastPort.BitMap,
            (ScreenWidth>>1) - (WindowWidth>>1),
            ScreenHeight - WindowHeight,
            &RP_Buffer,
            0, 0,
            Window_Max_X,
            Window_Max_Y,
            0xC0);

        UnlockPubScreen(NULL,Screen_struct);
    }

    if(picture[0] && picture[1] && picture[2])
    {
        y = WindowWidth - BackgroundData[0].Width - BackgroundData[2].Width;
        a = y / BackgroundData[1].Width;

        BltBitMapRastPort(bm[0],
            0,
            0,
            &RP_Buffer,
            0,
            WindowHeight - BackgroundData[0].Height,
            BackgroundData[0].Width,
            BackgroundData[0].Height,
            0xC0);

        for(x=0; x<a; x++)
        {
            BltBitMapRastPort(bm[1],
                0,
                0,
                &RP_Buffer,
                BackgroundData[0].Width + x * BackgroundData[1].Width,
                WindowHeight - BackgroundData[1].Height,
                BackgroundData[1].Width,
                BackgroundData[1].Height,
                0xC0);
        }

        BltBitMapRastPort(bm[1],
            0,
            0,
            &RP_Buffer,
            BackgroundData[0].Width + x * BackgroundData[1].Width,
            WindowHeight - BackgroundData[1].Height,
            y - BackgroundData[1].Width * a,
            BackgroundData[1].Height,
            0xC0);

        BltBitMapRastPort(bm[2],
            0,
            0,
            &RP_Buffer,
            WindowWidth - BackgroundData[2].Width,
            WindowHeight - BackgroundData[2].Height,
            BackgroundData[2].Width,
            BackgroundData[2].Height,
            0xC0);
    } else if(picture[1]) {
        for(x=0; x<WindowWidth; x=x+BackgroundData[1].Width)
        {
            BltBitMapRastPort(bm[1],
                0,
                0,
                &RP_Buffer,
                x,
                WindowHeight - BackgroundData[1].Height,
                BackgroundData[1].Width,
                BackgroundData[1].Height,
                0xC0);
        }
    }

    if(Icon_Remap == TRUE)
    {
        D(bug("[IconBar] Iconremap is true!\n"));

        for(x=Levels_Struct[CurrentLevel].Beginning; x<IconCounter; x++)
        {
            if(Icon[x]!=NULL)
            {
                // D(bug("[Iconbar] Icon#%d-%s was not NULL, so LayoutIcon Adapt a palette-mapped icon to display\n", x, Icons_Struct[x].Icon_Path);
                LayoutIcon(Icon[x], Screen_struct, NULL);
            }
        }
    }

    D(bug("[IconBar] Opening window: S=%d, W=%d\n", WindowWidth, WindowHeight));

    if((Window_struct = OpenWindowTags(NULL,
        WA_Left, BeginningWindow,
        WA_Top, ScreenHeight - WindowHeight,
        WA_InnerWidth,  WindowWidth,
        WA_InnerHeight, WindowHeight,
        WA_ReportMouse, TRUE,
        WA_MouseQueue, 3,
        WA_Borderless, TRUE,
        WA_SizeGadget, FALSE,
        WA_Activate, ((FirstOpening && Static) ? FALSE : TRUE),
        WA_PubScreenName, NULL,
        WA_BackFill, LAYERS_NOBACKFILL,
        WA_Flags, WFLG_NOCAREREFRESH|
                    WFLG_SMART_REFRESH|
                    WFLG_RMBTRAP,
        WA_IDCMP, IDCMP_NEWSIZE|
                    IDCMP_RAWKEY|
                    IDCMP_INACTIVEWINDOW|
                    IDCMP_MOUSEMOVE|
                    IDCMP_INTUITICKS|
                    IDCMP_MOUSEBUTTONS,
        TAG_END)))
    {
        D(bug("[IconBar] Window opened\n"));

        WindowMask = 1 << Window_struct->UserPort->mp_SigBit;

        BltBitMapRastPort(BMP_Buffer,
            0, 0,
            Window_struct->RPort,
            0, 0,
            WindowWidth,
            WindowHeight,
            0xC0);

        for(x=Levels_Struct[CurrentLevel].Beginning; x<IconCounter; x++)
        {
            if(Icon[x] != NULL)
            {
                DrawIconState(Window_struct->RPort,
                    Icon[x],
                    NULL,
                    Icons_Struct[x].Icon_PositionX,
                    Icons_Struct[x].Icon_PositionY,
                    IDS_NORMAL,
                    ICONDRAWA_Frameless, TRUE,
                    ICONDRAWA_EraseBackground, FALSE,
                    TAG_DONE);
            }
        }
    } else {
        D(bug("[IconBar] Can't open main toolbar window\n"));
        return FALSE;
    }

    Window_Open = TRUE;
    Window_Active = TRUE;

    return TRUE;
}

static void CloseMainWindow(void)
{
    D(bug("[IconBar] ClosingMainWindow...\n"));

    int x;

    if(Window_struct) CloseWindow(Window_struct);

    for(x=Levels_Struct[CurrentLevel].Beginning; x<IconCounter; x++)
    {
        Icons_Struct[x].Icon_Status = 0;
    }

    Window_Open = FALSE;
    WindowMask = 0;
}

static void CheckMousePosition(void)
{
    // noisy: D(bug("[IconBar] Mouse current position: Y=%d\n", Screen_struct->MouseY));

    if((Screen_struct->MouseY == ScreenHeight - 1) &&
        Window_Open == FALSE &&
        Screen_struct->MouseX > BeginningWindow &&
        Screen_struct->MouseX < EndingWindow)
    {
        D(bug("[IconBar] Opening the window\n"));
        if(OpenMainWindow() == FALSE)
        {
            BiB_Exit = TRUE;
        }
    }

    // LJ: trying to make window active when going to very bottom of the screen
    //     even in static mode  - but it doesn't work because this code is checked
    //     only in static=0 mode

    else if ((Screen_struct->MouseY == ScreenHeight - 1) &&
        Window_Open == TRUE &&
        Screen_struct->MouseX > BeginningWindow &&
        Screen_struct->MouseX < EndingWindow)
    {
        D(bug("[IconBar] You're at the very bottom! I should make this window active!\n"));
        Window_Active = TRUE;
    }
}

static void Show_Selected_Level(void)
{
    CloseMainWindow();

    IconCounter   = Levels_Struct[CurrentLevel+1].Beginning;
    WindowWidth = Levels_Struct[CurrentLevel].WindowPos_X;
    WindowHeight  = Levels_Struct[CurrentLevel].WindowPos_Y;
    BeginningWindow = (ScreenWidth>>1) - (WindowWidth>>1);
    EndingWindow   = (ScreenWidth>>1) + (WindowWidth>>1);

    //LJ: delay needed for AROS, otherwise garbage from old icons remains
    Delay(10); //10 ms seems to be enough

    if(OpenMainWindow() == FALSE)
    {
        BiB_Exit = TRUE;
    }
}

static void OpenMenuWindow(void)
{
    int x;

    if((MenuWindow_struct = OpenWindowTags(NULL,
            WA_Left, Screen_struct->MouseX,
            WA_Top, Screen_struct->MouseY,
            WA_InnerWidth, Lenght + 1,
            WA_InnerHeight, LevelCounter << 4,
            WA_AutoAdjust, TRUE,
            WA_ReportMouse, TRUE,
            WA_Borderless, TRUE,
            WA_IDCMP,IDCMP_MOUSEBUTTONS|
                    IDCMP_MOUSEMOVE|
                    IDCMP_INACTIVEWINDOW,
            WA_Flags,WFLG_ACTIVATE|
                    WFLG_RMBTRAP|
                    WFLG_SMART_REFRESH,
            TAG_END)))
    {
        MenuMask = 1 << MenuWindow_struct->UserPort->mp_SigBit;

        SetAPen(MenuWindow_struct->RPort, 1);
        Move(MenuWindow_struct->RPort, Lenght, 0);
        Draw(MenuWindow_struct->RPort, Lenght, LevelCounter << 4);
        Draw(MenuWindow_struct->RPort, 0, LevelCounter << 4);
        SetAPen(MenuWindow_struct->RPort, 2);
        Draw(MenuWindow_struct->RPort, 0, 0);
        Draw(MenuWindow_struct->RPort, Lenght, 0);

        SetAPen(MenuWindow_struct->RPort, 1);
        Move(MenuWindow_struct->RPort, 0, (LevelCounter - 1) << 4);
        Draw(MenuWindow_struct->RPort, Lenght - 1, (LevelCounter - 1) << 4);

        SetAPen(MenuWindow_struct->RPort, 2);
        Move(MenuWindow_struct->RPort, 0, ((LevelCounter - 1) << 4) + 1);
        Draw(MenuWindow_struct->RPort, Lenght - 1, ((LevelCounter - 1) << 4) + 1);

        for(x=0; x<LevelCounter; x++)
        {
            if(x == CurrentLevel) Names.FrontPen = 2;
            else Names.FrontPen = 1;

            strcpy(BufferList, Levels_Struct[x].Level_Name);

            PrintIText(MenuWindow_struct->RPort, &Names, 5, (x << 4) + 5);

            D(bug("[IconBar] Level %d: %s\n", x, Levels_Struct[x].Level_Name));
        }
        MenuWindow_Open = TRUE;
    }
    else D(bug("[IconBar] Can't open menu\n"));
}

static void CloseMenuWindow(void)
{
    CloseWindow(MenuWindow_struct);
    MenuMask = 0;

    if(PositionMenuOK == TRUE)
    {
        if(Position == (LevelCounter - 1))
        {
            Settings();
        } else {
            CurrentLevel = Position;
            Show_Selected_Level();
        }
    }
}

static void Decode_IDCMP2(struct IntuiMessage *KomIDCMP)
{
    int x, kolA, kolB, dluG, x4;

    dluG = Lenght - 2;

    switch(KomIDCMP->Class)
    {
        case IDCMP_MOUSEMOVE:
            if(MenuWindow_struct->MouseX > 0 &&
                MenuWindow_struct->MouseX < Lenght &&
                MenuWindow_struct->MouseY >0)
            {
                Position = MenuWindow_struct->MouseY >> 4;
            }
            else
            {
                Position = 100;
            }

            if(Position != OldPosition)
            {
                for(x=0; x<LevelCounter; x++)
                {
                    x4 = (x << 4) + 2;

                    if(x == Position)
                    {
                        kolA = 2;
                        kolB = 1;
                    }
                    else
                    {
                        kolA = 0;
                        kolB = 0;
                    }

                    SetAPen(MenuWindow_struct->RPort, kolA);
                    Move(MenuWindow_struct->RPort, dluG, x4);
                    Draw(MenuWindow_struct->RPort, dluG, x4 + 13);
                    Draw(MenuWindow_struct->RPort, 2, x4 + 13);
                    SetAPen(MenuWindow_struct->RPort, kolB);
                    Draw(MenuWindow_struct->RPort, 2, x4);
                    Draw(MenuWindow_struct->RPort, dluG, x4);
                }

                if(Position != CurrentLevel && Position != 100)
                    PositionMenuOK = TRUE;
                else PositionMenuOK = FALSE;

                OldPosition = Position;

                D(bug("[IconBar] Menu %d, level %d\n", Position, CurrentLevel));
            }
            break;

        case IDCMP_MOUSEBUTTONS:  //LJ: Adding IDCMP_ seems to do the trick...
            switch (KomIDCMP->Code)
            {
                case MENUUP:
                    MenuWindow_Open = FALSE;
                    rbm = 0;
                    break;
            }
            break;

        case IDCMP_INACTIVEWINDOW:
            MenuWindow_Open = FALSE;
            break;
    }
}

static void Launch_Program(char *Program)
{
    BPTR oldlock;

    // Get current directory lock
    oldlock = CurrentDir(BNULL);

    D(bug("[IconBar] Launch_Program %s\n", Program));
    OpenWorkbenchObject(Program, TAG_DONE);

    // Go back to old directory
    CurrentDir(oldlock);
}

static void Settings(void)
{
    if (!OpenWorkbenchObject("PROGDIR:BIBPrefs", TAG_DONE))
        OpenWorkbenchObject("SYS:Prefs/BoingIconBar", TAG_DONE);
}

static void Reload_BiB(void)
{
    D(bug("[IconBar] Reloading BiB...\n"));

    int x;

    if (Window_Open == TRUE)
    {
        D(bug("[IconBar] The Main window is open.. so let's close it...\n"));
        CloseMainWindow();
    }

    for(x=0; x<SUM_ICON; x++)
    {
        if(Icon[x] != NULL)
        {
            FreeDiskObject(Icon[x]);
        }
        Icons_Struct[x].Icon_Height  = 0;
        Icons_Struct[x].Icon_Width = 0;
        Icons_Struct[x].Icon_PositionX  = 0;
        Icons_Struct[x].Icon_PositionY  = 0;
    }

    if(BMP_Buffer) FreeBitMap(BMP_Buffer);

    if(BMP_DoubleBuffer) FreeBitMap(BMP_DoubleBuffer);

    if(ReadPrefs() == FALSE)
    {
        D(bug("[IconBar] Prefs error\n"));
        return;
    }

    if(Static)
    {
        Delay(Static * 50);
        OpenMainWindow();
        FirstOpening = FALSE;
    }            
}

void HandleCx(void)
{
    D(bug("[IconBar] Cx Signal received\n"));
    CxMsg *msg;
    ULONG msgid, msgtype;
    while((msg = (CxMsg *)GetMsg(BrokerMP)) != NULL)
    {
        msgid = CxMsgID(msg);
        msgtype = CxMsgType(msg);
        ReplyMsg((struct Message *)msg);

        switch(msgtype)
        {
            case CXM_COMMAND:
                switch(msgid)
                {
                    case CXCMD_DISABLE:
                        D(bug("[IconBar] CXCMD_DISABLE\n"));
                        ActivateCxObj(broker, 0L);
                        break;
                    case CXCMD_ENABLE:
                        D(bug("[IconBar] CXCMD_ENABLE\n"));
                        ActivateCxObj(broker, 1L);
                        break;
                    case CXCMD_KILL:
                        D(bug("[IconBar] CXCMD_KILL\n"));
                        BiB_Exit = TRUE;
                        break;
                    case CXCMD_UNIQUE:
                        D(bug("[IconBar] CXCMD_UNIQUE\n"));
                        BiB_Exit = TRUE;
                        break;
                    default:
                        D(bug("[IconBar] Unknown msgid\n"));
                        break;
                }
                break;
            default:
                D(bug("[IconBar] Unknown msgtype\n"));
                break;
        }
    }
}

static void CleanExit(CONST_STRPTR msg)
{
    LONG retval = RETURN_OK;
    int x;

    if (msg)
    {
        PutStr(msg);
        retval = RETURN_FAIL;
    }

    CloseMainWindow();

    for(x=0; x<SUM_ICON; x++)
    {
        if(Icon[x] != NULL)
        {
            FreeDiskObject(Icon[x]);
        }
    }

    if(BMP_Buffer)   FreeBitMap(BMP_Buffer);
    if(BMP_DoubleBuffer) FreeBitMap(BMP_DoubleBuffer);

    for(x=0; x<3; x++)
    {
        if(picture[x]) DisposeDTObject(picture[x]);
    }

    // Close notification of prefs change
    if(notification)
        EndNotify(nr);
    if(nr)
        FreeMem(nr, sizeof(struct NotifyRequest));
    if(BIBport)
        DeleteMsgPort(BIBport);

    if (broker) DeleteCxObjAll(broker);
    if (BrokerMP)
    {
        CxMsg *msg;
        while ((msg = (CxMsg *)GetMsg(BrokerMP)) != NULL)
            ReplyMsg((struct Message *)msg);
        DeleteMsgPort(BrokerMP);
    }
    if (rda) FreeArgs(rda);
    if (dob) FreeDiskObject(dob);
    exit(retval);
}

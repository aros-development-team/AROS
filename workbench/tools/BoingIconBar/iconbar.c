// News: Labeling Icons
// Version of BiB based on 1.0.2 68K 26.10.2013 source
// New features added by Phibrizzo 

//////////////////////////////////////////////////////////////
//                                                          //
// AROS PORT by LuKeJerry (at) gmail.com                    //
//   -- translated to ENG from 26-09-2011                   //
//                                                          //
//////////////////////////////////////////////////////////////


#include <aros/debug.h>

#include <proto/workbench.h>
#include <proto/icon.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/diskfont.h> 

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/exec.h>
#include <workbench/icon.h>
#include <workbench/startup.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <dos/dos.h>
#include <devices/rawkeycodes.h>
#include <aros/detach.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "locale.h"
#include "iconbar.h"

// ------------------------------
// Forward declarations ....
static BOOL ReadPrefs(void);                                            // load prefs
static void LoadBackground(void);                                       //load background pictures
static BOOL SetWindowParameters(void);                                  // check window sizes
static void Decode_Toolbar_IDCMP(struct IntuiMessage *KomIDCMP);        // decode IDCMP main signals
static void Change_State(LONG Mode);                                    // change icon state
static void Insert_Icon(LONG Tryb, LONG NrIcon);                        // draw icon
static void Blink_Icon(LONG NrIcon);                                    // blink the icon
static BOOL OpenMainWindow(void);                                       // open main window
static void CloseMainWindow(void);                                      // close main window
static void CheckMousePosition(void);                                   // check mouse position
static void Show_Selected_Level(void);                                  // change the submenu
static void OpenMenuWindow(void);                                       // open menu window
static void CloseMenuWindow(void);                                      // close menu window
static void Decode_Menu_IDCMP(struct IntuiMessage *KomIDCMP);           // decode IDCMP menu signals
static void Launch_Program(STRPTR Program);                             // start the chosed program
static void Settings(void);                                             // open the prefs program
static void Reload(void);                                               // reload the BiB
static void IconLabel(void);                                            // add label to icon

// ------------------------------
// App data...
 
#define SUM_ICON  200
#define ICON_ACTIVE (1<<7)
#define SELECTED_ICON (1<<6)

#define TEMPLATE "SPACE/N/K,STATIC/N/K,AUTOREMAP/S,NAMES/S,SYSFONT/S,LABELFONT/S,FONTSIZE/N/K"

// ------------------------------

enum {
    ARG_SPACE,
    ARG_STATIC,
    ARG_AUTOREMAP,
    ARG_NAMES,
    ARG_SYSFONT,
    ARG_LABELFONT,
    ARG_FONTSIZE,
    ARG_TOTAL
};

#define BIB_PREFS "ENV:Iconbar.prefs"

const TEXT version[]="$VER: BoingIconBar 1.11 (15.12.2020) by Robert 'Phibrizzo' Krajcarz - AROS port by LuKeJerry";

static BOOL                                     BiB_Exit=FALSE, Icon_Remap=FALSE, PositionMenuOK=FALSE; 
static BOOL                                     Window_Active=FALSE, Window_Open=FALSE, MenuWindow_Open=FALSE, FirstOpening=TRUE;

static BOOL                                     B_Labels=FALSE;
static TEXT                                     IT_Labels[100];  // buffer for label

// -----------
static char                                     *argSysFont = NULL, *argLabelFont = NULL;

static LONG                                     WindowHeight, WindowWidth, ScreenHeight, ScreenWidth, IconWidth;
static LONG                                     Spacing=5, Static=0, FontSize = 0, Position, OldPosition; 
static LONG                                     IconCounter, LevelCounter, CurrentLevel=0, lbm=0, rbm=0, MouseIcon;
static LONG                                     Length, BeginningWindow, EndingWindow, Window_Max_X, Window_Max_Y;
static BYTE                                     MovingTable[8]={0, 4, 7, 9, 10, 9, 7, 4};
static TEXT                                     BufferList[20]; 
static ULONG                                    WindowMask=0, MenuMask=0, WindowSignal;

static IPTR                                     args[ARG_TOTAL] = {
                        (IPTR)&Spacing,
                        (IPTR)&Static,
                        0,
                        0,
                        0,
                        0,
                        (IPTR)&FontSize
};

static struct DiskObject                        *Icon[SUM_ICON]; 

static struct Window                            *MainWindow, *MenuWindow; 
static struct Screen                            *MyScreen;

static struct BitMap                            *BMP_Buffer, *BMP_DoubleBuffer;
static struct RastPort                          RP_Buffer, RP_DoubleBuffer;

// struct of  icons
static struct Icon_Struct                       Icons[SUM_ICON];

// struct of submenu
static struct Level_Struct                      Levels[11];

// ---  Define used fonts
static struct TextFont                          *TF_LabelFont   = NULL, *TF_SysFont = NULL;
static const char                               labelfontName[] = "arial.font";
static const char                               sysfontName[]   = "topaz.font";
static struct TextAttr                          LabelFont       = { (STRPTR)labelfontName,      9, 0, FPF_DISKFONT };
static struct TextAttr                          SysFont         = { (STRPTR)sysfontName,        8, 0, FPF_ROMFONT };

static struct IntuiText                         Names  = { 1, 0, JAM1, 0, 0, &SysFont,   BufferList, NULL };
static struct IntuiText                         Labels = { 1, 0, JAM1, 0, 0, &LabelFont, IT_Labels,  NULL };

//  background pictures
static Object *picture[3];
static struct BitMap                            *bm[3];

static struct Struct_BackgroundData             BackgroundData[3];

// -------------

int main(int argc, char *argv[])
{
    LONG retval = RETURN_OK;

    LONG x;

    struct IntuiMessage *KomIDCMP,CopyIDCMP;
    struct RDArgs *rda=NULL;
    struct DiskObject *dob=NULL;
    struct NotifyRequest *NotRequest = NULL;
    struct MsgPort *BIBport = NULL;

    if (argc) // reading command line parameters 
    {
        if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
        {
            PrintFault(IoErr(), argv[0]);
            retval = RETURN_ERROR;
            goto bailout;
        }

        if (args[ARG_AUTOREMAP])
        {
            Icon_Remap = TRUE;
        }

        if (args[ARG_NAMES])
        {
            B_Labels = TRUE;
        }

        if (args[ARG_SPACE])
        {
            Spacing = *(LONG*)args[ARG_SPACE];
        }

        if (args[ARG_STATIC])
        {
            Static = *(LONG*)args[ARG_STATIC];
        }

        if (args[ARG_SYSFONT])
        {
            argSysFont = AllocVec(strlen((char *)args[ARG_SYSFONT]) + 1, MEMF_CLEAR);
            CopyMem((APTR)args[ARG_SYSFONT], argSysFont, strlen((char *)args[ARG_SYSFONT]));
        }

        if (args[ARG_LABELFONT])
        {
            argLabelFont = AllocVec(strlen((char *)args[ARG_LABELFONT]) + 1, MEMF_CLEAR);
            CopyMem((APTR)args[ARG_LABELFONT], argLabelFont, strlen((char *)args[ARG_LABELFONT]));
        }

        if (args[ARG_FONTSIZE])
        {
            FontSize = *(LONG*)args[ARG_FONTSIZE];
        }
    }
    else
    {   //reading ToolTypes parameters 
        struct WBArg *wba;
        struct WBStartup *wbs=(struct WBStartup*)argv;
        BPTR oldcd;

        if (wbs && wbs->sm_NumArgs > 0)
        {
            wba = &wbs->sm_ArgList[0];
            if (wba && wba->wa_Lock && wba->wa_Name)
            {
                oldcd=CurrentDir(wba->wa_Lock);
                if ((dob=GetDiskObjectNew(wba->wa_Name)))
                {
                    TEXT *str;

                    if ((str = FindToolType(dob->do_ToolTypes, "SPACE")))
                        Spacing = atoi(str);

                    if ((str = FindToolType(dob->do_ToolTypes, "STATIC")))
                        Static = atoi(str);

                    if ((str = FindToolType(dob->do_ToolTypes, "AUTOREMAP")))
                        Icon_Remap = TRUE;

                    if ((str = FindToolType(dob->do_ToolTypes, "NAMES")))
                        B_Labels = TRUE;

                    if ((str = FindToolType(dob->do_ToolTypes, "SYSFONT")))
                    {
                        argSysFont = AllocVec(strlen(str) + 1, MEMF_CLEAR);
                        CopyMem(str, argSysFont, strlen(str));
                    }

                    if ((str = FindToolType(dob->do_ToolTypes, "LABELFONT")))
                    {
                        argLabelFont = AllocVec(strlen(str) + 1, MEMF_CLEAR);
                        CopyMem(str, argLabelFont, strlen(str));
                    }

                    if ((str = FindToolType(dob->do_ToolTypes, "FONTSIZE")))
                        FontSize = atoi(str);
                }
                CurrentDir(oldcd);
            }
        }
    }

    if (rda)
    {
        FreeArgs(rda);
        rda = NULL;
    }
    if (dob)
    {
        FreeDiskObject(dob);
        dob = NULL;
    }

    Detach(); // must be done after ReadArgs()

    D(
        bug("[IconBar] space %d\n", Spacing);
        bug("[IconBar] static %d\n", Static);
        bug("[IconBar] autoremap %d\n", Icon_Remap);
        bug("[IconBar] names %d\n", B_Labels);
        bug("[IconBar] sysfont 0x%p\n", argSysFont);
        bug("[IconBar] labelfont 0x%p\n", argLabelFont);
        bug("[IconBar] fontsizes %d\n", FontSize);
    )

    // start notification on prefs file
    BIBport = CreateMsgPort();
    if (BIBport)
    {
        NotRequest = AllocVec(sizeof(struct NotifyRequest), MEMF_CLEAR);
        if (NotRequest)
        {
            NotRequest->nr_Name = BIB_PREFS;
            NotRequest->nr_Flags = NRF_SEND_MESSAGE;
            NotRequest->nr_stuff.nr_Msg.nr_Port = BIBport;

            if (StartNotify(NotRequest) == DOSFALSE)
            {
                printf("StartNotify failed: %ld\n", IoErr());
                retval = RETURN_ERROR;
                goto bailout;
            }
        }
        else
        {
            puts("Can't allocate NotifyRequest");
            retval = RETURN_ERROR;
            goto bailout;
        }
    }
    else
    {
        puts("Can't create MsgPort for notification");
        retval = RETURN_ERROR;
        goto bailout;
    }

    // ------ Opening font if parameter NAMES is active

    if (FontSize)
    {
        LabelFont.ta_YSize = FontSize;
#if (0)
        SysFont.ta_YSize = FontSize - 1;
#endif
    }

    if (argLabelFont)
    {
        LabelFont.ta_Name = argLabelFont;
    }

    if (argSysFont)
    {
        SysFont.ta_Name = argSysFont;
        if((TF_SysFont = OpenDiskFont(&SysFont)) == NULL)
        {
            printf("Failed to open %s:%u font - Resorting to %s:%u\n", SysFont.ta_Name, SysFont.ta_YSize, sysfontName, 8);
            SysFont.ta_Name = (char *)sysfontName;
        }
    }

    if(B_Labels == TRUE)
    {
        if((TF_LabelFont = OpenDiskFont(&LabelFont)) == NULL)
        {
            B_Labels = FALSE;
            printf("Failed to open %s:%u - Labelling disabled\n", LabelFont.ta_Name, LabelFont.ta_YSize);
        }
    }

    // ------------------------------

    if(ReadPrefs())
    {
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

            if (GetMsg(BIBport) != NULL)
            {
                Reload();
            }


            if(Window_Open || MenuWindow_Open)
            {
                WindowSignal = Wait(WindowMask | MenuMask);

                if(WindowSignal & WindowMask)
                {
                    while((KomIDCMP=GT_GetIMsg(MainWindow->UserPort)))
                    {
                        CopyMem(KomIDCMP,&CopyIDCMP,sizeof(struct IntuiMessage));
                        GT_ReplyIMsg(KomIDCMP);
                        Decode_Toolbar_IDCMP(&CopyIDCMP);
                    }

                    if(!(Static) && Window_Active == FALSE)
                        CloseMainWindow();
                }

                if(WindowSignal & MenuMask)
                {
                    while((KomIDCMP=GT_GetIMsg(MenuWindow->UserPort)))
                    {
                        CopyMem(KomIDCMP,&CopyIDCMP,sizeof(struct IntuiMessage));
                        GT_ReplyIMsg(KomIDCMP);
                        Decode_Menu_IDCMP(&CopyIDCMP);
                    }

                    if(MenuWindow_Open == FALSE)
                        CloseMenuWindow();
                }
            }
            else
            {
                Delay(50);
                CheckMousePosition();
            }
        }
        // ---- end of main loop
    }
    else
    {
        printf("No prefs\n");
        retval = RETURN_ERROR;
    }
    
bailout:
    CloseMainWindow();

    EndNotify(NotRequest); // replies all pending messages
    FreeVec(NotRequest);

    if (BIBport)
        DeleteMsgPort(BIBport);
    
    for(x=0; x<SUM_ICON; x++)
    {
        if(Icon[x] != NULL)
        {
            FreeDiskObject(Icon[x]);
        }
    }

    if(BMP_Buffer)
        FreeBitMap(BMP_Buffer);
    if(BMP_DoubleBuffer)
        FreeBitMap(BMP_DoubleBuffer);

    for(x=0; x<3; x++)
    {
        if(picture[x])
            DisposeDTObject(picture[x]);
    }

    if (TF_SysFont)
        CloseFont(TF_SysFont);
    if(TF_LabelFont)
        CloseFont(TF_LabelFont);
    if (rda)
        FreeArgs(rda);
    if (dob)
        FreeDiskObject(dob);

    FreeVec(argSysFont);
    FreeVec(argLabelFont);

    return retval;
}


static BOOL ReadPrefs(void)
{
    BPTR Prefs;
    LONG x, LengthText, NumberCharacters;

    Icons[0].Icon_OK = FALSE;
    IconCounter = 0;
    LevelCounter = 0;
    Length = 0;

    if((Prefs = Open(BIB_PREFS, MODE_OLDFILE)))
    {
        FGets(Prefs, Icons[0].Icon_Path, 255);

        if(strncmp(Icons[0].Icon_Path, "BOING_PREFS", 11) != 0)
        {
            Close(Prefs);
            puts("Prefs error\n");
            return FALSE;
        }

        if((MyScreen=LockPubScreen(NULL)))
        {
            while(FGets(Prefs, Icons[IconCounter].Icon_Path, 255) )    //&& IconCounter < SUM_ICON)
            {
                NumberCharacters = strlen(Icons[IconCounter].Icon_Path);
                Icons[IconCounter].Icon_Path[NumberCharacters-1] = '\0';
                if(Icons[IconCounter].Icon_Path[0] == ';')
                {
                    if(NumberCharacters > 20)
                        NumberCharacters = 20;
                    Icons[IconCounter].Icon_Path[19] = '\0';

                    for(x=1; x<NumberCharacters; x++)
                    {
                        Levels[LevelCounter].Level_Name[x-1] = Icons[IconCounter].Icon_Path[x];
                    }

                    strcpy(BufferList, Levels[LevelCounter].Level_Name);
                    LengthText = IntuiTextLength(&Names);
                    if(LengthText > Length)
                        Length = LengthText;

                    Levels[LevelCounter].Beginning = IconCounter;
                    LevelCounter++;
                }
                else
                {
                    if((Icon[IconCounter] = GetIconTags(Icons[IconCounter].Icon_Path,
                        ICONGETA_RemapIcon, FALSE,
                        TAG_DONE)))
                    {
                        IconControl(Icon[IconCounter],
                            ICONCTRLA_GetWidth, &Icons[IconCounter].Icon_Width,
                            ICONCTRLA_GetHeight, &Icons[IconCounter].Icon_Height,
                            ICONCTRLA_SetNewIconsSupport, TRUE,
                            TAG_END);

                        if(Icons[IconCounter].Icon_Width == 0)
                        {
                            Icons[IconCounter].Icon_Width = Icon[IconCounter]->do_Gadget.Width;
                        }

                        if(Icons[IconCounter].Icon_Height == 0)
                        {
                            Icons[IconCounter].Icon_Height = Icon[IconCounter]->do_Gadget.Height;
                        }

                        LayoutIcon(Icon[IconCounter], MyScreen, NULL);

                        // ---------------------- Extract Label from path to icon

                        Icons[IconCounter].IK_Label = FilePart((Icons[IconCounter].Icon_Path));

                        // --------------------------------------------------

                        Icons[IconCounter].Icon_OK = TRUE;
                        IconCounter++;

                        if(IconCounter == SUM_ICON)
                            break;
                    }
                    else
                        printf("No icon %s.\n", Icons[IconCounter].Icon_Path);
                }
            }
            Close(Prefs);

            ScreenWidth = MyScreen->Width;
            ScreenHeight  = MyScreen->Height;

            UnlockPubScreen(NULL, MyScreen);

            Levels[LevelCounter].Beginning = IconCounter;
            if (SetWindowParameters() == FALSE)
            {
                puts("Failed to set window parameters");
                return FALSE;
            }

            IconCounter = Levels[1].Beginning;
            WindowWidth = Levels[0].WindowPos_X;
            WindowHeight = Levels[0].WindowPos_Y;
            BeginningWindow = ScreenWidth / 2 - WindowWidth / 2;
            EndingWindow = ScreenWidth / 2 + WindowWidth / 2;
            CurrentLevel= 0;

            // add Settings menu entry
            sprintf(Levels[LevelCounter].Level_Name, _(MSG_MENU_SETTINGS));
            sprintf(Names.IText, "%s", Levels[LevelCounter].Level_Name);
            LengthText = IntuiTextLength(&Names);
            if(LengthText > Length)
                Length = LengthText;
            LevelCounter++;

            // add Quit menu entry
            sprintf(Levels[LevelCounter].Level_Name, _(MSG_MENU_QUIT));
            sprintf(Names.IText, "%s", Levels[LevelCounter].Level_Name);
            LengthText = IntuiTextLength(&Names);
            if(LengthText > Length)
                Length = LengthText;
            LevelCounter++;

            Length += 10;
        }
        else
            puts("Can't lock public screen");
    }
    return Icons[0].Icon_OK;
}


static void LoadBackground(void)
{
    LONG x;

    STRPTR names[3] = {"Images:bibgfx/left",
        "Images:bibgfx/middle",
        "Images:bibgfx/right"};

    for(x=0; x<3; x++)
    {
        picture[x] = NewDTObject (names[x],
            DTA_GroupID, GID_PICTURE,
            PDTA_Remap, TRUE,
            PDTA_DestMode, PMODE_V43,
            OBP_Precision, PRECISION_IMAGE,
            PDTA_Screen, (IPTR)MyScreen,
            TAG_END);

        if(picture[x])
        {
            DoDTMethod (picture[x], NULL, NULL, DTM_PROCLAYOUT, NULL, DTSIF_NEWSIZE);

            GetDTAttrs (picture[x],
                PDTA_DestBitMap,  (IPTR)&bm[x],
                DTA_NominalHoriz, (IPTR)&BackgroundData[x].Width,
                DTA_NominalVert,  (IPTR)&BackgroundData[x].Height,
                TAG_END);
        }
    }
}


static BOOL SetWindowParameters(void)
{
    LONG x, y, z;

    Window_Max_X=0;
    Window_Max_Y=0;

    Icons[0].Icon_PositionX = 0;

    for(y=0; y<(LevelCounter); y++)
    {
        WindowHeight  = 0;
        WindowWidth = 0;

        IconCounter = Levels[y+1].Beginning;

        for(x=Levels[y].Beginning; x<IconCounter; x++)
        {
            if(Icons[x].Icon_OK)
            {
                if(Icons[x].Icon_Height > WindowHeight)
                {
                    WindowHeight = Icons[x].Icon_Height;
                }

                if(Icons[x].Icon_Width > IconWidth)
                {
                    IconWidth = Icons[x].Icon_Width;
                }

                // ------------- Calculate lenght of Label

                strcpy(IT_Labels, Icons[x].IK_Label);

                for(z=strlen(Icons[x].IK_Label); z>1; z--)
                {
                    IT_Labels[z - 1] = '\0';

                    if(IntuiTextLength(&Labels) < Icons[x].Icon_Width)
                    {
                        Icons[x].IK_Label_Length = z;
                        break;
                    }
                }

                // ----------------------------------------

                if((WindowWidth + Icons[x].Icon_Width + 35) > ScreenWidth)
                {
                    IconCounter = x;
                    Levels[y+1].Beginning = x;
                    break;
                }

                WindowWidth = WindowWidth + Icons[x].Icon_Width + Spacing;
                Icons[x].Icon_PositionY = 10;

                if((x+1) != IconCounter)
                {
                    Icons[x+1].Icon_PositionX = WindowWidth;
                }
            }
            else
                break;
        }

        for(x=Levels[y].Beginning; x<IconCounter; x++)
        {
            if(Icons[x].Icon_OK)
            {
                Icons[x].Icon_PositionX = Icons[x].Icon_PositionX + 15; 
                Icons[x].Icon_PositionY = WindowHeight / 2 - Icons[x].Icon_Height / 2 + 15;
            }
        }

        Levels[y].WindowPos_X = WindowWidth + 30;
        Levels[y].WindowPos_Y = WindowHeight  + 20;

        // ------------- Up the icon if labeling is ON

        if(B_Labels == TRUE)
        {
            Levels[y].WindowPos_Y =  Levels[y].WindowPos_Y + 10;
        }

        // ----------------------------------

        if(Levels[y].WindowPos_X > ScreenWidth)
            Levels[y].WindowPos_X = WindowWidth;

        if(Window_Max_X < Levels[y].WindowPos_X)
            Window_Max_X = Levels[y].WindowPos_X;

        if(Window_Max_Y < Levels[y].WindowPos_Y)
            Window_Max_Y = Levels[y].WindowPos_Y;
    }

    if((MyScreen=LockPubScreen(NULL)))
    {
        BMP_Buffer = AllocBitMap(Window_Max_X,
            Window_Max_Y,
            GetBitMapAttr(MyScreen->RastPort.BitMap,
            BMA_DEPTH),
            BMF_MINPLANES|BMF_CLEAR,
            MyScreen->RastPort.BitMap);

        if (BMP_Buffer == NULL)
        {
            return FALSE;
        }

        InitRastPort(&RP_Buffer);
        RP_Buffer.BitMap = BMP_Buffer;
        RP_Buffer.Layer = NULL;

        BMP_DoubleBuffer = AllocBitMap(IconWidth + 8,
            Window_Max_Y,
            GetBitMapAttr(MyScreen->RastPort.BitMap,
            BMA_DEPTH),
            BMF_MINPLANES|BMF_CLEAR,
            MyScreen->RastPort.BitMap);
        if (BMP_DoubleBuffer == NULL)
        {
            return FALSE;
        }

        InitRastPort(&RP_DoubleBuffer);
        RP_DoubleBuffer.BitMap = BMP_DoubleBuffer;
        RP_DoubleBuffer.Layer = NULL;

        UnlockPubScreen(NULL,MyScreen);
    }
    return TRUE;
}


static void Decode_Toolbar_IDCMP(struct IntuiMessage *KomIDCMP)
{
    LONG x;

    switch(KomIDCMP->Class)
    {
        case IDCMP_CLOSEWINDOW:
            BiB_Exit = TRUE;
            break;

        case IDCMP_MOUSEMOVE:
            for(MouseIcon=Levels[CurrentLevel].Beginning; MouseIcon<IconCounter; MouseIcon++)
            {
                if(Icons[MouseIcon].Icon_OK)
                {

                    if((ScreenHeight - MyScreen->MouseY) > WindowHeight)
                    {
                        Window_Active = FALSE;
                    }

                    if(KomIDCMP->MouseX > Icons[MouseIcon].Icon_PositionX &&
                        KomIDCMP->MouseX < Icons[MouseIcon].Icon_PositionX + Icons[MouseIcon].Icon_Width &&
                        KomIDCMP->MouseY > 0 &&
                        KomIDCMP->MouseY < WindowHeight - 5)
                    {
                        if((Icons[MouseIcon].Icon_Status & ICON_ACTIVE) != ICON_ACTIVE)
                        {
                            for(x=Levels[CurrentLevel].Beginning; x<IconCounter; x++)
                            {
                                Icons[x].Icon_Status = Icons[x].Icon_Status & 0x07;
                            }

                            Icons[MouseIcon].Icon_Status = ICON_ACTIVE | (SELECTED_ICON * lbm);
                        }
                        break;
                    }
                    Icons[MouseIcon].Icon_Status = Icons[MouseIcon].Icon_Status & 0x07;
                }
            }
            break;


        case IDCMP_MOUSEBUTTONS:
            switch (KomIDCMP->Code)
            {
                case SELECTDOWN:
                    Icons[MouseIcon].Icon_Status = Icons[MouseIcon].Icon_Status | SELECTED_ICON;
                    lbm = 1;

                    break;

                case SELECTUP:
                    lbm = 0;

                    if((Icons[MouseIcon].Icon_Status & (SELECTED_ICON | ICON_ACTIVE)) == (SELECTED_ICON | ICON_ACTIVE))
                    {
                        Blink_Icon(MouseIcon);
                        Launch_Program(Icons[MouseIcon].Icon_Path);
                    }

                    Icons[MouseIcon].Icon_Status = Icons[MouseIcon].Icon_Status & 0xBF; //10001111b

                    break;

                case MENUDOWN:
                    rbm = 1;
                    OpenMenuWindow();
                    break;

            }
            break;

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
                                MyScreen,
                                NULL);
                        }
                    }

                    for(x=Levels[CurrentLevel].Beginning; x<IconCounter; x++)
                    {
                        if(Icon[x] != NULL)
                        {
                            DrawIconState(MainWindow->RPort,
                                Icon[x],
                                NULL,
                                Icons[x].Icon_PositionX,
                                Icons[x].Icon_PositionY,
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
                    if(CurrentLevel == (LevelCounter - 2 ))
                        CurrentLevel = 0;
                    Show_Selected_Level();
                    break;

                case RAWKEY_F3:
                case RAWKEY_NM_WHEEL_UP:
                    CurrentLevel--;
                    if(CurrentLevel < 0)
                        CurrentLevel = LevelCounter - 3;
                    Show_Selected_Level();
                    break; 

                case RAWKEY_F4:
                    Settings();
                    break;
            }
            break;

        case IDCMP_INACTIVEWINDOW:
            if(rbm == 0)
                Window_Active = FALSE;
            lbm = 0;

            if(Static)
            {
                for(x=Levels[CurrentLevel].Beginning; x<IconCounter; x++)
                {
                    if(Icons[x].Icon_Status != 0)
                    {
                        Icons[x].Icon_Status = 0;
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


static void Change_State(LONG Mode)
{
    LONG x;

    for(x=Levels[CurrentLevel].Beginning; x<IconCounter; x++)
    {
        if(Icons[x].Icon_OK)
        {
            if(Icons[x].Icon_Status != 0)
            {
                if(Mode == 0)
                {
                    if((Icons[x].Icon_Status & ICON_ACTIVE) == 0 && Icons[x].Icon_Status < 4)
                    {
                        Icons[x].Icon_Status = (Icons[x].Icon_Status - 1) & 0xC7; // 11000111b
                    }
                    else
                    {
                        Icons[x].Icon_Status = (Icons[x].Icon_Status + 1) & 0xC7;
                    }
                }
                else
                {
                    Icons[x].Icon_Status = 0;
                }

                Insert_Icon((Icons[x].Icon_Status & SELECTED_ICON) ? IDS_SELECTED : IDS_NORMAL, x);
            }
        }
    }
}


static void Insert_Icon(LONG Mode, LONG NrIcon)
{
    BltBitMapRastPort(BMP_Buffer,
        Icons[NrIcon].Icon_PositionX,
        0,
        &RP_DoubleBuffer,
        0,
        0,
        Icons[NrIcon].Icon_Width + 1,
        WindowHeight,
        0xC0);

    DrawIconState(&RP_DoubleBuffer,
        Icon[NrIcon],
        NULL,
        0,
        Icons[NrIcon].Icon_PositionY - MovingTable[(Icons[NrIcon].Icon_Status & 0x07)],
        Mode,
        ICONDRAWA_Frameless, TRUE,
        ICONDRAWA_EraseBackground, FALSE,
        TAG_DONE);

    BltBitMapRastPort(BMP_DoubleBuffer,
        0,
        0,
        MainWindow->RPort,
        Icons[NrIcon].Icon_PositionX,
        0,
        Icons[NrIcon].Icon_Width + 1,
        WindowHeight,
        0xC0);
}


static void Blink_Icon(LONG NrIcon)
{
    LONG x, Mode=IDS_SELECTED;

    for(x=0; x<8; x++)
    {
        Insert_Icon(Mode, NrIcon);
        Delay(3);

        if(Mode == IDS_NORMAL)
            Mode = IDS_SELECTED;
        else
            Mode = IDS_NORMAL;
    }
}


static BOOL OpenMainWindow(void)
{
    LONG x, y, a;

    if((MyScreen=LockPubScreen(NULL)))
    {
        BltBitMapRastPort(MyScreen->RastPort.BitMap,
            ScreenWidth / 2 - WindowWidth / 2,
            ScreenHeight - WindowHeight,
            &RP_Buffer,
            0, 0,
            Window_Max_X,
            Window_Max_Y,
            0xC0);

        UnlockPubScreen(NULL,MyScreen);
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
        }
        else if(picture[1])
        {
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

        // ---------- Labeling icon if NAMES is ON
        if(B_Labels == TRUE)
            IconLabel();

        if(Icon_Remap == TRUE)
        {
            for(x=Levels[CurrentLevel].Beginning; x<IconCounter; x++)
            {
                if(Icon[x]!=NULL)
                {
                    LayoutIcon(Icon[x], MyScreen, NULL);
                }
            }
        }

        if((MainWindow=OpenWindowTags(NULL,
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
            WindowMask = 1 << MainWindow->UserPort->mp_SigBit;

            BltBitMapRastPort(BMP_Buffer,
                0, 0,
                MainWindow->RPort,
                0, 0,
                WindowWidth,
                WindowHeight,
                0xC0);

        for(x=Levels[CurrentLevel].Beginning; x<IconCounter; x++)
        {
            if(Icon[x] != NULL)
            {
                DrawIconState(MainWindow->RPort,
                    Icon[x],
                    NULL,
                    Icons[x].Icon_PositionX,
                    Icons[x].Icon_PositionY,
                    IDS_NORMAL,
                    ICONDRAWA_Frameless, TRUE,
                    ICONDRAWA_EraseBackground, FALSE,
                    TAG_DONE);
            }
        }
    }
    else
    {
        printf("Can't open main toolbar window\n");
        return FALSE;
    }

    Window_Open = TRUE;        
    Window_Active = TRUE;

    return TRUE;
}


static void CloseMainWindow(void)
{
    LONG x;
    if(MainWindow)
        CloseWindow(MainWindow);
    for(x=Levels[CurrentLevel].Beginning; x<IconCounter; x++)
    {
        Icons[x].Icon_Status = 0;
    }

    Window_Open = FALSE;
    WindowMask = 0;
}


static void CheckMousePosition(void)
{
    if((MyScreen->MouseY > ScreenHeight - 8) &&
        Window_Open == FALSE &&
        MyScreen->MouseX > BeginningWindow &&
        MyScreen->MouseX < EndingWindow)
    {
        if(OpenMainWindow() == FALSE)
        {
            BiB_Exit = TRUE;
        }
    }

    // LJ: trying to make window active when going to very bottom of the screen
    //     even in static mode  - but it doesn't work because this code is checked 
    //     only in static=0 mode 

    else
        if ((MyScreen->MouseY > ScreenHeight - 8) &&
            Window_Open == TRUE &&
            MyScreen->MouseX > BeginningWindow &&
            MyScreen->MouseX < EndingWindow)
    {
            printf("LJ: You're at the very bottom! I should make this window active!\n"); 
            Window_Active = TRUE;
    }
}


static void Show_Selected_Level(void)
{
    CloseMainWindow();

    IconCounter   = Levels[CurrentLevel+1].Beginning;
    WindowWidth = Levels[CurrentLevel].WindowPos_X;
    WindowHeight  = Levels[CurrentLevel].WindowPos_Y;
    BeginningWindow = ScreenWidth / 2 - WindowWidth / 2;
    EndingWindow   = ScreenWidth / 2 + WindowWidth / 2;

    //LJ: delay needed for AROS, otherwise garbage from old icons remains
    Delay(10); // 200 ms seems to be enough

    if(OpenMainWindow() == FALSE)
    {
        BiB_Exit = TRUE;
    }
}


static void OpenMenuWindow(void)
{
    LONG x;

    if((MenuWindow = OpenWindowTags(NULL,
                            WA_Left, MyScreen->MouseX,
                            WA_Top, MyScreen->MouseY,
                            WA_InnerWidth, Length + 1,
                            WA_InnerHeight, LevelCounter * 16,
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
        MenuMask = 1 << MenuWindow->UserPort->mp_SigBit;

        SetAPen(MenuWindow->RPort, 1);
        Move(MenuWindow->RPort, Length, 0);
        Draw(MenuWindow->RPort, Length, LevelCounter * 16);
        Draw(MenuWindow->RPort, 0, LevelCounter * 16);
        SetAPen(MenuWindow->RPort, 2);
        Draw(MenuWindow->RPort, 0, 0);
        Draw(MenuWindow->RPort, Length, 0);

        // draw bar
        SetAPen(MenuWindow->RPort, 1);
        Move(MenuWindow->RPort, 0, (LevelCounter - 2) * 16);
        Draw(MenuWindow->RPort, Length - 1, (LevelCounter - 2) * 16);

        SetAPen(MenuWindow->RPort, 2);
        Move(MenuWindow->RPort, 0, ((LevelCounter - 2) * 16) + 1);
        Draw(MenuWindow->RPort, Length - 1, ((LevelCounter - 2) * 16) + 1);

        for(x=0; x<LevelCounter; x++)
        {
            if(x == CurrentLevel)
                Names.FrontPen = 2;
            else
                Names.FrontPen = 1;

            strcpy(BufferList, Levels[x].Level_Name);

            PrintIText(MenuWindow->RPort,
                &Names,
                5,
                (x * 16) + 5);
        }
        MenuWindow_Open = TRUE;
    }
    else
        printf("Can't open menu\n");
}


static void CloseMenuWindow(void)
{
    CloseWindow(MenuWindow);
    MenuMask = 0;

    if(PositionMenuOK == TRUE)
    {
        if(Position == (LevelCounter - 1))
        {
            BiB_Exit = TRUE;
        }
        else if(Position == (LevelCounter - 2))
        {
            Settings();
        }
        else
        {
            CurrentLevel = Position;
            Show_Selected_Level();
        }
    }
}


static void Decode_Menu_IDCMP(struct IntuiMessage *KomIDCMP)
{
    LONG x, kolA, kolB, dluG, x4;

    dluG = Length - 2;

    switch(KomIDCMP->Class)
    {
        case IDCMP_MOUSEMOVE:
            if(MenuWindow->MouseX > 0 &&
                MenuWindow->MouseX < Length &&
                MenuWindow->MouseY >0)
            {
                Position = MenuWindow->MouseY / 16;
            }
            else
            {
                Position = 100;
            }

            if(Position != OldPosition)
            {
                for(x=0; x<LevelCounter; x++)
                {
                    x4 = (x * 16) + 2;

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

                    SetAPen(MenuWindow->RPort, kolA);
                    Move(MenuWindow->RPort, dluG, x4);
                    Draw(MenuWindow->RPort, dluG, x4 + 13);
                    Draw(MenuWindow->RPort, 2, x4 + 13);
                    SetAPen(MenuWindow->RPort, kolB);
                    Draw(MenuWindow->RPort, 2, x4);
                    Draw(MenuWindow->RPort, dluG, x4);
                }

                if(Position != CurrentLevel && Position != 100)
                    PositionMenuOK = TRUE;
                else
                    PositionMenuOK = FALSE;

                OldPosition = Position;

            }
            break;

        case IDCMP_MOUSEBUTTONS:
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


static void Launch_Program(STRPTR Program)
{
    BPTR oldlock;

    // Get current directory lock
    oldlock = CurrentDir(NULL);

    OpenWorkbenchObject(Program, TAG_DONE);

    // Go back to old directory
    CurrentDir(oldlock);
}


static void Settings(void)
{
    OpenWorkbenchObject("SYS:Prefs/BoingIconBar", TAG_DONE);
}


static void Reload(void)
{
    LONG x;

    if (Window_Open == TRUE)
    {
        CloseMainWindow();
    }

    for(x=0; x<SUM_ICON; x++)
    {
        if(Icon[x] != NULL)
        {
            FreeDiskObject(Icon[x]);
            Icon[x]=NULL;
        }
        Icons[x].Icon_Height  = 0;
        Icons[x].Icon_Width = 0;
        Icons[x].Icon_PositionX  = 0;
        Icons[x].Icon_PositionY  = 0;
    }

    if(BMP_Buffer)
        FreeBitMap(BMP_Buffer);
    if(BMP_DoubleBuffer)
        FreeBitMap(BMP_DoubleBuffer);

    if(ReadPrefs() == FALSE)
    {
        puts("Prefs error\n");
        return;
    }

    if(Static)
    {
        Delay(Static * 50);
        OpenMainWindow();
        FirstOpening = FALSE;
    }
}


static void IconLabel(void)
{
    LONG x, y, pos_x;

    for(x=Levels[CurrentLevel].Beginning; x<IconCounter; x++)
    {
        for(y=0; y<Icons[x].IK_Label_Length; y++)
        {
            IT_Labels[y] = Icons[x].IK_Label[y];
        }

        IT_Labels[y] = '\0';

        pos_x = Icons[x].Icon_PositionX + Icons[x].Icon_Width / 2 - IntuiTextLength(&Labels) / 2;

        Labels.FrontPen = 1;

        PrintIText(&RP_Buffer,
            &Labels,
            pos_x,
            WindowHeight - 12);

        Labels.FrontPen = 2;

        PrintIText(&RP_Buffer,
            &Labels,
            pos_x + 1,
            WindowHeight - 13);

    }
}

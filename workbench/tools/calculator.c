#include <exec/types.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/intuition.h>
#include <proto/bgui.h>

#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>
#include <libraries/locale.h>

#include <dos/rdargs.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef _AROS
struct IntuitionBase    *IntuitionBase;
struct LocaleBase       *LocaleBase;
#endif

struct Library          *BGUIBase   = NULL;


#define MAX_VAL_LEN 13

enum
{
    STATE_LEFTVAL,
    STATE_OP,
    STATE_RIGHTVAL,
    STATE_EQU
};


#define ARG_TEMPLATE "PUBSCREEN,TAPE/K"

enum
{
    ARG_PUBSCREEN,
    ARG_TAPE,
    NUM_ARGS
};

#ifndef _AROS
const 
#endif
enum
{
    /* Gadgets */

    ID_0,
    ID_1,
    ID_2,
    ID_3,
    ID_4,
    ID_5,
    ID_6,
    ID_7,
    ID_8,
    ID_9,
    ID_COMMA,
    ID_BS,
    ID_CA,
    ID_CE,
    ID_ADD,
    ID_SUB,
    ID_MUL,
    ID_DIV,
    ID_SIGN,
    ID_EQU,

    ID_LED,

    NUM_GADGETS,

    /* Menu-items */

    ID_QUIT,
    ID_CUT,
    ID_COPY,
    ID_PASTE,
    ID_TAPE
};

struct ButtonInfo
{
    char *label;
    WORD id;
    STRPTR key1;
    STRPTR key2;
};

static struct ButtonInfo bi[ NUM_GADGETS - 1 ] =
{
    { "0"  , ID_0     , "0" ,  0  } ,
    { "1"  , ID_1     , "1" ,  0  } ,
    { "2"  , ID_2     , "2" ,  0  } ,
    { "3"  , ID_3     , "3" ,  0  } ,
    { "4"  , ID_4     , "4" ,  0  } ,
    { "5"  , ID_5     , "5" ,  0  } ,
    { "6"  , ID_6     , "6" ,  0  } ,
    { "7"  , ID_7     , "7" ,  0  } ,
    { "8"  , ID_8     , "8" ,  0  } ,
    { "9"  , ID_9     , "9" ,  0  } ,
    { "."  , ID_COMMA , "." , "," } ,
    { "«"  , ID_BS    , "\x8", 0  } ,
    { "CA" , ID_CA    , "A" , "\x7F" } ,
    { "CE" , ID_CE    , "E" ,  0  } ,
    { "+"  , ID_ADD   , "+" ,  0  } ,
    { "-"  , ID_SUB   , "-" ,  0  } ,
    { "×"  , ID_MUL   , "*" , "X" } ,
    { "÷"  , ID_DIV   , "/" , ":" } ,
    { "±"  , ID_SIGN  , "S" , "±" } ,
    { "="  , ID_EQU   , "=" , "\xD" }
};

struct NewMenu menustrip[] =
{
    Title( "Project" ),
        Item( "Clear entry" , "E" , ID_CE    ),
        Item( "Clear all"   , "A" , ID_CA    ),
        ItemBar,
        Item( "Quit"        , "Q" , ID_QUIT  ),
    Title( "Edit" ),
        Item( "Cut"         , "X" , ID_CUT   ),
        Item( "Copy"        , "C" , ID_COPY  ),
        Item( "Paste"       , "V" , ID_PASTE ),
    Title( "Windows" ),
        Item( "Show tape"   , "T" , ID_TAPE  ),
    End
};

/*** Global variables ********************************************************/

static struct RDArgs *MyArgs = NULL;
static LONG Args[NUM_ARGS];

static char *deftapename = "RAW:%ld/%ld/%ld/%ld/Calculator Tape/INACTIVE/SCREEN%s";
static FILE *tapefh      = NULL;
static BOOL dotape       = FALSE;

static double leftval, rightval;
static WORD vallen, state, operation;

static char comma;
static char ledstring[256], visledstring[256], tempstring[256], tapename[256];

static char *pubscrname;
struct Window   *window;
Object          *WO_Window = NULL , *GO_Led , *buttons[ NUM_GADGETS ] , *base;

/*** Prototypes **************************************************************/

static void DosError( void );

static void Cleanup( char *msg );
static void OpenLibs( void );
static void CloseLibs( void );

static void DoLocale( void );

static void MakeGUI( void );
static void OpenGUI( void );
static void OpenTape( void );

/*** Functions ***************************************************************/

#ifdef _AROS

#undef AROS_TAGRETURNTYPE
#define AROS_TAGRETURNTYPE Object *

Object *BGUI_NewObject(ULONG num, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = BGUI_NewObjectA(num, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
}

#endif

/// static void DosError( void )
static void DosError( void )
{
    Fault( IoErr(), 0, tempstring, 255 );
    Cleanup( tempstring );
} /// DosError()

/// static void Cleanup( char *msg )
static void Cleanup( char *msg )
{
    WORD rc;

    if( msg )
    {
        printf("Calculator: %s\n",msg);
        rc = RETURN_WARN;
    } else {
        rc = RETURN_OK;
    }

    if( tapefh ) fclose( tapefh );

    if( MyArgs ) FreeArgs( MyArgs );

    if( WO_Window ) DisposeObject( WO_Window );

    CloseLibs();

    exit( rc );
} /// Cleanup()
/// static void OpenLibs( void )
static void OpenLibs( void )
{
    if( !(BGUIBase = OpenLibrary( BGUINAME , BGUIVERSION )) )
    {
        sprintf( tempstring , "Can't open %s V%d" , BGUINAME , BGUIVERSION );
        Cleanup( tempstring );
    }

    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library",39);
    
    #ifdef _AROS
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    if (!IntuitionBase) Cleanup("Can't open intuition.librarv V39!");
    #endif
    
} /// OpenLibs()
/// static void CloseLibs( void )
static void CloseLibs( void )
{
    if( BGUIBase ) CloseLibrary( BGUIBase );
    if( LocaleBase ) CloseLibrary( (struct Library *) LocaleBase );
    
    #ifdef _AROS
    if ( IntuitionBase ) CloseLibrary( (struct Library *) IntuitionBase);
    #endif
} /// CloseLibs()

/// static void DoLocale(void)
static void DoLocale(void)
{
    //struct Locale *loc;

    comma = '.';

    /* SDuvan: removed Locale code as it seemed to cause segfaults
               when running calculator several times */
    //    if ((loc = OpenLocale(0)))
    //    {
    //  comma = loc->loc_DecimalPoint[0];
    //  CloseLocale(loc);
    //    }

    bi[ID_COMMA].label[0] = comma;
} /// DoLocale()
/// static void GetArguments( void )
static void GetArguments( void )
{
    if( !(MyArgs = ReadArgs( ARG_TEMPLATE, (LONG *) Args, 0 )) )
    {
        DosError();
    }

    pubscrname = (char *) Args[ARG_PUBSCREEN];

    if( Args[ARG_TAPE] )
    {
        strcpy( tapename, (char *) Args[ARG_TAPE] );
        dotape = TRUE;
    }
} /// GetArguments()

/// static void MakeGUI( void )
static void MakeGUI( void )
{
    int i;

    WO_Window = WindowObject,
        WINDOW_Title,        "Calculator",
        WINDOW_MenuStrip,     menustrip,
        WINDOW_PubScreenName, pubscrname,
        WINDOW_SmartRefresh,  TRUE,
        WINDOW_CloseOnEsc,    TRUE,
        WINDOW_AutoAspect,    TRUE,
        WINDOW_LockHeight,    TRUE,

        WINDOW_MasterGroup,
            base = VGroupObject,
                Offset( 2 ),
                Spacing( 2 ),

                StartMember, GO_Led = InfoObject,
                    DefaultFrame,
                    INFO_TextFormat, "\33r0",
                    INFO_HorizOffset, 2,
                    INFO_VertOffset,  2,
                EndObject, EndMember,

                StartMember ,
                    HGroupObject, EqualWidth, Spacing( 2 ) ,
                        StartMember , buttons[ID_7] = PrefButton( bi[ID_7].label , bi[ID_7].id ) , EndMember,
                        StartMember , buttons[ID_8] = PrefButton( bi[ID_8].label , bi[ID_8].id ) , EndMember,
                        StartMember , buttons[ID_9] = PrefButton( bi[ID_9].label , bi[ID_9].id ) , EndMember,
                        StartMember , buttons[ID_DIV] = PrefButton( bi[ID_DIV].label , bi[ID_DIV].id ) , EndMember,
                        StartMember , buttons[ID_CA] = PrefButton( bi[ID_CA].label , bi[ID_CA].id ) , EndMember,
                    EndObject,
                EndMember,

                StartMember ,
                    HGroupObject, EqualWidth, Spacing( 2 ) ,
                        StartMember , buttons[ID_4] = PrefButton( bi[ID_4].label , bi[ID_4].id ) , EndMember,
                        StartMember , buttons[ID_5] = PrefButton( bi[ID_5].label , bi[ID_5].id ) , EndMember,
                        StartMember , buttons[ID_6] = PrefButton( bi[ID_6].label , bi[ID_6].id ) , EndMember,
                        StartMember , buttons[ID_MUL] = PrefButton( bi[ID_MUL].label , bi[ID_MUL].id ) , EndMember,
                        StartMember , buttons[ID_CE] = PrefButton( bi[ID_CE].label , bi[ID_CE].id ) , EndMember,
                    EndObject,
                EndMember,

                StartMember ,
                    HGroupObject, EqualWidth, Spacing( 2 ) ,
                        StartMember , buttons[ID_1] = PrefButton( bi[ID_1].label , bi[ID_1].id ) , EndMember,
                        StartMember , buttons[ID_2] = PrefButton( bi[ID_2].label , bi[ID_2].id ) , EndMember,
                        StartMember , buttons[ID_3] = PrefButton( bi[ID_3].label , bi[ID_3].id ) , EndMember,
                        StartMember , buttons[ID_SUB] = PrefButton( bi[ID_SUB].label , bi[ID_SUB].id ) , EndMember,
                        StartMember , buttons[ID_BS] = PrefButton( bi[ID_BS].label , bi[ID_BS].id ) , EndMember,
                    EndObject,
                EndMember,

                StartMember ,
                    HGroupObject, EqualWidth, Spacing( 2 ) ,
                        StartMember , buttons[ID_0] = PrefButton( bi[ID_0].label , bi[ID_0].id ) , EndMember,
                        StartMember , buttons[ID_COMMA] = PrefButton( bi[ID_COMMA].label , bi[ID_COMMA].id ) , EndMember,
                        StartMember , buttons[ID_SIGN] = PrefButton( bi[ID_SIGN].label , bi[ID_SIGN].id ) , EndMember,
                        StartMember , buttons[ID_ADD] = PrefButton( bi[ID_ADD].label , bi[ID_ADD].id ) , EndMember,
                        StartMember , buttons[ID_EQU] = PrefButton( bi[ID_EQU].label , bi[ID_EQU].id ) , EndMember,
                    EndObject,
                EndMember,

            EndObject,
    EndObject;

    if( WO_Window )
    {
        // Assign hotkeys to all buttons, but stop before reaching ID_LED

        for( i = 0 ; i < (NUM_GADGETS - 1) ; i++ )
        {
            if( bi[i].key1 )
                GadgetKey( WO_Window , buttons[i] , bi[i].key1 );

            if( bi[i].key2 )
                GadgetKey( WO_Window , buttons[i] , bi[i].key2 );
        }
    }
} /// MakeGUI()
/// static void OpenGUI( void )
static void OpenGUI( void )
{
    if( WO_Window )
    {
        if( !(window = WindowOpen( WO_Window )) )
        {
            Cleanup( "Couldn't open window!" );
        }
    }
} /// OpenGUI()
/// static void OpenTape( void )
static void OpenTape( void )
{
    struct List *l;
    struct PubScreenNode *psn;
    char *scrname = "";
    WORD x,y,w,h;

    if( !(tapename[0]) )
    {
        l = LockPubScreenList();

        psn = (struct PubScreenNode *)l->lh_Head;

        while( psn->psn_Node.ln_Succ )
        {
            if( psn->psn_Screen == window->WScreen )
            {
                if( psn->psn_Node.ln_Name )
                {
                    scrname = psn->psn_Node.ln_Name;
                }
                break;
            }
            psn = (struct PubScreenNode *)psn->psn_Node.ln_Succ;
        }

        UnlockPubScreenList();

        w = window->Width * 5 / 4;
        h = window->Height;

        x = window->LeftEdge;
        y = window->TopEdge;

        if (x > (window->WScreen->Width - (x + w)))
        {
            x -= w;
        } else {
            x += window->Width;
        }
        sprintf( tapename, deftapename, x, y, w, h, scrname );
    }

    if( !(tapefh = fopen( tapename, "w" )) )
    {
        DisplayBeep( window->WScreen );
    }
} ///
/// static void CloseTape( void )
static void CloseTape( void )
{
    fclose( tapefh );
    tapefh = NULL;
} /// CloseTape()
/// static void RefreshLED( void )
static void RefreshLED( void )
{
    strcpy( visledstring , ISEQ_R );  /* Code for the InfoObject to right justify the text */

    if( (ledstring[0] == ',') ||
        (ledstring[0] == '\0') ||
        ((ledstring[0] >= '0') && (ledstring[0] <= '9')) )
    {
        if( (ledstring[0] == ',') ||
            (ledstring[0] == '.') ||
            (ledstring[0] == '\0') )
        {
            strcat( visledstring, "0" );
        }
        strcat( visledstring, ledstring );
    }
    else
    {
        strcat( visledstring, ledstring );
    }

    SetGadgetAttrs( (struct Gadget *) GO_Led , window , NULL , INFO_TextFormat , visledstring , TAG_END );
} /// RefreshLED()

/// static double GetValue( void )
static double GetValue( void )
{
    double val;
    char c,*sp;

    sp = strchr(ledstring,comma);
    if( sp )
    {
        c = *sp;
        *sp = '.';
    }

    val = strtod( ledstring, 0 );

    if (sp) *sp = c;

    return val;
} /// GetValue()
/// static void GetLeftValue( void )
static void GetLeftValue( void )
{
    leftval = GetValue();
} /// GetLeftValue()
/// static void GetRightValue( void )
static void GetRightValue( void )
{
    rightval = GetValue();
} /// GetRightValue()
/// static void LeftValToLED( void )
static void LeftValToLED( void )
{
    char *sp = NULL;
    int  i;

    sprintf( ledstring, "%lf", leftval );

    sp = strchr( ledstring, '.' );
    if( !sp ) sp = strchr( ledstring, ',' );
    if( sp ) *sp = comma;

    /* Remove useless zeroes at the end of the string,
       ie. after the decimal-point (if there is one).  */

    if( sp )
    {
        i = strlen( ledstring ) - 1;
        while( ledstring[i] == '0' && ledstring[i-1] != comma )
        {
            ledstring[i] = '\0';
            i--;
        }
    }
} /// LeftValToLED()

/// static char *DoOperation( void )
static char *DoOperation( void )
{
    char *matherr = NULL;

    switch( operation )
    {
    case ID_ADD:
        leftval += rightval;
        break;

    case ID_SUB:
        leftval -= rightval;
        break;

    case ID_MUL:
        leftval *= rightval;
        break;

    case ID_DIV:
        if (rightval == 0.0)
        {
            matherr = "Division by zero!";
        } else {
            leftval /= rightval;
        }
        break;
    }

    if ( !matherr ) LeftValToLED();

    return matherr;
} /// DoOperation()

/// static void HandleAll( void )
static void HandleAll( void )
{
    ULONG signal = 0, rc;
    BOOL  running = TRUE , refresh_led = FALSE;
    WORD  checklen;
    char  * matherr = NULL;

    if( dotape ) OpenTape();

    GetAttr( WINDOW_SigMask, WO_Window, &signal );
    do
    {
        Wait( signal );
        while (( rc = HandleEvent( WO_Window )) != WMHI_NOMORE )
        {
            refresh_led = FALSE;

            switch ( rc )
            {
                case WMHI_CLOSEWINDOW:
                   running = FALSE;
                   break;

                case ID_0:
                case ID_1:
                case ID_2:
                case ID_3:
                case ID_4:
                case ID_5:
                case ID_6:
                case ID_7:
                case ID_8:
                case ID_9:
                    checklen = vallen;
                    if( (strchr(ledstring,comma)) ) checklen--;
                    if( (strchr(ledstring,'-')) ) checklen--;

                    if (checklen < MAX_VAL_LEN)
                    {
                        if (state == STATE_OP)
                        {
                            state = STATE_RIGHTVAL;
                        } else if (state == STATE_EQU)
                        {
                            state = STATE_LEFTVAL;
                        }

                        if ((vallen > 0) || (rc != ID_0))
                        {
                            ledstring[vallen++] = rc + '0';
                        }
                        ledstring[vallen] = '\0';

                        refresh_led = TRUE;

                    } /* if (vallen < MAX_VAL_LEN) */
                    break;

                case ID_COMMA:
                    if( !strchr( ledstring, comma ) )
                    {
                        if( state == STATE_OP )
                        {
                            state = STATE_RIGHTVAL;
                        } else if (state == STATE_EQU)
                        {
                            state = STATE_LEFTVAL;
                        }

                        ledstring[vallen++] = comma;
                        ledstring[vallen] = '\0';

                        refresh_led = TRUE;

                    } /* if (!strchr(ledstring,comma)) */
                    break;

                case ID_BS:
                    if (vallen)
                    {
                        ledstring[--vallen] = '\0';
                        if (vallen == 0) strcpy(ledstring,"0");
                        refresh_led = TRUE;
                    }
                    break;

                case ID_CA:
                    vallen = 0;
                    leftval = 0.0;
                    rightval = 0.0;
                    operation = ID_ADD;

                    state = STATE_LEFTVAL;

                    strcpy(ledstring,"0");
                    refresh_led = TRUE;

                    if (tapefh) fputs("\n",tapefh);

                    break;

                case ID_CE:
                    vallen = 0;
                    strcpy(ledstring,"0");
                    refresh_led = TRUE;

                    switch (state)
                    {
                    case STATE_LEFTVAL:
                        leftval = 0.0;
                        break;

                    case STATE_OP:
                    case STATE_RIGHTVAL:
                        rightval = 0.0;
                        break;
                    }
                    break;

                case ID_ADD:
                case ID_SUB:
                case ID_MUL:
                case ID_DIV:
                    switch(state)
                    {
                    case STATE_LEFTVAL:
                    case STATE_EQU:
                        GetLeftValue();
                        rightval = leftval;

                        state = STATE_OP;
                        vallen = 0;
                        strcpy(ledstring,"0");

                        if (tapefh)
                        {
                            fprintf(tapefh,"\t%s\n",visledstring);
                            fflush(tapefh);
                        }
                        break;

                    case STATE_OP:
                        break;

                    case STATE_RIGHTVAL:
                        GetRightValue();
                        matherr = DoOperation();
                        state = STATE_OP;
                        vallen = 0;
                        refresh_led = TRUE;

                        if( tapefh )
                        {
                            fprintf(tapefh,"%s\t%s\n",(operation == ID_ADD) ? "+" :
                                            (operation == ID_SUB) ? "-" :
                                            (operation == ID_DIV) ? "/" :
                                            "×" ,visledstring);
                            fflush(tapefh);
                        }
                        break;

                    } /* switch(state) */

                    operation = rc;
                    break;

                case ID_SIGN:
                    switch(state)
                    {
                    case STATE_LEFTVAL:
                    case STATE_RIGHTVAL:
                        if (ledstring[0] == '-')
                        {
                            strcpy(ledstring,&ledstring[1]);
                        } else {
                            strcpy(tempstring,ledstring);
                            strcpy(ledstring,"-");
                            strcat(ledstring,tempstring);
                        }
                        refresh_led = TRUE;
                        break;

                    case STATE_EQU:
                        leftval = -leftval;
                        LeftValToLED();
                        refresh_led = TRUE;
                        break;
                    }
                    break;

                case ID_EQU:
                    if (state == STATE_LEFTVAL)
                    {
                        GetLeftValue();
                        if (tapefh)
                        {
                            fprintf(tapefh,"\t%s\n",visledstring);
                            fflush(tapefh);
                        }
                    }
                    else if (state == STATE_RIGHTVAL)
                    {
                        GetRightValue();
                        if (tapefh)
                        {
                            fprintf(tapefh,"%s\t%s\n",(operation == ID_ADD) ? "+" :
                                            (operation == ID_SUB) ? "-" :
                                            (operation == ID_DIV) ? ":" :
                                            "×" ,visledstring);
                            fflush(tapefh);
                        }
                    }

                    matherr = DoOperation();
                    state = STATE_EQU;

                    vallen = 0;

                    if (!matherr)
                    {
                        RefreshLED();
                        if (tapefh)
                        {
                            fprintf(tapefh,"=\t%s\n",visledstring);
                            fflush(tapefh);
                        }
                    } else {
                        refresh_led = TRUE;
                    }
                    break;

                case ID_TAPE:
                    if( dotape )
                    {
                        CloseTape();
                        dotape = FALSE;
                    }
                    else
                    {
                        OpenTape();
                        dotape = TRUE;
                    }

                    break;
            }

           if (matherr)
           {
               leftval = rightval = 0.0;
               state = STATE_LEFTVAL;
               operation = ID_ADD;
               vallen = 0;
               strcpy(ledstring,matherr);
               refresh_led = TRUE;
           }

          if( refresh_led ) RefreshLED();

        }
    } while( running );
} /// HandleAll()

/// int main( void )
int main( void )
{
    OpenLibs();
    GetArguments();
    DoLocale();
    MakeGUI();
    OpenGUI();
    HandleAll();
    Cleanup( 0 );

    return 0;
} /// main()

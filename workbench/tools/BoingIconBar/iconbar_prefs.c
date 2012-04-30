//////////////////////////////////////////////////////////////
//                                                          //
//       AROS PORT by LuKeJerry (at) gmail.com              //
//                                                          //
// - Last modified 30 Apr 2012                              //
//                                                          //
// - History:                                               //
//     20120430 olivieradam - CTRL_C support                //
//     20120424 olivieradam - cleanup for AROS svn          //
//     20120424 olivieradam - mmakefile.src                 //
//     20120424 olivieradam - localization                  //
//     20120424 olivieradam - catalog translation file      //
//     20120424 olivieradam - french catalog                //
//     20120424 olivieradam - removed warnings              //
//     20121111 lukejerry   - AROS port                     //
//     20110604 stefkos     - rebuild                       //
//                                                          //
//////////////////////////////////////////////////////////////

char version[] = "$VER: BoingIconBar Prefs 1.02 by Robert 'Phibrizzo' Krajcarz";

/// "includes"

#ifndef __MORPHOS__
#ifndef __AROS__
#define __NOLIBBASE__
#endif
#endif



#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/locale.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <exec/exec.h>

#include <libraries/mui.h>

#include <clib/alib_protos.h>

#include <stdio.h>

#include <string.h>

#include "locale.h"

#define CURRENT_SELECTION   23
//#define  ADD_ID           40
//#define SAVE_ID           41

enum { ADD_DOCK = 40, DELETE_DOCK, ADD_PROGRAM, DELETE_PROGRAM, SAVE_PREFS, MAX };

#define MAX_PROGRAMS    20
#define MAX_DOCS        10
#define MAX_PATH        300

static struct DiskObject *disko;

typedef struct dock {
char name[ 50 ];
char programs[ MAX_PROGRAMS+1 ][ MAX_PATH ];
}dock;

dock docs[ MAX_DOCS ];

int saveSettings( char *name );
int     loadList( char *name );

void initTable( char *name )
{
    int i,j;
    
    D(bug("[IconBarPrefs] clear table\n"));
    
    for( i=0 ; i < MAX_DOCS ; i++ )
    {
        for( j=0; j < MAX_PROGRAMS+1 ; j++ )
        {
            docs[ i ].programs[ j ][ 0 ] = 0;
        }
    }
    
    loadList( name );
}

void deleteTable( char *name )
{
    int i,j;
    
    for( i=0 ; i < MAX_DOCS ; i++ )
    {
        for( j=0; j < MAX_PROGRAMS ; j++ )
        {
            docs[ i ].programs[ j ][ 0 ] = 0;
        }
    }
}

///
/// Structures

#ifndef __MORPHOS__
#ifndef __AROS__
extern struct Library *SysBase;
#endif
#endif

///
/// Class data
BOOL running = TRUE;
int var;
Object *app, *win, *Dock, *Programs, *S_Dock;
Object* create_button(char *label, char control)
{
    Object *obj;

    obj = MUI_NewObject(MUIC_Text,
        MUIA_Text_Contents, (ULONG)label,
        MUIA_Text_PreParse, "\33c",
        MUIA_Frame, MUIV_Frame_Button,
        MUIA_Background, MUII_ButtonBack,
        MUIA_Font, MUIV_Font_Button,
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        MUIA_Text_HiChar, control,
        MUIA_ControlChar, control,
        MUIA_CycleChain, TRUE,
        MUIA_HorizWeight, 1,
    TAG_END);

    return obj;
}
Object *Baton[ MAX ];

///
/// "MUIASL"
void add(void)
  {
    struct FileRequester *freq;
    STRPTR name = NULL;
    int docsE = 0, docsS = 0;
    
    get( Dock, MUIA_List_Entries, &docsE );
    get( Dock, MUIA_List_Active, &docsS );
    
    if( docsE < 0 || docsE >= MAX_DOCS )
    {
        D(bug("[IconBarPrefs] Doc not selected , cannot create_docks program!\n"));
    }

    if ((freq = AllocAslRequestTags(ASL_FileRequest, TAG_END))!=NULL)
      {
        if (AslRequestTags(freq,
          ASLFR_TitleText, _(MSG_ADD_P),
          ASLFR_DoPatterns, TRUE,
          ASLFR_RejectIcons, TRUE,
          TAG_END))
          {
            ULONG namelen = strlen(freq->fr_File) + strlen(freq->fr_Drawer) + 4;

            if ((name = AllocVec(namelen + 1, MEMF_ANY | MEMF_CLEAR))!=NULL)
              {
              int entries = 0;
              
              get( Programs, MUIA_List_Entries, &entries );
              
              if( entries < MAX_PROGRAMS )
              {
                strcpy( name, freq->fr_Drawer);
                AddPart( name, freq->fr_File, MAX_PATH );
                DoMethod(Programs, MUIM_List_InsertSingle, name, MUIV_List_Insert_Bottom);
                strcpy( docs[ docsS ].programs[ entries ], name );
               }
              FreeVec( name );
              }
          }
        FreeAslRequest(freq);
      }
    return;
  }
///
/// "GUI"


long GUI (void)
{
    app = ApplicationObject,
        MUIA_Application_Title, _(MSG_APP),
        MUIA_Application_Version, (IPTR)version,
        MUIA_Application_Description, _(MSG_DESCRIPTION),
        MUIA_Application_Base, (IPTR)"BIBPREFS",
        MUIA_Application_DiskObject, (IPTR)disko,
        SubWindow, win = WindowObject,
            MUIA_Window_Title, _(MSG_TITLE),
            MUIA_Window_ID, (IPTR)MAKE_ID('B','I','P','8'),
            MUIA_Window_SizeGadget, TRUE,
            WindowContents, VGroup,
                Child, HGroup,
                    Child, VGroup,
                        Child, TextObject,    MUIA_Text_Contents, _(MSG_DOCK), End,
                        Child, Dock = ListviewObject,
                            MUIA_Listview_List, ListObject,
                                MUIA_Frame, MUIV_Frame_InputList,
                                MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                                MUIA_List_DestructHook, MUIV_List_DestructHook_String,
                            End,
                        End,
                        Child, HGroup,
                        Child, Baton[ ADD_DOCK ] = create_button(_(MSG_ADD_DOCK),'1'),
                                Child, Baton[ DELETE_DOCK ] = create_button(_(MSG_DEL_DOCK),'2'),
                        End,
                        Child, S_Dock = StringObject, MUIA_String_AdvanceOnCR, TRUE,StringFrame,End,
                        Child, TextObject, End,
                    End,
                    Child, VGroup,
                        Child, TextObject,    MUIA_Text_Contents, _(MSG_PROGRAMS),End,
                        Child, Programs = ListviewObject,
                            MUIA_Listview_List, ListObject,
                                MUIA_Frame, MUIV_Frame_InputList,
                                MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                                MUIA_List_DestructHook, MUIV_List_DestructHook_String,
                            End,
                        End,
                        Child, HGroup,
                            Child, Baton[ ADD_PROGRAM ] = create_button(_(MSG_ADD_P),'4'),
                            Child, Baton[ DELETE_PROGRAM ] = create_button(_(MSG_DEL_P),'5'),
                        End,
                        Child, Baton[ SAVE_PREFS ] = create_button(_(MSG_SAVE),'3'),
                    End,
                End,
            End,
        End,
    End;
    return (long)app;
}

///

/// support functions
void load(void)
{
    int vartosc=0;
    {
    get(S_Dock, MUIA_String_Contents, &vartosc);
    var=vartosc;
    }
}

void save(void)
{
    return;
}
///

///widelec

/*nazwê doka + tablicê programów warto by³oby IMHO spi±æ w jak±¶ ³adn± strukturê*/

void create_docks(void)
{
    /*wrzuca doki na listê (ca³a tablica, a¿ do NULL
      przy u¿yciu struktur mo¿na zamieniæ na MUIM_List_Insert_Single*/
    //DoMethod(Dock, MUIM_List_Insert, docki,-1, MUIV_List_Insert_Bottom);
}

void change_programs(void)
{
    //DoMethod(Programs, MUIM_List_Redraw, MUIV_List_Redraw_All); /* odrysuwuje listê, nie wiem czy niezbêdne ;-) */
}

///

/// Notifications
VOID notifications (void)
{
    DoMethod (win, MUIM_Notify, MUIA_Window_CloseRequest,
    MUIV_EveryTime, app, 2, MUIM_Application_ReturnID,
    MUIV_Application_ReturnID_Quit);

    DoMethod (Baton[5], MUIM_Notify, MUIA_Pressed, FALSE, (LONG)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod (Baton[ ADD_DOCK ], MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, ADD_DOCK );

    DoMethod (Baton[ DELETE_DOCK ], MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, DELETE_DOCK );

    DoMethod (Baton[ SAVE_PREFS ], MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, SAVE_PREFS );

    DoMethod (Baton[ ADD_PROGRAM ], MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, ADD_PROGRAM );

    DoMethod (Baton[ DELETE_PROGRAM ], MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, DELETE_PROGRAM );

    DoMethod (Dock, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, app, 2, MUIM_Application_ReturnID, CURRENT_SELECTION);
}

///

/// "MainLoop"
VOID mainloop (void)
{
    ULONG signals = 0;

    SetAttrs (win, MUIA_Window_Open, TRUE, TAG_END);
    
    initTable( "ENV:iconbar.prefs" );

    while (running)
    {
        switch (DoMethod (app, MUIM_Application_Input, &signals))
        {
            case MUIV_Application_ReturnID_Quit:
                running = FALSE;
                break;

            case ADD_DOCK:
            {
                char *ptr=NULL;
                int ent=0;

                get( Dock, MUIA_List_Entries, &ent );

                get( S_Dock, MUIA_String_Contents, &ptr );

                if( ptr != NULL && strlen( ptr ) > 0 && ent < 11 )
                {
                    int i;
                    DoMethod(Dock, MUIM_List_InsertSingle, ptr, MUIV_List_Insert_Bottom);
                    strcpy( docs[ ent ].name, ptr );

                        for( i=0; i < MAX_PROGRAMS ; i++ )
                        {
                            docs[ ent ].programs[ i ][ 0 ] = 0;
                        }

                    nnset( Dock, MUIA_List_Active, MUIV_List_Active_Bottom );
                }
            }
            break;

            case DELETE_DOCK:
            {
                int dact=0, dent=0;

                get( Dock, MUIA_List_Active, &dact );
                get( Dock, MUIA_List_Entries, &dent );

                if( dact >= 0 )
                {
                    int i;

                    for( i=dact ; i < dent ; i++ )
                    {
                        if( strlen( docs[ i+1 ].name ) > 0 )
                        {
                            memcpy( &docs[ i ], &docs[ i+ 1 ], sizeof( dock ) );
                        }else{
                            docs[ i ].name[ 0 ] = 0;
                        }
                    }

                    for( i=0; i < MAX_PROGRAMS ; i++ )
                    {
                        if( docs[ dent ].programs[ i ][ 0 ] != 0 )
                        {
                            docs[ dent ].programs[ i ][ 0 ] = 0;
                        }
                    }
                    DoMethod(Dock, MUIM_List_Remove, dact);
                }
            }
            break;

            case ADD_PROGRAM:
            {
                add();
            }
            break;

            case DELETE_PROGRAM:
            {
                int del=0, ent=0;
                int dact=0;

                get( Dock, MUIA_List_Active, &dact );
                get( Programs, MUIA_List_Active, &del );
                get( Programs, MUIA_List_Entries, &ent );

                if( del >= 0 )
                {
                    int i;

                    for( i=del ; i < ent ; i++ )
                    {
                        if( docs[ dact ].programs[ i+1 ][ 0 ] != 0 )
                        {
                            strcpy( docs[ dact ].programs[ i ], docs[ dact ].programs[ i+1 ] );
                        } else {
                            //FreeVec( docs[ dact ].programs[ i ] );
                            docs[ dact ].programs[ i ][ 0 ] = 0;
                        }
                    }
                    DoMethod(Programs, MUIM_List_Remove, MUIV_List_Remove_Selected);
                }
            }
            break;

            case SAVE_PREFS:
            {
                saveSettings( "ENVARC:iconbar.prefs" );
                saveSettings( "ENV:iconbar.prefs" );
                //save();
            }
            break;
                
            case CURRENT_SELECTION:
            {
                LONG numer_docka = 0;
                int j;

                get(Dock, MUIA_List_Active, &numer_docka);

                if( numer_docka < 0 )
                {
                    DoMethod(Programs, MUIM_List_Clear, 0);
                    break;
                }

                set(Programs, MUIA_List_Quiet, TRUE);

                DoMethod(Programs, MUIM_List_Clear, 0);

                for( j=0; j < MAX_PROGRAMS ; j++ )
                {
                    if( docs[numer_docka].programs[ j ][ 0 ] != 0 )
                    {
                        DoMethod(Programs, MUIM_List_InsertSingle,
                            docs[numer_docka].programs[ j ], MUIV_List_Insert_Bottom);  // programs to launch
                        // D(bug("[IconBar] doc %d changed to: \n", numer_docka,docs[numer_docka].programs[ j ] ));
                    }
                }

                set(Programs, MUIA_List_Quiet, FALSE); /*w³±cza od¶wie¿anie*/
            }
            break;
        }
        if (running && signals)
            signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
        if (signals & SIGBREAKF_CTRL_C) running = FALSE;
        if (signals & SIGBREAKF_CTRL_F) running = FALSE;
    }
    deleteTable( "ENV:iconbar.prefs" );

    SetAttrs (win, MUIA_Window_Open, FALSE, TAG_END);
    return;
}

///

/// main function
int main (int argc, char **argv)
{
    static struct WBStartup *argmsg;
    static struct WBArg *wb_arg;
    static STRPTR cxname;

    if (argc)
    {
        cxname = argv[0];
    } else {
        argmsg = (struct WBStartup *)argv;
        wb_arg = argmsg->sm_ArgList;
        cxname = wb_arg->wa_Name;
    }
    disko = GetDiskObject(cxname);

    if (GUI())
    {
        create_docks();
        load ();
        notifications ();
        mainloop ();
        MUI_DisposeObject (app);
    }
    return 0;
}
///

//
// encode pass
// arguments:
// pass - ptr to password string
// return 1 if ok, 0 nok
//

// process priority, encoding password, list post local-host/host-local

int saveSettings( char *name )
{
    FILE *fp;

    if( ( fp = fopen( name, "wb" ) ) != NULL )
    {
        int i, j;

        fprintf( fp, "BOING_PREFS\n" );

        for( i=0 ; i < MAX_DOCS ; i++ )
        {
            if( strlen( docs[ i ].name ) > 0 )
            {
                fprintf( fp, ";%s\n", docs[ i ].name );

                for( j=0; j < MAX_PROGRAMS ; j ++ )
                {
                    if( docs[ i ].programs[ j ][0] != 0 )
                    {
                        fprintf(fp, "%s\n", docs[ i ].programs[ j ] );
                    }else{
                        break;
                    }
                }
            }else{
                break;
            }
        }
        fclose( fp );
    }
    return 1;
}

int loadList( char *name )
{
    FILE *fp;
    int active = -1;
    int entries = 0;
    int line = 0;

    if( ( fp = fopen( name, "rb" ) ) != NULL )
    {
        char buffer[ 500 ];

        while( ( fgets( buffer, 300, fp) ) != NULL )
        {
            buffer[ strlen( buffer ) -1 ] = 0;

            if( line == 0 )
            {
                if( strncmp( buffer, "BOING_PREFS", 11 ) != 0 )
                {
                    D(bug("[IconBarPrefs] %s: is not BOING BAR configuration file!\n"));
                    break;
                }
            }
            else
            {
                if( buffer[ 0 ] == ';' )
                {
                    active++;
                    strcpy( docs[ active ].name, &buffer[ 1 ] );
                    entries = 0;
                    DoMethod(Dock, MUIM_List_InsertSingle, &buffer[ 1 ], MUIV_List_Insert_Bottom );
                    D(bug("[IconBarPrefs] %s buffer\n", &buffer[ 1 ] ));
                }
                else
                {
                    {
                        strcpy( docs[ active ].programs[ entries ], buffer );
                        D(bug("[IconBarPrefs] data set %d / %d   - %s\n", active, entries, docs[ active ].programs[ entries ] ));
                        entries++;
                    }
                }
            }
            line++;
        }
        fclose( fp );
    }
    else
    {
        D(bug("[IconBarPrefs] cannot open file! %s\n", name ));
    }
    return 1;
}
///

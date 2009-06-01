#include <aros/debug.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>

LONG main( LONG argc, char **argv )
{
    if( argc != 2 )
    {
        bug("Argument missing\n");
        return RETURN_WARN;
    }

    struct EasyStruct InfoReq =
        { sizeof( struct EasyStruct ), 0, 0L, 0L, "Ok" };
    BYTE ModuleName[64], Title[108];
    STRPTR Ptr;
    STRPTR TxtBuffer;

    strcpy( ModuleName, FilePart( argv[1] ));
    if( Ptr = strstr( ModuleName, ".txt" ))
        *Ptr = '\0';
    strcpy( Title, ModuleName );
    strcat( Title, " Information" );

    if( TxtBuffer = AllocVec( 1024, MEMF_CLEAR ))
    {
        BPTR InfoHandle;

        if( InfoHandle = Open( argv[1], MODE_OLDFILE ))
        {
            LONG BytesRead;

            BytesRead = Read( InfoHandle, TxtBuffer, 1024 );
            if( TxtBuffer[BytesRead-1] == '\n' )
                TxtBuffer[BytesRead-1] = '\0';
            else
                TxtBuffer[BytesRead] = '\0';
            Close( InfoHandle );
        }
        else
        {
            strcpy( TxtBuffer,
                "No information available for module: " );
            strcat( TxtBuffer, ModuleName );
            strcat( TxtBuffer, "." );
        }

        InfoReq.es_Title = Title;
        InfoReq.es_TextFormat = TxtBuffer;
        EasyRequestArgs( 0L, &InfoReq, 0L, 0L );
        FreeVec( TxtBuffer );
    }

    return RETURN_OK;
}

#include <proto/exec.h>
#include <proto/dos.h>

#include "modes.h"
#include "package.h"
#include "support.h"

int main()
{
    APTR pkg = PKG_Open( "/test.pkg", MODE_READ );
    if( pkg == NULL ) goto error;
    
    PKG_ExtractEverything( pkg );
    
    PKG_Close( pkg );

    return 0;

error:
    return 1;
}

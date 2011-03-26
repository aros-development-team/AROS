#include <aros/debug.h>
#include <proto/dos.h>

BPTR scripthandle;
CONST_STRPTR scriptname;
UBYTE command[200];
LONG error;
LONG failcnt;
LONG errorcnt;
LONG warncnt;
LONG okcnt;

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        scriptname = "testscript";
    }
    else if (argc == 2)
    {
        scriptname = argv[1];
    }
    else
    {
        PutStr("Usage runtest [scriptfile]\n");
    }

    scripthandle = Open(scriptname, MODE_OLDFILE);
    if (!scripthandle)
    {
        PutStr("Can't open file\n");
        return 0;
    }

    PutStr("Reading commands from file ");
    PutStr(scriptname);
    PutStr("\nOutput will be sent to the debugging console\n\n");

    while (FGets(scripthandle, command, sizeof command))
    {
        if (command[0] != '#' && command[0] != '\n')
        {
            bug("===============================\n");
            bug("Running command \"%s\"", command);
            error = SystemTags(command, TAG_DONE);
            bug("returns: %d\n", error);
            
            if (error >= RETURN_FAIL)
                failcnt++;
            else if (error >= RETURN_ERROR)
                errorcnt++;
            else if (error >= RETURN_WARN)
                warncnt++;
            else
                okcnt++;
        }
    }
    bug("===============================\n");
    bug("Summary: ok %d warn %d error %d fail %d\n", okcnt, warncnt, errorcnt, failcnt);

    return 0;
}

#include <proto/exec.h>
#include <proto/dos.h>

int main(int argc, char **argv) {
IPTR args[1] = {FALSE};
struct RDArgs *rda;
LONG error = RETURN_OK;

	rda = ReadArgs("FULL/S", args, NULL);
	if (rda)
	{
		if (args[0])
		{
			PutStr("niy\n");
		}
		else
			ColdReboot();
	}
	else
		error = IoErr();
	if (error != RETURN_OK)
	{
		PrintFault(error, argv[0]);
		error = RETURN_FAIL;
	}
	return error;
}

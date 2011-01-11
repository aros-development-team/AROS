/*
 * We use Printf() here instead of C++ traditional cout because linking in cout
 * increases executable size up to 3 megabytes (!!!), making it difficult to
 * disassemble it. Anyway our aim is to test exceptions.
 */
#include <exec/alerts.h>
#include <proto/dos.h>
#include <proto/exec.h>

/*
 * Non-working call frame unwinding causes callibg abort().
 * Here we manually override this function to call Alert(),
 * which can give us a stack trace.
 */
void abort(void)
{
    Printf("abort() called\n");

    Alert(AN_Unknown);
}

/* Just to make things a little bit more complex */
int sub()
{
    Printf("sub() entered\n");

    throw 20;
}

int main ()
{
    try
    {
	sub();
    }
    catch (int e)
    {
	Printf("An exception occurred. Exception Nr. %d\n", e);
    }

    Printf("Exiting\n");
    return 0;
}

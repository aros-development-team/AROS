#include <proto/exec.h>
#include <aros/symbolsets.h>

int set_open_libraries(struct libraryset *set[])
{
    int n = 1;
    while(set[n])
    {
	*(set[n]->baseptr) = OpenLibrary(set[n]->name, set[n]->version);
	if (!*(set[n]->baseptr))
	    return 20;

        if (set[n]->postopenfunc)
	{
	    int ret = set[n]->postopenfunc();
	    if (ret) return ret;
        }

        n++;
    }
    return 0;
}

void set_close_libraries(struct libraryset *set[])
{
    int n = 1;
    while(set[n])
    {
	if (*(set[n]->baseptr))
	{
	    if (set[n]->preclosefunc) set[n]->preclosefunc();
	    CloseLibrary(*(set[n]->baseptr));
        }

	n++;
    }
}

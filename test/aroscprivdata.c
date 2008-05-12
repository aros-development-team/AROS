#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/bptr.h>
#include <dos/dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../rom/exec/etask.h"

struct userdata
{
        struct Task *parent;
};

void showuserdata(struct Task *this, struct Task *parent)
{
	struct arosc_privdata *acpd = NULL;
	acpd = GetIntETask(this)->iet_acpd;
	printf("Task %p acpd: %p\n", this, acpd);
	
	if(acpd == NULL)
	{
		printf("acpd is NULL, trying it in parent\n");
		if (parent)
		{
	        acpd = GetIntETask(parent)->iet_acpd;
		    printf("Parent %p acpd: %p\n", parent, acpd);		    
	    }
	}
}

LONG secondchild()
{
    struct Task *parent;
    struct Task *this;

    printf("Grandchild\n");
	this = FindTask(0);
	parent = ((struct userdata*)this->tc_UserData)->parent;
	showuserdata(this, parent);
    Forbid();
    Signal(parent, SIGBREAKF_CTRL_F);	
}

LONG firstchild()
{
    struct Task *parent;
    struct Task *this;
	struct Process *proc = NULL;
	struct userdata taskdata;

    printf("Child\n");
	this = FindTask(0);
	parent = ((struct userdata*)this->tc_UserData)->parent;
	showuserdata(this, parent);

	taskdata.parent = this;

    proc = CreateNewProcTags(
        NP_Entry,                secondchild,
        NP_Name,                 "Ikari Shinji",
        NP_UserData,             &taskdata,
        TAG_DONE);
    
	if ( proc != NULL ) {
        Wait(SIGBREAKF_CTRL_F);
    }
	
	Forbid();
    Signal(parent, SIGBREAKF_CTRL_F);
}

int main(int argc, char ** argv)
{
	struct Process *proc = NULL;
	struct userdata taskdata;
	struct Task *thisTask = FindTask(0);
	taskdata.parent = thisTask;

    proc = CreateNewProcTags(
        NP_Entry,                firstchild,
        NP_Name,                 "Ayanami Rei",
        NP_UserData,             &taskdata,
        TAG_DONE);
    
	if ( proc != NULL ) {
        Wait(SIGBREAKF_CTRL_F);
    }
}
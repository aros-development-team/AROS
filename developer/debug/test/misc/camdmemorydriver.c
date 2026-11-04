/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Exercise the camd.library V43 resident-driver lifecycle.
*/

#include <exec/types.h>
#include <libcore/compiler.h>
#include <midi/camd.h>
#include <midi/camddevices.h>
#include <proto/camd.h>
#include <proto/exec.h>
#include <stdio.h>

struct Library *CamdBase;

static ULONG init_count, expunge_count, ordered_expunge_count;
static ULONG open_count, close_count;
static struct MidiPortData portdata;

static void ASM ActivateXmit(REG(a2) APTR userdata, REG(d0) ULONG portnum)
{
    (void)userdata;
    (void)portnum;
}

static SAVEDS ASM BOOL DriverInit(REG(a6) APTR sysbase)
{
    (void)sysbase;
    ++init_count;
    return TRUE;
}

static void DriverExpunge(void)
{
    if(close_count==expunge_count+1) ++ordered_expunge_count;
    ++expunge_count;
}

static SAVEDS ASM struct MidiPortData *DriverOpenPort(
    REG(a3) struct MidiDeviceData *data,
    REG(d0) LONG portnum,
    REG(a0) ULONG (ASM *transmit)(REG(a2) APTR userdata),
    REG(a1) void (ASM *receive)(REG(d0) UWORD input, REG(a2) APTR userdata),
    REG(a2) APTR userdata)
{
    (void)data;
    (void)portnum;
    (void)transmit;
    (void)receive;
    (void)userdata;
    ++open_count;
    portdata.ActivateXmit=ActivateXmit;
    return &portdata;
}

static ASM void DriverClosePort(
    REG(a3) struct MidiDeviceData *data,
    REG(d0) LONG portnum)
{
    (void)data;
    (void)portnum;
    ++close_count;
}

static struct MidiDeviceData driver={
    MDD_Magic,
    (char *)"memorytest",
    (char *)"CAMD resident-driver lifecycle test",
    1, 0,
    DriverInit,
    DriverExpunge,
    DriverOpenPort,
    DriverClosePort,
    1,
    1
};

static int failures;

static void check(BOOL condition, const char *name)
{
    printf("%s %s\n",condition ? "PASS" : "FAIL",name);
    if(!condition) ++failures;
}

static BOOL cluster_has_driver(CONST_STRPTR name)
{
    struct MidiCluster *cluster;
    struct TagItem tags[2];
    APTR lock;
    IPTR present=FALSE;

    lock=LockCAMD(CD_Linkages);
    cluster=FindCluster((STRPTR)name);
    tags[0].ti_Tag=MCLA_HasDriver;
    tags[0].ti_Data=(IPTR)&present;
    tags[1].ti_Tag=TAG_DONE;
    tags[1].ti_Data=0;
    if(cluster!=NULL) GetClusterAttrsA(cluster,tags);
    UnlockCAMD(lock);
    return cluster!=NULL && present!=FALSE;
}

int main(void)
{
    struct MidiNode *node=NULL;
    struct MidiLink *link=NULL;
    struct TagItem node_tags[2];
    struct TagItem link_tags[3];
    ULONG error=0;

    CamdBase=OpenLibrary("camd.library",43);
    if(CamdBase==NULL){
        printf("FAIL open camd.library V43\n");
        return 1;
    }

    check(AddMidiDevice(&driver),"add resident driver");
    check(!AddMidiDevice(&driver),"reject duplicate resident driver");
    check(init_count==1,"duplicate does not call Init");
    check(cluster_has_driver("memorytest.out.0"),"driver joins its cluster");

    node_tags[0].ti_Tag=MIDI_Name;
    node_tags[0].ti_Data=(IPTR)"resident-driver-test";
    node_tags[1].ti_Tag=TAG_DONE;
    node_tags[1].ti_Data=0;
    node=CreateMidiA(node_tags);
    check(node!=NULL,"create client node");
    if(node!=NULL){
        link_tags[0].ti_Tag=MLINK_Location;
        link_tags[0].ti_Data=(IPTR)"memorytest.out.0";
        link_tags[1].ti_Tag=MLINK_ErrorCode;
        link_tags[1].ti_Data=(IPTR)&error;
        link_tags[2].ti_Tag=TAG_DONE;
        link_tags[2].ti_Data=0;
        link=AddMidiLinkA(node,MLTYPE_Sender,link_tags);
    }
    check(link!=NULL && open_count==1,"client link opens resident port");

    check(RemoveMidiDevice(&driver),"remove resident driver");
    check(close_count==1 && expunge_count==1 && ordered_expunge_count==1,
        "remove closes ports before expunge");
    check(!cluster_has_driver("memorytest.out.0"),
        "waiting client cluster no longer has a driver");
    check(!RemoveMidiDevice(&driver),"reject second removal");

    check(AddMidiDevice(&driver),"add resident driver again");
    check(open_count==2,"waiting client link reopens returning port");
    check(RemoveMidiDevice(&driver),"remove returning resident driver");
    check(close_count==2 && expunge_count==2 && ordered_expunge_count==2,
        "second removal closes before expunge");

    if(link!=NULL) RemoveMidiLink(link);
    if(node!=NULL) DeleteMidi(node);
    CloseLibrary(CamdBase);

    printf("camdmemorydriver: %d failure(s)\n",failures);
    return failures ? 1 : 0;
}

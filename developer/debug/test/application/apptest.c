/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: application.library test. Prints one PASS/FAIL line per check;
          with WAITBACK=n it then waits n seconds for the pop-up's
          double-click message.
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/application.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/application.h>
#include <proto/rexxsyslib.h>

#include <stdio.h>
#include <string.h>

struct Library *ApplicationBase;
struct RxsLib *RexxSysBase;

static int passed, failed;

static void check(BOOL ok, const char *what)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (ok)
        passed++;
    else
        failed++;
}

/* Take every message waiting on a port; return how many had 'type' (and,
   if from is not ~0, that sender). */
static int drainfrom(struct MsgPort *port, ULONG type, ULONG from, char *text, int textsize)
{
    struct ApplicationMsg *am;
    int n = 0;

    while ((am = (struct ApplicationMsg *)GetMsg(port)))
    {
        if (am->type == type && (from == (ULONG)~0 || am->senderAppID == from))
        {
            n++;
            if (text && type == APPLIBMT_CustomMsg)
            {
                strncpy(text, ((struct ApplicationCustomMsg *)am)->customMsg, textsize - 1);
                text[textsize - 1] = '\0';
            }
        }
        ReplyMsg(&am->msg);
    }
    return n;
}

static int drain(struct MsgPort *port, ULONG type, char *text, int textsize)
{
    return drainfrom(port, type, (ULONG)~0, text, textsize);
}

/*
 * Send one command to an ARexx port the way Hollywood's SendRexxCommand()
 * does: a RexxMsg built with rexxsyslib, no interpreter involved.
 * Returns rm_Result1, or -1 if the port is missing; rc2 gets rm_Result2.
 */
static LONG rexx(CONST_STRPTR portname, CONST_STRPTR command, LONG *rc2)
{
    struct MsgPort *reply, *port;
    struct RexxMsg *rm;
    LONG rc = -1;

    if (!RexxSysBase || !(reply = CreateMsgPort()))
        return -1;
    if ((rm = CreateRexxMsg(reply, NULL, NULL)))
    {
        rm->rm_Args[0] = (IPTR)CreateArgstring((STRPTR)command, strlen(command));
        rm->rm_Action = RXCOMM;
        Forbid();
        if ((port = FindPort(portname)))
            PutMsg(port, &rm->rm_Node);
        Permit();
        if (port)
        {
            WaitPort(reply);
            GetMsg(reply);
            rc = rm->rm_Result1;
            if (rc2)
                *rc2 = rm->rm_Result2;
        }
        DeleteArgstring((UBYTE *)rm->rm_Args[0]);
        DeleteRexxMsg(rm);
    }
    DeleteMsgPort(reply);
    return rc;
}

/* A memory stream for ALPO_Serialize / ALPO_Deserialize. */
struct MemIO
{
    char buf[4096];
    LONG len, pos, back;
};

static void mem_Put(LONG c, struct ALPOPutProcInfo *pi)
{
    struct MemIO *m = pi->handlePtr;

    if (m->len < (LONG)sizeof(m->buf) - 1)
        m->buf[m->len++] = c;
    m->buf[m->len] = '\0';
}

static LONG mem_Get(struct ALPOGetProcInfo *pi)
{
    struct MemIO *m = pi->handlePtr;

    if (m->back >= 0)
    {
        LONG c = m->back;

        m->back = -1;
        return c;
    }
    return m->pos < m->len ? (UBYTE)m->buf[m->pos++] : -1;
}

static LONG mem_UnGet(LONG c, struct ALPOGetProcInfo *pi)
{
    ((struct MemIO *)pi->handlePtr)->back = c;
    return c;
}

/* Read 'xml' into a new dictionary; the error goes to *err, the dictionary
   back (or NULL when the type was not a dictionary). */
static PrefsObject *fromxml(struct MemIO *mem, CONST_STRPTR xml, ULONG *err)
{
    struct ALPOGetProcInfo gpi;
    TEXT otag[TAGBUFFER_SIZE];
    PrefsObject *dict = NULL;

    if ((const char *)xml != mem->buf)
    {
        memset(mem, 0, sizeof(*mem));
        strncpy(mem->buf, xml, sizeof(mem->buf) - 1);
    }
    mem->len = strlen(mem->buf);
    mem->pos = 0;
    mem->back = -1;
    memset(&gpi, 0, sizeof(gpi));
    gpi.getChProc = mem_Get;
    gpi.unGetChProc = mem_UnGet;
    gpi.handlePtr = mem;
    gpi.key_otag = otag;
    gpi.encoding = ALPOENC_UTF8;
    *err = 0;
    if (BeginDeserialization(&gpi) == ALPOT_Dictionary)
    {
        dict = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE);
        PrefsDictionary(dict, err, ALPO_Deserialize, (IPTR)&gpi, TAG_DONE);
    }
    else
        *err = ALPOEF_WRONG_OTAG;
    return dict;
}

static double realof(PrefsObject *dict, CONST_STRPTR key)
{
    double d = 0;
    PrefsObject *o = DictGetObjectForKey(dict, key);

    if (o)
        PrefsNumber(o, NULL, ALPONUM_GetDouble, (IPTR)&d, TAG_DONE);
    return d;
}

static PrefsObject *num(LONG v)
{
    return PrefsNumber(NULL, NULL, ALPONUM_AllocSetLong, v, TAG_DONE);
}

static PrefsObject *str(CONST_STRPTR s)
{
    return PrefsString(NULL, NULL, ALPOSTR_AllocSetString, (IPTR)s, TAG_DONE);
}

static void release(PrefsObject *o)
{
    PrefsBaseObject(o, NULL, ALPO_Release, 0, TAG_DONE);
}

/* A dictionary with one of everything. */
static PrefsObject *sample(void)
{
    static UBYTE bytes[5] = { 0, 1, 0xfe, 0xff, 'x' };
    struct ALPOBinData bd = { bytes, sizeof(bytes) };
    double d = -3.25;
    PrefsObject *dict, *arr;

    dict = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE);
    DictSetObjectForKey(dict, str("a <tag> & \"quotes\" caf\xe9"), "String");
    DictSetObjectForKey(dict, num(-42), "Integer");
    DictSetObjectForKey(dict, PrefsNumber(NULL, NULL, ALPONUM_AllocSetBool, TRUE, TAG_DONE), "Switch");
    DictSetObjectForKey(dict, PrefsNumber(NULL, NULL, ALPONUM_AllocSetDouble, (IPTR)&d, TAG_DONE), "Real");
    DictSetObjectForKey(dict, PrefsDate(NULL, NULL, ALPODAT_AllocSetCTime, 1760000000, TAG_DONE), "When");
    DictSetObjectForKey(dict, PrefsBinary(NULL, NULL, ALPOBIN_AllocWithData, (IPTR)&bd, TAG_DONE), "Blob");
    DictSetObjectForKey(dict, str("SecondOption"), "Choice");
    arr = PrefsArray(NULL, NULL, ALPO_Alloc, 0,
                     ALPOARR_AddObj, (IPTR)num(1),
                     ALPOARR_AddObj, (IPTR)num(2),
                     ALPOARR_AddObj, (IPTR)num(3),
                     TAG_DONE);
    DictSetObjectForKey(dict, arr, "this is an array");
    return dict;
}

/* TRUE if dict holds what sample() put in. */
static BOOL same(PrefsObject *dict)
{
    static CONST_STRPTR options[] = { "FirstOption", "SecondOption", "ThirdOption", NULL };
    PrefsObject *o, *arr;
    struct ALPOObjIndex oi;
    struct ALPOBinData bd = { NULL, 0 };
    double d = 0;
    ULONG secs = 0, type = 0;
    LONG count = 0;

    if (strcmp(DictGetStringForKey(dict, "String", ""), "a <tag> & \"quotes\" caf\xe9") != 0)
        return FALSE;
    if (DictGetIntegerForKey(dict, "Integer", 0) != -42 || !DictGetBoolForKey(dict, "Switch", FALSE))
        return FALSE;
    if (!(o = DictGetObjectForKey(dict, "Real")))
        return FALSE;
    PrefsNumber(o, NULL, ALPONUM_GetDouble, (IPTR)&d, TAG_DONE);
    if (d != -3.25)
        return FALSE;
    if (!(o = DictGetObjectForKey(dict, "When")))
        return FALSE;
    PrefsDate(o, NULL, ALPODAT_GetSecsSince1978, (IPTR)&secs, TAG_DONE);
    if (secs != 1760000000UL - 2922UL * 86400UL)
        return FALSE;
    if (!(o = DictGetObjectForKey(dict, "Blob")))
        return FALSE;
    PrefsBinary(o, NULL, ALPOBIN_GetDataStruct, (IPTR)&bd, TAG_DONE);
    if (bd.size != 5 || bd.data[0] != 0 || bd.data[2] != 0xfe || bd.data[4] != 'x')
        return FALSE;
    if (DictGetOptionForKey(dict, "Choice", options, -1) != 1)
        return FALSE;
    arr = DictGetObjectForKey(dict, "this is an array");
    PrefsBaseObject(arr, NULL, ALPO_Identify, (IPTR)&type, TAG_DONE);
    PrefsArray(arr, NULL, ALPOARR_GetCount, (IPTR)&count, TAG_DONE);
    oi.index = 2;
    PrefsArray(arr, NULL, ALPOARR_GetObjAtIndex, (IPTR)&oi, TAG_DONE);
    return type == ALPOT_Array && count == 3 && oi.obj
           && PrefsNumber(oi.obj, NULL, ALPONUM_GetLong, (IPTR)&count, TAG_DONE) && count == 3;
}

static void test_prefsobjects(void)
{
    PrefsObject *dict, *copy, *back, *o;
    struct ALPOPutProcInfo ppi;
    struct ALPOGetProcInfo gpi;
    struct ALPOObjKeyIndex oki;
    struct ALPOObjIndex oi;
    struct MemIO *mem;
    TEXT otag[TAGBUFFER_SIZE];
    ULONG err = 0, type;
    LONG count = -1, rc;

    dict = sample();
    check(dict && same(dict), "PrefsObjects: every type stored and read back");

    copy = PrefsDictionary(dict, &err, ALPO_Copy, 0, TAG_DONE);
    DictSetObjectForKey(copy, num(7), "Integer");
    check(err == 0 && copy != dict && DictGetIntegerForKey(dict, "Integer", 0) == -42
          && DictGetIntegerForKey(copy, "Integer", 0) == 7, "ALPO_Copy is deep");
    release(copy);

    o = DictGetObjectForKey(dict, "String");
    PrefsBaseObject(o, NULL, ALPO_Retain, 0, TAG_DONE);
    PrefsDictionary(dict, NULL, ALPODICT_RemoveObjForKey, (IPTR)"String", TAG_DONE);
    count = 0;
    PrefsBaseObject(o, NULL, ALPO_GetRetainCount, (IPTR)&count, TAG_DONE);
    check(count == 1 && DictGetObjectForKey(dict, "String") == NULL, "retained object survives its removal");
    DictSetObjectForKey(dict, o, "String");

    err = 0;
    PrefsString(DictGetObjectForKey(dict, "Integer"), &err, ALPOSTR_SetString, (IPTR)"x", TAG_DONE);
    check(err & ALPOEF_WRONG_OBJECTTYPE, "a method of the wrong type is refused");

    oki.obj = num(1);
    oki.key = "Integer";
    oki.index = 0;
    err = 0;
    PrefsDictionary(dict, &err, ALPODICT_InsertObjAtIndex, (IPTR)&oki, TAG_DONE);
    check(err & ALPOEF_DUPLICATE, "inserting a key twice is refused");
    release(oki.obj);

    PrefsDictionary(dict, NULL, ALPODICT_SortAscending, 0, TAG_DONE);
    oki.index = 0;
    PrefsDictionary(dict, NULL, ALPODICT_GetObjAndKeyAtIndex, (IPTR)&oki, TAG_DONE);
    check(oki.key && strcmp(oki.key, "Blob") == 0, "dictionary sorted by key");

    o = DictGetObjectForKey(dict, "this is an array");
    oi.obj = num(0);
    oi.index = 0;
    PrefsArray(o, NULL, ALPOARR_InsertObjAtIndex, (IPTR)&oi, TAG_DONE);
    oi.index = -1;
    PrefsArray(o, NULL, ALPOARR_GetIndexForObj, (IPTR)&oi, TAG_DONE);
    rc = oi.index;
    PrefsArray(o, NULL, ALPOARR_RemoveObj, (IPTR)oi.obj, ALPOARR_GetCount, (IPTR)&count, TAG_DONE);
    check(rc == 0 && count == 3, "array insert, find and remove");

    /* Serialize to memory and read it back by hand */
    mem = AllocVec(sizeof(*mem), MEMF_CLEAR);
    memset(&ppi, 0, sizeof(ppi));
    ppi.putChProc = mem_Put;
    ppi.handlePtr = mem;
    ppi.encoding = ALPOENC_UTF8;
    err = 0;
    PrefsDictionary(dict, &err, ALPO_Serialize, (IPTR)&ppi, TAG_DONE);
    check(err == 0 && strstr(mem->buf, "<key>Integer</key>") && strstr(mem->buf, "&lt;tag&gt; &amp;")
          && strstr(mem->buf, "caf\xc3\xa9"), "ALPO_Serialize writes escaped UTF-8 XML");

    memset(&gpi, 0, sizeof(gpi));
    mem->back = -1;
    gpi.getChProc = mem_Get;
    gpi.unGetChProc = mem_UnGet;
    gpi.handlePtr = mem;
    gpi.key_otag = otag;
    gpi.encoding = ALPOENC_UTF8;
    type = BeginDeserialization(&gpi);
    back = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE);
    err = 0;
    PrefsDictionary(back, &err, ALPO_Deserialize, (IPTR)&gpi, TAG_DONE);
    check(type == ALPOT_Dictionary && err == 0 && same(back), "BeginDeserialization + ALPO_Deserialize");
    release(back);
    FreeVec(mem);

    /* Files */
    err = WritePrefs(dict, WRITEPREFS_FileName, (IPTR)"RAM:apptest-prefs.xml", TAG_DONE);
    back = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE);
    DictSetObjectForKey(back, num(1), "Stale");
    rc = ReadPrefs(back, READPREFS_FileName, (IPTR)"RAM:apptest-prefs.xml", TAG_DONE);
    check(err == 0 && rc == 0 && same(back) && !DictGetObjectForKey(back, "Stale"),
          "WritePrefs + ReadPrefs round trip (dictionary emptied first)");
    /* only the ".new" copy left, as after a reset between delete and rename */
    Rename("RAM:apptest-prefs.xml", "RAM:apptest-prefs.xml.new");
    rc = ReadPrefs(back, READPREFS_FileName, (IPTR)"RAM:apptest-prefs.xml", TAG_DONE);
    check(rc == 0 && same(back), "ReadPrefs falls back to name.new when name is missing");
    DeleteFile("RAM:apptest-prefs.xml.new");
    DeleteFile("RAM:apptest-prefs.xml");

    rc = ReadPrefs(back, READPREFS_FileName, (IPTR)"RAM:no-such-prefs.xml", TAG_DONE);
    count = -1;
    PrefsDictionary(back, NULL, ALPODICT_GetCount, (IPTR)&count, TAG_DONE);
    check(rc != 0 && count == 0, "ReadPrefs of a missing file fails and leaves it empty");
    release(back);

    check(DictGetIntegerForKey(NULL, "x", 5) == 5 && DictGetStringForKey(NULL, "x", NULL) == NULL,
          "Dict getters on NULL give the default");

    /* Reals of any size survive a save and reload */
    {
        double big = 3e9, tiny = 1e-12, neg = -123456.789, third = 1.0 / 3.0;

        PrefsDictionary(dict, NULL, ALPODICT_Clear, 0, TAG_DONE);
        DictSetObjectForKey(dict, PrefsNumber(NULL, NULL, ALPONUM_AllocSetDouble, (IPTR)&big, TAG_DONE), "big");
        DictSetObjectForKey(dict, PrefsNumber(NULL, NULL, ALPONUM_AllocSetDouble, (IPTR)&tiny, TAG_DONE), "tiny");
        DictSetObjectForKey(dict, PrefsNumber(NULL, NULL, ALPONUM_AllocSetDouble, (IPTR)&neg, TAG_DONE), "neg");
        DictSetObjectForKey(dict, PrefsNumber(NULL, NULL, ALPONUM_AllocSetDouble, (IPTR)&third, TAG_DONE), "third");
        mem = AllocVec(sizeof(*mem), MEMF_CLEAR);
        memset(&ppi, 0, sizeof(ppi));
        ppi.putChProc = mem_Put;
        ppi.handlePtr = mem;
        ppi.encoding = ALPOENC_UTF8;
        err = 0;
        PrefsDictionary(dict, &err, ALPO_Serialize, (IPTR)&ppi, TAG_DONE);
        back = err ? NULL : fromxml(mem, mem->buf, &err);
        check(back && err == 0 && realof(back, "big") == 3e9 && realof(back, "tiny") == 1e-12
              && realof(back, "neg") == -123456.789 && realof(back, "third") == 1.0 / 3.0,
              "reals 3e9, 1e-12, -123456.789 and 1/3 round-trip exactly");
        if (!back || err || realof(back, "third") != 1.0 / 3.0)
            printf("  err 0x%lx, %.*s\n", (unsigned long)err, mem->len > 400 ? 400 : (int)mem->len, mem->buf);
        if (back)
            release(back);
        FreeVec(mem);
    }

    /* A hostile file: deep nesting, a huge exponent, a comment ending in --->, the same object stored again */
    {
        static const char deep[] =
            "<dict><key>a</key>"
            "<array><array><array><array><array><array><array><array><array><array>"
            "<array><array><array><array><array><array><array><array><array><array>"
            "<array><array><array><array><array><array><array><array><array><array>"
            "<array><array><array><array><array><array><array><array><array><array>"
            "</array></array></array></array></array></array></array></array></array></array>"
            "</array></array></array></array></array></array></array></array></array></array>"
            "</array></array></array></array></array></array></array></array></array></array>"
            "</array></array></array></array></array></array></array></array></array></array>"
            "</dict>";
        LONG retain = 0;

        mem = AllocVec(sizeof(*mem), MEMF_CLEAR);
        back = fromxml(mem, deep, &err);
        check((err & ALPOEF_XMLERROR) != 0, "forty nested arrays are refused");
        if (!(err & ALPOEF_XMLERROR))
            printf("  err 0x%lx\n", (unsigned long)err);
        if (back)
            release(back);

        back = fromxml(mem, "<dict><key>r</key><real>1e2000000000</real><key>i</key><integer>1e2000000000</integer>"
                            "<key>m</key><real>-1e-2000000000</real></dict>", &err);
        check(back && err == 0 && realof(back, "r") == realof(back, "r") && realof(back, "r") <= 1.7976931348623157e308
              && realof(back, "r") > 1e308 && DictGetIntegerForKey(back, "i", 0) == 0x7fffffff
              && realof(back, "m") == 0, "a huge exponent gives a finite clamp, not a hang");
        if (back)
            release(back);

        back = fromxml(mem, "<?xml version=\"1.0\"?><!-- a comment ending oddly ---><dict><key>k</key><integer>5</integer></dict>",
                       &err);
        check(back && err == 0 && DictGetIntegerForKey(back, "k", 0) == 5, "a comment ending in ---> is skipped");
        if (back)
        {
            struct ALPOObjKey ok2;

            o = DictGetObjectForKey(back, "k");
            ok2.obj = o;
            ok2.key = "k";
            PrefsBaseObject(o, NULL, ALPO_Retain, 0, TAG_DONE);
            PrefsDictionary(back, NULL, ALPODICT_ReplaceObjForKey, (IPTR)&ok2, TAG_DONE);
            PrefsBaseObject(o, NULL, ALPO_GetRetainCount, (IPTR)&retain, TAG_DONE);
            check(retain == 1 && DictGetObjectForKey(back, "k") == o,
                  "storing the object a key already holds keeps one reference");
            /* the same without retaining first, as a careless caller would */
            ok2.obj = DictGetObjectForKey(back, "k");
            PrefsDictionary(back, NULL, ALPODICT_ReplaceObjForKey, (IPTR)&ok2, TAG_DONE);
            retain = 0;
            PrefsBaseObject(ok2.obj, NULL, ALPO_GetRetainCount, (IPTR)&retain, TAG_DONE);
            check(retain == 1 && DictGetObjectForKey(back, "k") == ok2.obj && DictGetIntegerForKey(back, "k", 0) == 5,
                  "storing a borrowed object over itself does not free it");
            release(back);
        }
        FreeVec(mem);
    }

    /* a whole-number real stays a real through its text form */
    {
        struct ALPOString as;
        TEXT text[40];
        double big = 3e9, got = 0;
        LONG type = 0;

        o = PrefsNumber(NULL, NULL, ALPONUM_AllocSetDouble, (IPTR)&big, TAG_DONE);
        as.string = text;
        as.length = sizeof(text);
        PrefsBaseObject(o, NULL, ALPO_GetAsString, (IPTR)&as, TAG_DONE);
        release(o);
        o = num(0);
        PrefsBaseObject(o, NULL, ALPO_SetWithString, (IPTR)text, TAG_DONE);
        PrefsNumber(o, NULL, ALPONUM_GetType, (IPTR)&type, ALPONUM_GetDouble, (IPTR)&got, TAG_DONE);
        check(type == ALPONUMT_Double && got == 3e9, "3e9 as text reads back as a real, not a saturated LONG");
        release(o);
    }
    release(dict);
}

static BOOL env_exists(CONST_STRPTR path)
{
    BPTR lock = Lock(path, SHARED_LOCK);

    if (lock)
        UnLock(lock);
    return lock != 0;
}

static BOOL envarc_writable(void)
{
    BPTR fh = Open("ENVARC:AppTest.writable", MODE_NEWFILE);

    if (fh)
    {
        Close(fh);
        DeleteFile("ENVARC:AppTest.writable");
    }
    return fh != BNULL;
}

static void test_appprefs(void)
{
    PrefsObject *p = NULL;
    ULONG id;

    DeleteFile("ENV:AppTest/PrefsApp.aros.org.xml");
    DeleteFile("ENVARC:AppTest/PrefsApp.aros.org.xml");

    id = RegisterApplication("PrefsApp", REGAPP_URLIdentifier, (IPTR)"aros.org",
                             REGAPP_LoadPrefs, TRUE, REGAPP_SavePrefs, TRUE,
                             REGAPP_ENVDir, (IPTR)"AppTest", TAG_DONE);
    GetApplicationAttrs(id, APPATTR_MainPrefsDict, (IPTR)&p, TAG_DONE);
    check(id && p && DictGetIntegerForKey(p, "Runs", 0) == 0, "REGAPP_LoadPrefs without a file: empty dictionary");
    DictSetObjectForKey(p, num(1), "Runs");
    SetApplicationAttrs(id, APPATTR_FlushPrefs, FALSE, TAG_DONE);
    check(!env_exists("ENV:AppTest/PrefsApp.aros.org.xml"), "APPATTR_FlushPrefs FALSE writes nothing");
    /* FALSE back when ENVARC: cannot be written, ENV: is written all the same */
    check(SetApplicationAttrs(id, APPATTR_FlushPrefs, TRUE, TAG_DONE) == envarc_writable()
          && env_exists("ENV:AppTest/PrefsApp.aros.org.xml"), "APPATTR_FlushPrefs writes ENV:[ENVDir/]ident.xml");
    DictSetObjectForKey(p, num(2), "Runs");
    UnregisterApplication(id, NULL);
    if (envarc_writable())
        check(env_exists("ENVARC:AppTest/PrefsApp.aros.org.xml"), "REGAPP_SavePrefs writes on unregister, to ENVARC: too");
    else
        printf("SKIP REGAPP_SavePrefs to ENVARC: (ENVARC: is not writable)\n");

    id = RegisterApplication("PrefsApp", REGAPP_URLIdentifier, (IPTR)"aros.org",
                             REGAPP_LoadPrefs, TRUE, REGAPP_ENVDir, (IPTR)"AppTest", TAG_DONE);
    p = NULL;
    GetApplicationAttrs(id, APPATTR_MainPrefsDict, (IPTR)&p, TAG_DONE);
    check(DictGetIntegerForKey(p, "Runs", 0) == 2, "the next run reads them back");
    p = NULL;
    check(ReadPrefs(PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE), READPREFS_AppID, id, TAG_DONE) == 0,
          "ReadPrefs by READPREFS_AppID");
    DeleteFile("ENV:AppTest/PrefsApp.aros.org.xml");
    SetApplicationAttrs(id, APPATTR_FlushPrefs, TRUE, TAG_DONE);
    check(env_exists("ENV:AppTest/PrefsApp.aros.org.xml"), "APPATTR_FlushPrefs writes without REGAPP_SavePrefs too");
    UnregisterApplication(id, NULL);

    DeleteFile("ENV:AppTest/PrefsApp.aros.org.xml");
    DeleteFile("ENVARC:AppTest/PrefsApp.aros.org.xml");
    DeleteFile("ENV:AppTest");
    DeleteFile("ENVARC:AppTest");
}

static void test_applib(ULONG self, struct MsgPort *selfport)
{
    struct ApplicationIconInfo aii = { APPICONT_CustomIcon, NULL };
    PrefsObject *arr = NULL;
    struct ALPOObjIndex oi;
    struct MsgPort *wport = NULL, *bport = NULL;
    ULONG watcher, blanker, other, allowed = 2, gm = 2, max = 0;
    LONG count = 0;
    BOOL found = FALSE;

    /* PrefsApp registered last, in test_appprefs(); AppTest before it */
    GetAppLibAttrs(APPLIBATTR_LastUsedAppsArray, (IPTR)&arr, TAG_DONE);
    PrefsArray(arr, NULL, ALPOARR_GetCount, (IPTR)&count, TAG_DONE);
    oi.index = 0;
    if (PrefsArray(arr, NULL, ALPOARR_GetObjAtIndex, (IPTR)&oi, TAG_DONE) && oi.obj)
        found = strcmp(DictGetStringForKey(oi.obj, LUA_APPNAME_KEY, ""), "PrefsApp") == 0;
    for (oi.index = 1; found && oi.index < count; oi.index++)
    {
        PrefsArray(arr, NULL, ALPOARR_GetObjAtIndex, (IPTR)&oi, TAG_DONE);
        if (strcmp(DictGetStringForKey(oi.obj, LUA_APPIDENTIFIER_KEY, ""), "AppTest.aros.org") == 0)
            break;
    }
    check(arr && found && oi.index < count, "last used applications: most recent first, AppTest listed");
    release(arr);

    GetAppLibAttrs(APPLIBATTR_LastUsedDocsMaxCount, (IPTR)&max, TAG_DONE);
    SetAppLibAttrs(0, APPLIBATTR_LastUsedDocsMaxCount, 3, TAG_DONE);
    SetApplicationAttrs(self, APPATTR_AppOpenedDocument, (IPTR)"RAM:d1", TAG_DONE);
    SetApplicationAttrs(self, APPATTR_AppOpenedDocument, (IPTR)"RAM:d2", TAG_DONE);
    SetApplicationAttrs(self, APPATTR_AppOpenedDocument, (IPTR)"RAM:d3", TAG_DONE);
    SetApplicationAttrs(self, APPATTR_AppOpenedDocument, (IPTR)"RAM:d4", TAG_DONE);
    SetApplicationAttrs(self, APPATTR_AppOpenedDocument, (IPTR)"RAM:d2", TAG_DONE);
    arr = NULL;
    GetApplicationAttrs(self, APPATTR_LastUsedDocsArray, (IPTR)&arr, TAG_DONE);
    count = 0;
    PrefsArray(arr, NULL, ALPOARR_GetCount, (IPTR)&count, TAG_DONE);
    oi.index = 0;
    PrefsArray(arr, NULL, ALPOARR_GetObjAtIndex, (IPTR)&oi, TAG_DONE);
    check(count == 3 && oi.obj && strcmp(DictGetStringForKey(oi.obj, LUD_DOCPATH_KEY, ""), "RAM:d2") == 0,
          "last used documents: most recent first, capped by LastUsedDocsMaxCount");
    release(arr);
    SetApplicationAttrs(self, APPATTR_ClearLastUsedDocs, TRUE, TAG_DONE);
    SetAppLibAttrs(0, APPLIBATTR_LastUsedDocsMaxCount, max ? max : 10, TAG_DONE);

    watcher = RegisterApplication("Watcher", REGAPP_AppNotifications, TRUE, REGAPP_NoIcon, TRUE, TAG_DONE);
    blanker = RegisterApplication("Blanker", REGAPP_BlankerNotifications, TRUE, REGAPP_NoIcon, TRUE, TAG_DONE);
    GetApplicationAttrs(watcher, APPATTR_Port, (IPTR)&wport, TAG_DONE);
    GetApplicationAttrs(blanker, APPATTR_Port, (IPTR)&bport, TAG_DONE);
    drain(wport, 0, NULL, 0);
    drain(bport, 0, NULL, 0);

    other = RegisterApplication("Other", TAG_DONE);
    check(drainfrom(wport, APPLIBMT_AppRegister, other, NULL, 0) == 1, "AppNotifications: AppRegister");

    SetApplicationAttrs(other, APPATTR_Hidden, TRUE, TAG_DONE);
    check(drainfrom(wport, APPLIBMT_AppDidChangeHidden, other, NULL, 0) == 1, "AppNotifications: AppDidChangeHidden");

    GetAppLibAttrs(APPLIBATTR_BlankerIsAllowed, (IPTR)&allowed, TAG_DONE);
    check(allowed == TRUE, "blanker allowed");
    SetApplicationAttrs(other, APPATTR_AllowsBlanker, FALSE, TAG_DONE);
    GetAppLibAttrs(APPLIBATTR_BlankerIsAllowed, (IPTR)&allowed, TAG_DONE);
    check(allowed == FALSE && drain(bport, APPLIBMT_BlankerDisallow, NULL, 0) == 1,
          "an application vetoes the blanker; blankers told");
    SendApplicationMsg(self, 0, NULL, APPLIBMT_BlankerBlank);
    check(drain(bport, APPLIBMT_BlankerBlank, NULL, 0) == 1 && drain(wport, APPLIBMT_BlankerBlank, NULL, 0) == 0
          && drain(selfport, APPLIBMT_BlankerBlank, NULL, 0) == 0, "blanker messages reach only blankers");

    SetApplicationAttrs(other, APPATTR_NeedsGameMode, TRUE, TAG_DONE);
    GetAppLibAttrs(APPLIBATTR_GameModeIsActive, (IPTR)&gm, TAG_DONE);
    check(gm == TRUE && Notify(self, APPNOTIFY_Text, (IPTR)"not now", TAG_DONE) == APPNOTIFY_ERROR_GAMEMODEACTIVE,
          "game mode: Notify() refuses");
    if (env_exists("SYS:System/Notifications"))
        check(Notify(self, APPNOTIFY_Text, (IPTR)"logged during game mode", APPNOTIFY_LogOnly, TRUE, TAG_DONE)
              == APPNOTIFY_OK_MSGQUEUED, "game mode: APPNOTIFY_LogOnly still logs");
    drain(selfport, 0, NULL, 0);

    check(LockApplicationIcon(other), "LockApplicationIcon");
    check(!SetApplicationAttrs(other, APPATTR_IconType, (IPTR)&aii, TAG_DONE)
          && drainfrom(wport, APPLIBMT_AppWantsChangeIconType, other, NULL, 0) == 1,
          "icon type locked: refused, docks asked to unlock");
    check(!UnregisterApplication(other, UNREGAPP_WaitForUnlock, FALSE, TAG_DONE),
          "UNREGAPP_WaitForUnlock FALSE fails while locked");
    UnlockApplicationIcon(other);
    check(SetApplicationAttrs(other, APPATTR_IconType, (IPTR)&aii, TAG_DONE)
          && drainfrom(wport, APPLIBMT_AppDidChangeIconType, other, NULL, 0) == 1,
          "unlocked: icon type changes, docks told");

    UnregisterApplication(other, NULL);
    GetAppLibAttrs(APPLIBATTR_BlankerIsAllowed, (IPTR)&allowed, APPLIBATTR_GameModeIsActive, (IPTR)&gm, TAG_DONE);
    check(drainfrom(wport, APPLIBMT_AppUnregister, other, NULL, 0) == 1 && allowed && !gm
          && drain(bport, APPLIBMT_BlankerAllow, NULL, 0) == 1,
          "unregister undoes game mode and the veto, docks told");
    drain(selfport, 0, NULL, 0);

    /* a dock that never unlocks: unregister goes ahead after five seconds,
       the icon information it read stays readable until its unlock */
    {
        struct ApplicationIconInfo *held = NULL;
        struct DateStamp before, after;
        ULONG id = RegisterApplication("Held", TAG_DONE);
        LONG ticks;
        BOOL unreg;

        check(id && LockApplicationIcon(id)
              && GetApplicationAttrs(id, APPATTR_IconType, (IPTR)&held, TAG_DONE) && held
              && held->iconType == APPICONT_ProgramIcon, "icon locked and read by a dock");
        DateStamp(&before);
        unreg = UnregisterApplication(id, NULL);
        DateStamp(&after);
        ticks = (after.ds_Days - before.ds_Days) * 24 * 60 * TICKS_PER_SECOND * 60
                + (after.ds_Minute - before.ds_Minute) * 60 * TICKS_PER_SECOND
                + (after.ds_Tick - before.ds_Tick);
        check(unreg && ticks >= 4 * TICKS_PER_SECOND && ticks < 8 * TICKS_PER_SECOND
              && FindApplication(FINDAPP_Name, (IPTR)"Held", TAG_DONE) == 0,
              "unregister with the icon still locked goes ahead after five seconds");
        check(held && held->iconType == APPICONT_ProgramIcon, "the icon information is still readable");
        UnlockApplicationIcon(id);
        drainfrom(wport, APPLIBMT_AppUnregister, id, NULL, 0);
    }

    UnregisterApplication(blanker, NULL);
    UnregisterApplication(watcher, NULL);
}

static void test_rexx(void)
{
    LONG rc2 = -1;

    RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0);
    check(rexx("RINGHIO", "REGISTERAPP APP=RexxTest", NULL) == 0, "ARexx REGISTERAPP at the RINGHIO port");
    check(FindApplication(FINDAPP_Name, (IPTR)"RexxTest", TAG_DONE) != 0, "ARexx registration is a real application");
    check(rexx("RINGHIO", "RINGHIO APP=RexxTest CLOSEONDC SCREEN=\"FRONT\" TITLE=\"ARexx\" IMGVALIGN=1 "
                          "TEXT=Hello from the RINGHIO port, as Hollywood sends it", NULL) == 0,
          "ARexx RINGHIO pop-up");
    check(rexx("RINGHIO", "NOTIFY APP=RexxTest PRI=2 TITLE=\"ARexx NOTIFY\" TEXT=The OS4 NOTIFY command", NULL) == 0,
          "ARexx NOTIFY");
    check(rexx("RINGHIO", "NOTIFY APP=NobodyHere TEXT=x", &rc2) == RC_ERROR && rc2 == APPNOTIFY_ERROR_APPNOTREGISTERED,
          "ARexx NOTIFY for an unregistered app: RC2 = APPNOTIFY_ERROR_APPNOTREGISTERED");
    check(rexx("RINGHIO", "UNREGISTERAPP APP=RexxTest", NULL) == 0
          && FindApplication(FINDAPP_Name, (IPTR)"RexxTest", TAG_DONE) == 0, "ARexx UNREGISTERAPP");
    check(rexx("RINGHIO", "NOSUCHCOMMAND", NULL) != 0, "ARexx unknown command fails");
    if (RexxSysBase)
        CloseLibrary((struct Library *)RexxSysBase);
}

static void test_blocked(void)
{
    PrefsObject *saved, *dict, *apps;
    ULONG id;
    BOOL had;

    /* Block one application in the server's settings, as Prefs/Notifications would. */
    saved = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE);
    had = ReadPrefs(saved, READPREFS_FileName, (IPTR)"Sys/Notifications.xml", READPREFS_ReadENVARC, FALSE,
                    TAG_DONE) == 0;
    dict = PrefsDictionary(saved, NULL, ALPO_Copy, 0, TAG_DONE);
    apps = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE);
    DictSetObjectForKey(apps, PrefsNumber(NULL, NULL, ALPONUM_AllocSetBool, FALSE, TAG_DONE), "Blocked");
    DictSetObjectForKey(dict, apps, "Applications");
    WritePrefs(dict, WRITEPREFS_FileName, (IPTR)"Sys/Notifications.xml", TAG_DONE);
    release(dict);
    Delay(25);

    id = RegisterApplication("Blocked", TAG_DONE);
    check(Notify(id, APPNOTIFY_Text, (IPTR)"should not show", TAG_DONE) == APPNOTIFY_ERROR_APPNOTALLOWED,
          "an application switched off in the preferences: APPNOTIFY_ERROR_APPNOTALLOWED");
    UnregisterApplication(id, NULL);

    if (had)
        WritePrefs(saved, WRITEPREFS_FileName, (IPTR)"Sys/Notifications.xml", TAG_DONE);
    else
        DeleteFile("ENV:Sys/Notifications.xml");
    release(saved);
    Delay(25);
}

static BOOL log_has(CONST_STRPTR what)
{
    char line[512];
    BOOL found = FALSE;
    BPTR fh = Open("T:Notifications.log", MODE_OLDFILE);

    if (!fh)
        return FALSE;
    while (!found && FGets(fh, line, sizeof(line)))
        found = strstr(line, what) != NULL;
    Close(fh);
    return found;
}

int main(void)
{
    struct RDArgs *rda;
    IPTR args[2] = { 0, 0 };
    LONG waitback = 0;
    STRPTR image = NULL;
    BOOL server;
    ULONG id1, id2, id3, id4, counter = 9;
    struct MsgPort *p1 = NULL, *p2 = NULL, *p3 = NULL;
    STRPTR ident = NULL;
    struct ApplicationCustomMsg cm;
    struct MinList *list;
    struct ApplicationNode *an;
    char text[64];
    int count;

    if (!(rda = ReadArgs("WAITBACK/K/N,IMAGE/K", args, NULL)))
    {
        PrintFault(IoErr(), "AppTest");
        return RETURN_FAIL;
    }
    if (args[0])
        waitback = *(LONG *)args[0];
    image = (STRPTR)args[1];

    if (!(ApplicationBase = OpenLibrary("application.library", 1)))
    {
        printf("FAIL cannot open application.library\n");
        return RETURN_FAIL;
    }

    id1 = RegisterApplication("AppTest", REGAPP_URLIdentifier, (IPTR)"www.aros.org",
                              REGAPP_Description, (IPTR)"application.library test", TAG_DONE);
    check(id1 != 0, "register first instance");
    GetApplicationAttrs(id1, APPATTR_AppIdentifier, (IPTR)&ident, APPATTR_Port, (IPTR)&p1, TAG_DONE);
    check(ident && strcmp(ident, "AppTest.aros.org") == 0, "identifier name.url, www. stripped");
    check(p1 != NULL, "first instance has a port");

    id2 = RegisterApplication("AppTest", REGAPP_URLIdentifier, (IPTR)"aros.org", TAG_DONE);
    ident = NULL;
    GetApplicationAttrs(id2, APPATTR_AppIdentifier, (IPTR)&ident, APPATTR_Port, (IPTR)&p2,
                        APPATTR_AppIdentifierCounter, (IPTR)&counter, TAG_DONE);
    check(id2 && id2 != id1 && ident && strcmp(ident, "AppTest_1.aros.org") == 0 && counter == 1,
          "second instance is AppTest_1.aros.org");
    check(!RegisterApplication("AppTest", REGAPP_URLIdentifier, (IPTR)"aros.org",
                               REGAPP_IdenticalNameCounter, FALSE, TAG_DONE),
          "REGAPP_IdenticalNameCounter FALSE: no third instance");

    check(FindApplication(FINDAPP_Name, (IPTR)"apptest", TAG_DONE) == id1, "find by name (first instance)");
    check(FindApplication(FINDAPP_AppIdentifier, (IPTR)"AppTest_1.aros.org", TAG_DONE) == id2,
          "find by identifier");
    check(FindApplication(FINDAPP_Name, (IPTR)"NoSuchApp", TAG_DONE) == 0, "find unknown is 0");

    memset(&cm, 0, sizeof(cm));
    strcpy(text, "hello");
    cm.customMsg = text;
    check(SendApplicationMsg(id1, id2, &cm.almsg, APPLIBMT_CustomMsg), "send custom message");
    strcpy(text, "clobbered");          /* the library must have copied it */
    text[0] = '\0';
    count = drain(p2, APPLIBMT_CustomMsg, text, sizeof(text));
    check(count == 1 && strcmp(text, "hello") == 0, "custom message arrives, string copied");

    check(SendApplicationMsg(id1, 0, NULL, APPLIBMT_ToFront), "broadcast");
    check(drain(p2, APPLIBMT_ToFront, NULL, 0) == 1 && drain(p1, APPLIBMT_ToFront, NULL, 0) == 0,
          "broadcast skips the sender");

    id3 = RegisterApplication("UniqApp", REGAPP_UniqueApplication, TRUE, TAG_DONE);
    GetApplicationAttrs(id3, APPATTR_Port, (IPTR)&p3, TAG_DONE);
    id4 = RegisterApplication("UniqApp", TAG_DONE);
    check(id3 && !id4, "second instance of a unique application refused");
    check(p3 && drain(p3, APPLIBMT_Unique, NULL, 0) == 1, "running instance told APPLIBMT_Unique");

    SetApplicationAttrs(id1, APPATTR_NeedsGameMode, TRUE, TAG_DONE);
    check(drain(p2, APPLIBMT_GameModeEntered, NULL, 0) == 1 && drain(p3, APPLIBMT_GameModeEntered, NULL, 0) == 1,
          "game mode broadcast to the others");
    SetApplicationAttrs(id1, APPATTR_NeedsGameMode, FALSE, TAG_DONE);
    drain(p2, APPLIBMT_GameModeLeft, NULL, 0);
    drain(p3, APPLIBMT_GameModeLeft, NULL, 0);

    list = GetApplicationList();
    count = 0;
    if (list)
        ForeachNode(list, an)
        {
            if ((strcmp(an->identifier, "AppTest.aros.org") == 0 && strcmp(an->name, "AppTest") == 0 && an->appID == id1)
                || strcmp(an->identifier, "AppTest_1.aros.org") == 0 || strcmp(an->name, "UniqApp") == 0)
                count++;
        }
    FreeApplicationList(list);
    check(count == 3, "application list has all three");

    test_prefsobjects();
    test_appprefs();
    test_applib(id1, p1);

    server = env_exists("SYS:System/Notifications");
    if (server)
    check(Notify(id1,
                 APPNOTIFY_Title,        (IPTR)"application.library",
                 APPNOTIFY_Text,         (IPTR)"Notifications work. Double-click to send a message back.",
                 APPNOTIFY_BackMsg,      (IPTR)"back-from-popup",
                 APPNOTIFY_CloseOnDC,    TRUE,
                 APPNOTIFY_ConfirmClose, TRUE,
                 APPNOTIFY_Pri,          waitback > 0 ? 10 : 0,
                 TAG_DONE) == APPNOTIFY_OK_MSGQUEUED, "Notify() shows a pop-up (server started on demand)");
    check(Notify(id1, APPNOTIFY_Pri, 3, TAG_DONE) == APPNOTIFY_ERROR_TAGS, "Notify() without title or text: _TAGS");
    check(Notify(0x7fffffff, APPNOTIFY_Text, (IPTR)"x", TAG_DONE) == APPNOTIFY_ERROR_APPNOTREGISTERED,
          "Notify() for no application: _APPNOTREGISTERED");
    if (!server)
    {
        struct DateStamp before, after;
        ULONG rc;
        LONG ticks;

        DateStamp(&before);
        rc = Notify(id1, APPNOTIFY_Text, (IPTR)"nobody is listening", TAG_DONE);
        DateStamp(&after);
        ticks = (after.ds_Days - before.ds_Days) * 24 * 60 * TICKS_PER_SECOND * 60
                + (after.ds_Minute - before.ds_Minute) * 60 * TICKS_PER_SECOND
                + (after.ds_Tick - before.ds_Tick);
        check(rc == APPNOTIFY_ERROR_SERVERNOTRUNNING && ticks < TICKS_PER_SECOND,
              "Notify() without SYS:System/Notifications: _SERVERNOTRUNNING within a second");
    }
    if (server)
        check(log_has("Notifications work.") && log_has("logged during game mode"),
              "every notification goes to T:Notifications.log");

    if (server && waitback > 0)
    {
        ULONG left = waitback * 50;
        BOOL got = FALSE, closed = FALSE;

        text[0] = '\0';
        while (left-- && !(got && closed))
        {
            struct ApplicationMsg *am;

            Delay(1);
            while ((am = (struct ApplicationMsg *)GetMsg(p1)))
            {
                if (am->type == APPLIBMT_CustomMsg)
                {
                    strncpy(text, ((struct ApplicationCustomMsg *)am)->customMsg, sizeof(text) - 1);
                    got = TRUE;
                }
                else if (am->type == APPLIBMT_ConfirmClose)
                    closed = TRUE;
                ReplyMsg(&am->msg);
            }
        }
        check(got && strcmp(text, "back-from-popup") == 0, "double-click sent APPNOTIFY_BackMsg back");
        check(closed, "closing it sent APPLIBMT_ConfirmClose");
    }

    if (server)
    {
        test_rexx();
        test_blocked();
    }
    else
        printf("SKIP pop-ups, ARexx and the settings (SYS:System/Notifications is not installed)\n");

    check(UnregisterApplication(id3, NULL) && UnregisterApplication(id2, NULL)
          && UnregisterApplication(id1, NULL), "unregister");
    check(FindApplication(FINDAPP_Name, (IPTR)"AppTest", TAG_DONE) == 0, "gone after unregister");
    check(!UnregisterApplication(id1, NULL), "unregister twice is refused");

    if (server && image)
    {
        ULONG id5 = RegisterApplication("AppTestImage", TAG_DONE);

        check(Notify(id5, APPNOTIFY_Title, (IPTR)"With an image",
                     APPNOTIFY_Text, (IPTR)"The picture is scaled to 32x32 and drawn with its alpha.",
                     APPNOTIFY_ImageFile, (IPTR)image, APPNOTIFY_ImageVertAlign, 1, TAG_DONE)
              == APPNOTIFY_OK_MSGQUEUED, "pop-up with an image");
        check(Notify(id5, APPNOTIFY_Title, (IPTR)"From an icon",
                     APPNOTIFY_Text, (IPTR)"APPNOTIFY_IconFile takes a Workbench icon.",
                     APPNOTIFY_IconFile, (IPTR)"SYS:Prefs/Asl.info", TAG_DONE)
              == APPNOTIFY_OK_MSGQUEUED, "pop-up with an icon");
        check(Notify(id5, APPNOTIFY_Title, (IPTR)"Urgent",
                     APPNOTIFY_Text, (IPTR)"Priority 10 stays until you close it.",
                     APPNOTIFY_Pri, 10, TAG_DONE)
              == APPNOTIFY_OK_MSGQUEUED, "priority 10 pop-up");
        UnregisterApplication(id5, NULL);
    }

    printf("APPTEST %s %d passed, %d failed\n", failed ? "FAILED" : "OK", passed, failed);

    CloseLibrary(ApplicationBase);
    FreeArgs(rda);
    return failed ? RETURN_ERROR : RETURN_OK;
}

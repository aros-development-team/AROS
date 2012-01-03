/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <proto/dos.h>
#include <proto/exec.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "prefsdata.h"

#include <aros/debug.h>

static struct TCPPrefs prefs;

struct Tokenizer
{
    STRPTR tokenizerLine;
    STRPTR token;
    FILE * tokenizedFile;
    BOOL newline;
    BOOL fend;
};

/* List of devices that require NOTRACKING option */
static STRPTR notrackingdevices[] = {"prm-rtl8029.device", NULL};

void OpenTokenFile(struct Tokenizer * tok, STRPTR FileName)
{
    tok->tokenizedFile = fopen(FileName, "r");
    tok->token = NULL;
    tok->newline = TRUE;
    if (!tok->tokenizedFile)
        tok->fend = TRUE;
    else
    {
        tok->tokenizerLine = malloc(8192);
        tok->fend = FALSE;
        tok->newline = TRUE;
    }
}

void CloseTokenFile(struct Tokenizer * tok)
{
    if (tok->tokenizedFile)
    {
        fclose(tok->tokenizedFile);
        free(tok->tokenizerLine);
        tok->fend = TRUE;
        tok->token = NULL;
    }
}

void GetNextToken(struct Tokenizer * tok, STRPTR tk)
{
    tok->newline = FALSE;
    if (tok->token != NULL)
    {
        tok->token = strtok(NULL, tk);
        if (!tok->token)
            GetNextToken(tok, tk);
    }
    else
    {
        tok->newline = TRUE;
        if (!feof(tok->tokenizedFile))
        {
            tok->tokenizerLine[0] = 0;
            fgets(tok->tokenizerLine, 8192, tok->tokenizedFile);
            if (tok->tokenizerLine == NULL)
            {
                tok->token = NULL;
                GetNextToken(tok, tk);
            }
            else
                tok->token = strtok(tok->tokenizerLine, tk);
        }
        else
            tok->fend = TRUE;
    }
}

void SetDefaultNetworkPrefsValues()
{
    LONG i;
    for (i = 0; i < MAXINTERFACES; i++)
    {
        InitInterface(GetInterface(i));
    }
    SetInterfaceCount(0);
    SetDomain(DEFAULTDOMAIN);
    SetHost(DEFAULTHOST);
    SetGate(DEFAULTGATE);
    SetDNS(0, DEFAULTDNS);
    SetDNS(1, DEFAULTDNS);
    SetDHCP(FALSE);

    SetAutostart(FALSE);
}

void SetDefaultWirelessPrefsValues()
{
    LONG i;
    for (i = 0; i < MAXNETWORKS; i++)
    {
        InitNetwork(GetNetwork(i));
    }
    SetNetworkCount(0);

    SetWirelessDevice(NULL);
}

void SetDefaultMobilePrefsValues()
{
    LONG i;
    for (i = 0; i < MAXATCOMMANDS; i++)
    {
        prefs.mobile.atcommand[i][0] = 0;
    }
    SetMobile_atcommand(0,"AT+CGDCONT=1,\"IP\",\"insert.your.apn.here\"");
    SetMobile_atcommand(1,"ATDT*99***1#");
    SetMobile_timeout( 10 );
    SetMobile_Autostart(FALSE);
    SetMobile_devicename( "usbmodem.device" );
    SetMobile_unit( 0 );
    SetMobile_username("");
    SetMobile_password("");
}

void InitInterface(struct Interface *iface)
{
    SetName(iface, DEFAULTNAME);
    SetIfDHCP(iface, TRUE);
    SetIP(iface, DEFAULTIP);
    SetMask(iface, DEFAULTMASK);
    SetDevice(iface, DEFAULTDEVICE);
    SetUnit(iface, 0);
    SetUp(iface, FALSE);
}

/* Returns TRUE if directory has been created or already existed */
BOOL RecursiveCreateDir(CONST_STRPTR dirpath)
{
    /* Will create directory even if top level directory does not exist */

    BPTR lock = BNULL;
    ULONG lastdirseparator = 0;
    ULONG dirpathlen = strlen(dirpath);
    STRPTR tmpdirpath = AllocVec(dirpathlen + 2, MEMF_CLEAR | MEMF_PUBLIC);

    CopyMem(dirpath, tmpdirpath, dirpathlen);

    /* Recurvice directory creation */
    while(TRUE)
    {
        if (lastdirseparator >= dirpathlen) break;

        for (; lastdirseparator < dirpathlen; lastdirseparator++)
            if (tmpdirpath[lastdirseparator] == '/') break;

        tmpdirpath[lastdirseparator] = '\0'; /* cut */

        /* Unlock any lock from previous interation. Last iteration lock will be returned. */
        if (lock != BNULL)
        {
            UnLock(lock);
            lock = BNULL;
        }

        /* Check if directory exists */
        lock = Lock(tmpdirpath, SHARED_LOCK);
        if (lock == BNULL)
        {
            lock = CreateDir(tmpdirpath);
            if (lock == BNULL)
                break; /* Error with creation */
        }

        tmpdirpath[lastdirseparator] = '/'; /* restore */
        lastdirseparator++;
    }

    FreeVec(tmpdirpath);

    if (lock == BNULL)
        return FALSE;
    else
    {
        UnLock(lock);
        lock = BNULL;
        return TRUE;
    }
}

/* Returns TRUE if selected device needs to use NOTRACKING option */
BOOL GetNoTracking(struct Interface *iface)
{
    STRPTR devicename = NULL;
    LONG pos = 0;
    TEXT devicepath[strlen(GetDevice(iface)) + 1];
    strcpy(devicepath, GetDevice(iface));
    strupr(devicepath);

    while ((devicename = notrackingdevices[pos++]) != NULL)
    {
        /* Comparison is done on upper case string so it is case insensitive */
        strupr(devicename);
        if (strstr(devicepath, devicename) != NULL)
            return TRUE;
    }

    return FALSE;
}

/* Puts part 1 into empty buffer */
VOID CombinePath1P(STRPTR dstbuffer, ULONG dstbufferlen, CONST_STRPTR part1)
{
    dstbuffer[0] = '\0'; /* Make sure buffer is treated as empty */
    AddPart(dstbuffer, part1, dstbufferlen);
}

/* Combines part1 with part2 into an empty buffer */
VOID CombinePath2P(STRPTR dstbuffer, ULONG dstbufferlen, CONST_STRPTR part1, CONST_STRPTR part2)
{
    CombinePath1P(dstbuffer, dstbufferlen, part1);
    AddPart(dstbuffer, part2, dstbufferlen);
}

/* Combines part1 with part2 with part3 into an empty buffer */
VOID CombinePath3P(STRPTR dstbuffer, ULONG dstbufferlen, CONST_STRPTR part1, CONST_STRPTR part2, CONST_STRPTR part3)
{
    CombinePath2P(dstbuffer, dstbufferlen, part1, part2);
    AddPart(dstbuffer, part3, dstbufferlen);
}

BOOL WriteNetworkPrefs(CONST_STRPTR  destdir)
{
    FILE *ConfFile;
    LONG i;
    struct Interface *iface;
    ULONG filenamelen = strlen(destdir) + 4 + 20;
    TEXT filename[filenamelen];
    ULONG destdbdirlen = strlen(destdir) + 3 + 1;
    TEXT destdbdir[destdbdirlen];
    LONG interfacecount = GetInterfaceCount();

    CombinePath2P(destdbdir, destdbdirlen, destdir, "db");

    /* Create necessary directories */
    if(!RecursiveCreateDir(destdir)) return FALSE;
    if(!RecursiveCreateDir(destdbdir)) return FALSE;

    /* Write variables */
    CombinePath2P(filename, filenamelen, destdir, "Config");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;
    fprintf(ConfFile, "%s/db", PREFS_PATH_ENV);
    fclose(ConfFile);

    CombinePath2P(filename, filenamelen, destdir, "AutoRun");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;
    fprintf(ConfFile, "%s", (GetAutostart()) ? "True" : "False");
    fclose(ConfFile);

    CombinePath2P(filename, filenamelen, destdir, "MobileAutorun");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;
    fprintf(ConfFile, "%s", (GetMobile_Autostart()) ? "True" : "False");
    fclose(ConfFile);

    /* Write configuration files */
    CombinePath2P(filename, filenamelen, destdbdir, "general.config");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;
    fprintf(ConfFile, "USELOOPBACK=YES\n");
    fprintf(ConfFile, "DEBUGSANA=NO\n");
    fprintf(ConfFile, "USENS=SECOND\n");
    fprintf(ConfFile, "GATEWAY=NO\n");
    fprintf(ConfFile, "HOSTNAME=%s.%s\n", GetHost(), GetDomain());
    fprintf(ConfFile, "LOG FILTERFILE=5\n");
    fprintf(ConfFile, "GUI PANEL=MUI\n");
    fprintf(ConfFile, "OPENGUI=YES\n");
    fclose(ConfFile);

    CombinePath2P(filename, filenamelen, destdbdir, "interfaces");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;
    for(i = 0; i < interfacecount; i++)
    {
        iface = GetInterface(i);
        fprintf
        (
            ConfFile, "%s DEV=%s UNIT=%d %s IP=%s NETMASK=%s %s\n",
            GetName(iface), GetDevice(iface), (int)GetUnit(iface),
            (GetNoTracking(iface) ? (CONST_STRPTR)"NOTRACKING" : (CONST_STRPTR)""),
            (GetIfDHCP(iface) ? (CONST_STRPTR)"DHCP" : GetIP(iface)),
            GetMask(iface),
            (GetUp(iface) ? (CONST_STRPTR)"UP" : (CONST_STRPTR)"")
        );
        if (strstr(GetDevice(iface), "atheros5000.device") != NULL
            || strstr(GetDevice(iface), "prism2.device") != NULL
            || strstr(GetDevice(iface), "realtek8180.device") != NULL)
        {
            SetWirelessDevice(GetDevice(iface));
            SetWirelessUnit(GetUnit(iface));
        }
    }
    fclose(ConfFile);

    CombinePath2P(filename, filenamelen, destdbdir, "netdb-myhost");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;

    for(i = 0; i < interfacecount; i++)
    {
        iface = GetInterface(i);
        fprintf
        (
            ConfFile, "HOST %s %s.%s %s\n",
            GetIP(iface), GetHost(), GetDomain(), GetHost()
        );
    }

    if (!GetDHCP())
    {
        // FIXME: old version wrote Gateway even when DHCP was enabled
        fprintf(ConfFile, "HOST %s gateway\n", GetGate());
        fprintf(ConfFile, "; Domain names\n");
        fprintf(ConfFile, "; Name servers\n");
        fprintf(ConfFile, "NAMESERVER %s\n", GetDNS(0));
        fprintf(ConfFile, "NAMESERVER %s\n", GetDNS(1));
    }
    fclose(ConfFile);

    CombinePath2P(filename, filenamelen, destdbdir, "static-routes");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;
    if (!GetDHCP())
    {
        // FIXME: old version wrote Gateway even when DHCP was enabled
        fprintf(ConfFile, "DEFAULT GATEWAY %s\n", GetGate());
    }
    fclose(ConfFile);

    CombinePath2P(filename, filenamelen, destdir, "WirelessAutoRun");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;
    fprintf(ConfFile, "%s", (GetWirelessDevice() != NULL) ? "True" : "False");
    fclose(ConfFile);

    if (GetWirelessDevice() != NULL)
    {
        CombinePath2P(filename, filenamelen, destdir, "WirelessDevice");
        ConfFile = fopen(filename, "w");
        if (!ConfFile) return FALSE;
        fprintf(ConfFile, "%s UNIT %ld", GetWirelessDevice(),
            (long int)GetWirelessUnit());
        fclose(ConfFile);
    }

    return TRUE;
}

BOOL WriteWirelessPrefs(CONST_STRPTR destdir)
{
    FILE *ConfFile;
    LONG i;
    struct Network *net;
    ULONG filenamelen = strlen(destdir) + 4 + 20;
    TEXT filename[filenamelen];

    /* Write wireless config */
    if (prefs.networkCount > 0)
    {
        CombinePath2P(filename, filenamelen, destdir, "Wireless.prefs");
        ConfFile = fopen(filename, "w");
        if (!ConfFile) return FALSE;

        for (i = 0; i < prefs.networkCount; i++)
        {
            net = &prefs.networks[i];
            fprintf(ConfFile, "network={\n");
            if (net->name[0] != '\0')
                fprintf(ConfFile, "\tssid=\"%s\"\n", net->name);
            switch (net->encType)
            {
                case 2:
                    if (net->keyIsHex)
                        fprintf(ConfFile, "\tpsk=%s\n", net->key);
                    else
                        fprintf(ConfFile, "\tpsk=\"%s\"\n", net->key);
                    fprintf(ConfFile, "\tkey_mgmt=WPA-PSK\n");
                    break;
                case 1:
                    if (net->keyIsHex)
                        fprintf(ConfFile, "\twep_key0=%s\n", net->key);
                    else
                        fprintf(ConfFile, "\twep_key0=\"%s\"\n", net->key);
                    fprintf(ConfFile, "\twep_tx_keyidx=0\n");
                default:
                    fprintf(ConfFile, "\tkey_mgmt=NONE\n");
            }
            if (net->hidden)
                fprintf(ConfFile, "\tscan_ssid=1\n");
            if (net->adHoc)
                fprintf(ConfFile, "\tmode=1\n");
            fprintf(ConfFile, "}\n\n");
        }

        fclose(ConfFile);
    }

    return TRUE;
}


BOOL WriteMobilePrefs(CONST_STRPTR destdir)
{
    FILE *ConfFile;
    LONG i;
    ULONG filenamelen = strlen(destdir) + 4 + 30;
    TEXT filename[filenamelen];

    CombinePath2P(filename, filenamelen, destdir, "MobileBroadband.prefs");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;

    if( strlen(GetMobile_devicename()) > 0 ) fprintf(ConfFile, "DEVICE %s\n" ,GetMobile_devicename() );
    fprintf(ConfFile, "UNIT %d\n" , (int)GetMobile_unit() );
    if( strlen(GetMobile_username()) > 0 ) fprintf(ConfFile, "USERNAME %s\n" ,GetMobile_username() );
    if( strlen(GetMobile_password()) > 0 ) fprintf(ConfFile, "PASSWORD %s\n" ,GetMobile_password() );

    for (i = 0; i < MAXATCOMMANDS; i++)
    {
        if( strlen( GetMobile_atcommand(i) ) > 0 ){
            fprintf(ConfFile, "SEND %s\n" ,GetMobile_atcommand(i) );
        }
    }

    fclose(ConfFile);

    return TRUE;
}


#define BUFSIZE 2048
BOOL CopyFile(CONST_STRPTR srcfile, CONST_STRPTR dstfile)
{
    BPTR from = BNULL, to = BNULL;
    TEXT buffer[BUFSIZE];

    if ((from = Open(srcfile, MODE_OLDFILE)))
    {
        if ((to = Open(dstfile, MODE_NEWFILE)))
        {
            LONG s = 0;

            do
            {
                if ((s = Read(from, buffer, BUFSIZE)) == -1)
                {
                    Close(to);
                    Close(from);
                    return FALSE;
                }

                if (Write(to, buffer, s) == -1)
                {
                    Close(to);
                    Close(from);
                    return FALSE;
                }
            } while (s == BUFSIZE);

            Close(to);
            Close(from);
            return TRUE;
        }

        Close(from);
    }

    return FALSE;
}

CONST_STRPTR GetDefaultStackLocation()
{
    /* Use static variable so that it is initialized only once (and can be returned) */
    static TEXT path [1024] = {0};

    /* Load path if needed - this will happen only once */
    if (path[0] == '\0')
    {
        GetVar(AROSTCP_PACKAGE_VARIABLE, path, 1024, LV_VAR);
    }

    return path;
}

BOOL IsStackRunning()
{
    return FindTask("bsdsocket.library") != NULL;
}

BOOL RestartStack()
{
    ULONG trycount = 0;

    /* Shutdown */
    if (IsStackRunning())
    {
        struct Task * arostcptask = FindTask("bsdsocket.library");
        if (arostcptask != NULL)
            Signal(arostcptask, SIGBREAKF_CTRL_C);
    }

    /* Check if shutdown successful */
    trycount = 0;
    while(IsStackRunning())
    {
        if (trycount > 4) return FALSE;
        Delay(50);
        trycount++;
    }

    /* Startup */
    {
        CONST_STRPTR srcdir = GetDefaultStackLocation();
        ULONG arostcppathlen = strlen(srcdir) + 3 + 20;
        TEXT arostcppath[arostcppathlen];
        struct TagItem tags[] =
        {
            { SYS_Input,        (IPTR)NULL          },
            { SYS_Output,       (IPTR)NULL          },
            { SYS_Error,        (IPTR)NULL          },
            { SYS_Asynch,       (IPTR)TRUE          },
            { TAG_DONE,         0                   }
        };

        CombinePath3P(arostcppath, arostcppathlen, srcdir, "C", "AROSTCP");

        SystemTagList(arostcppath, tags);
    }

    /* Check if startup successful */
    trycount = 0;
    while (!IsStackRunning())
    {
        if (trycount > 9) return FALSE;
        Delay(50);
        trycount++;
    }

    /* All ok */
    return TRUE;
}

BOOL StopWireless()
{
    ULONG trycount = 0;

    /* Shutdown */
    {
        struct Task *task = FindTask("C:WirelessManager");
        if (task != NULL)
            Signal(task, SIGBREAKF_CTRL_C);
    }

    /* Check if shutdown successful */
    trycount = 0;
    while(FindTask("C:WirelessManager") != NULL)
    {
        if (trycount > 4) return FALSE;
        Delay(50);
        trycount++;
    }

    /* All ok */
    return TRUE;
}

BOOL StartWireless()
{
    ULONG trycount = 0;
    TEXT command[80];

    /* Startup */
    {
        struct TagItem tags[] =
        {
            { SYS_Input,        (IPTR)NULL          },
            { SYS_Output,       (IPTR)NULL          },
            { SYS_Error,        (IPTR)NULL          },
            { SYS_Asynch,       (IPTR)TRUE          },
            { TAG_DONE,         0                   }
        };

        snprintf(command, 80, "C:WirelessManager \"%s\" UNIT %ld\n",
            GetWirelessDevice(), (long int)GetWirelessUnit());
        SystemTagList(command, tags);
    }

    /* Check if startup successful */
    trycount = 0;
    while(FindTask("C:WirelessManager") == NULL)
    {
        if (trycount > 9) return FALSE;
        Delay(50);
        trycount++;
    }

    /* All ok */
    return TRUE;
}

BOOL StopMobile()
{
    ULONG trycount = 0;

    /* Shutdown */
    {
        struct Task *task = FindTask("C:ModemManager");
        if (task != NULL)
            Signal(task, SIGBREAKF_CTRL_C);
    }

    /* Check if shutdown successful */
    trycount = 0;
    while(FindTask("C:ModemManager") != NULL)
    {
        if (trycount > 4) return FALSE;
        Delay(50);
        trycount++;
    }

    /* All ok */
    return TRUE;
}

BOOL StartMobile()
{
    ULONG trycount = 0;

    /* Startup */
    {
        struct TagItem tags[] =
        {
            { SYS_Input,        (IPTR)NULL          },
            { SYS_Output,       (IPTR)NULL          },
            { SYS_Error,        (IPTR)NULL          },
            { SYS_Asynch,       (IPTR)TRUE          },
            { TAG_DONE,         0                   }
        };

        SystemTagList("C:ModemManager", tags);
    }

    /* Check if startup successful */
    trycount = 0;
    while(FindTask("C:ModemManager") == NULL)
    {
        if (trycount > 9) return FALSE;
        Delay(50);
        trycount++;
    }

    /* All ok */
    return TRUE;
}

/* This is not a general use function! It assumes destinations directory exists */
BOOL AddFileFromDefaultStackLocation(CONST_STRPTR filename, CONST_STRPTR dstdir)
{
    /* Build paths */
    CONST_STRPTR srcdir = GetDefaultStackLocation();
    ULONG srcfilelen = strlen(srcdir) + 4 + strlen(filename) + 1;
    TEXT srcfile[srcfilelen];
    ULONG dstfilelen = strlen(dstdir) + 4 + strlen(filename) + 1;
    TEXT dstfile[dstfilelen];
    BPTR dstlock = BNULL;

    CombinePath3P(srcfile, srcfilelen, srcdir, "db", filename);
    CombinePath3P(dstfile, dstfilelen, dstdir, "db", filename);

    /* Check if the destination file already exists. If yes, do not copy */
    dstlock = Lock(dstfile, SHARED_LOCK);
    if (dstlock != BNULL)
    {
        UnLock(dstlock);
        return TRUE;
    }

    return CopyFile(srcfile, dstfile);
}

/* Copies files not created by prefs but needed to start stack */
BOOL CopyDefaultConfiguration(CONST_STRPTR destdir)
{
    ULONG destdbdirlen = strlen(destdir) + 3 + 1;
    TEXT destdbdir[destdbdirlen];
    CombinePath2P(destdbdir, destdbdirlen, destdir, "db");

    /* Create necessary directories */
    if (!RecursiveCreateDir(destdir)) return FALSE;
    if (!RecursiveCreateDir(destdbdir)) return FALSE;

    /* Copy files */
    if (!AddFileFromDefaultStackLocation("hosts", destdir)) return FALSE;
    if (!AddFileFromDefaultStackLocation("inet.access", destdir)) return FALSE;
    if (!AddFileFromDefaultStackLocation("netdb", destdir)) return FALSE;
    if (!AddFileFromDefaultStackLocation("networks", destdir)) return FALSE;
    if (!AddFileFromDefaultStackLocation("protocols", destdir)) return FALSE;
    if (!AddFileFromDefaultStackLocation("services", destdir)) return FALSE;

    return TRUE;
}

enum ErrorCode SaveNetworkPrefs()
{
    if (!CopyDefaultConfiguration(PREFS_PATH_ENVARC)) return NOT_COPIED_FILES_ENVARC;
    if (!WriteNetworkPrefs(PREFS_PATH_ENVARC)) return NOT_SAVED_PREFS_ENVARC;
    if (!WriteWirelessPrefs(WIRELESS_PATH_ENVARC)) return NOT_SAVED_PREFS_ENVARC;
    if (!WriteMobilePrefs(MOBILEBB_PATH_ENVARC)) return NOT_SAVED_PREFS_ENVARC;
    return UseNetworkPrefs();
}

enum ErrorCode UseNetworkPrefs()
{
    if (!CopyDefaultConfiguration(PREFS_PATH_ENV)) return NOT_COPIED_FILES_ENV;
    if (!WriteNetworkPrefs(PREFS_PATH_ENV)) return NOT_SAVED_PREFS_ENV;
    if (!WriteWirelessPrefs(WIRELESS_PATH_ENV)) return NOT_SAVED_PREFS_ENV;
    if (!WriteMobilePrefs(MOBILEBB_PATH_ENV)) return NOT_SAVED_PREFS_ENV;
    if(StopWireless())
        if (GetWirelessDevice() != NULL)
            if (!StartWireless()) return NOT_RESTARTED_WIRELESS;
    if (!RestartStack()) return NOT_RESTARTED_STACK;
    if(StopMobile())
        if (GetMobile_Autostart())
            if (!StartMobile()) return NOT_RESTARTED_MOBILE;
    return ALL_OK;
}

/* Directory points to top of config, so to AAA/AROSTCP not to AAA/AROSTCP/db */
void ReadNetworkPrefs(CONST_STRPTR directory)
{
    ULONG filenamelen = strlen(directory) + 4 + 20;
    TEXT filename[filenamelen];
    BOOL comment = FALSE;
    STRPTR tstring;
    struct Tokenizer tok;
    LONG interfacecount;
    struct Interface *iface = NULL;

    /* This function will not fail. It will load as much data as possible. Rest will be default values */

    SetDHCP(FALSE);

    CombinePath3P(filename, filenamelen, directory, "db", "general.config");
    OpenTokenFile(&tok, filename);
    while (!tok.fend)
    {
        if (tok.newline)
        { // read tokens from the beginning of line
            if (tok.token)
            {
                if (strcmp(tok.token, "HOSTNAME") == 0)
                {
                    GetNextToken(&tok, "=\n");
                    tstring = strchr(tok.token, '.');
                    SetDomain(tstring + 1);
                    tstring[0] = 0;
                    SetHost(tok.token);
                }
            }
        }
        GetNextToken(&tok, "=\n");
    }
    CloseTokenFile(&tok);

    CombinePath3P(filename, filenamelen, directory, "db", "interfaces");
    OpenTokenFile(&tok, filename);

    SetInterfaceCount(0);
    interfacecount = 0;

    while (!tok.fend && (interfacecount < MAXINTERFACES))
    {
        GetNextToken(&tok, " \n");
        if (tok.token)
        {
            if (tok.newline) comment = FALSE;
            if (strncmp(tok.token, "#", 1) == 0) comment = TRUE;

            if (!comment)
            {
                if (tok.newline)
                {
                    iface = GetInterface(interfacecount);
                    SetName(iface, tok.token);
                    interfacecount++;
                    SetInterfaceCount(interfacecount);
                }
                else if (strncmp(tok.token, "DEV=", 4) == 0)
                {
                    tstring = strchr(tok.token, '=');
                    SetDevice(iface, tstring + 1);
                }
                else if (strncmp(tok.token, "UNIT=", 5) == 0)
                {
                    tstring = strchr(tok.token, '=');
                    SetUnit(iface, atol(tstring + 1));
                }
                else if (strncmp(tok.token, "IP=", 3) == 0)
                {
                    tstring = strchr(tok.token, '=');
                    if (strncmp(tstring + 1, "DHCP", 4) == 0)
                    {
                        SetIfDHCP(iface, TRUE);
                        SetIP(iface, DEFAULTIP);
                    }
                    else
                    {
                        SetIP(iface, tstring + 1);
                        SetIfDHCP(iface, FALSE);
                    }
                }
                else if (strncmp(tok.token, "NETMASK=", 8) == 0)
                {
                    tstring = strchr(tok.token, '=');
                    SetMask(iface, tstring + 1);
                }
                else if (strncmp(tok.token, "UP", 2) == 0)
                {
                    SetUp(iface, TRUE);
                }
            }
        }
    }
    CloseTokenFile(&tok);

    CombinePath3P(filename, filenamelen, directory, "db", "netdb-myhost");
    OpenTokenFile(&tok, filename);
    int dnsc = 0;
    while (!tok.fend)
    {
        GetNextToken(&tok, " \n");
        if (tok.token)
        {
            // Host and Domain are already read from general.config
            if (strncmp(tok.token, "NAMESERVER", 10) == 0)
            {
                GetNextToken(&tok, " \n");
                SetDNS(dnsc, tok.token);
                dnsc++;
                if (dnsc > 1) dnsc = 1;
            }
        }
    }
    // Assume DHCP if there is no nameserver
    if (dnsc == 0)
    {
        SetDHCP(TRUE);
    }
    CloseTokenFile(&tok);

    CombinePath3P(filename, filenamelen, directory, "db", "static-routes");
    OpenTokenFile(&tok, filename);
    while (!tok.fend)
    {
        GetNextToken(&tok, " \n");
        if (tok.token)
        {
            if (strncmp(tok.token, "DEFAULT", 7) == 0)
            {
                GetNextToken(&tok, " \n");
                if (strncmp(tok.token, "GATEWAY", 7) == 0)
                {
                    GetNextToken(&tok, " \n");
                    SetGate(tok.token);
                }
            }
        }
    }
    CloseTokenFile(&tok);

    CombinePath2P(filename, filenamelen, directory, "Autorun");
    OpenTokenFile(&tok, filename);
    while (!tok.fend)
    {
        GetNextToken(&tok, " \n");
        if (tok.token)
        {
            if (strncmp(tok.token, "True", 4) == 0)
            {
                SetAutostart(TRUE);
                break;
            }
            else
            {
                SetAutostart(FALSE);
                break;
            }
        }
    }
    CloseTokenFile(&tok);

    CombinePath2P(filename, filenamelen, directory, "MobileAutorun");
    OpenTokenFile(&tok, filename);
    while (!tok.fend)
    {
        GetNextToken(&tok, " \n");
        if (tok.token)
        {
            if (strncmp(tok.token, "True", 4) == 0)
            {
                SetMobile_Autostart(TRUE);
                break;
            }
            else
            {
                SetMobile_Autostart(FALSE);
                break;
            }
        }
    }
    CloseTokenFile(&tok);

}

void ReadWirelessPrefs(CONST_STRPTR directory)
{
    ULONG filenamelen = strlen(directory) + 4 + 20;
    TEXT filename[filenamelen];
    BOOL comment = FALSE;
    STRPTR tstring;
    struct Tokenizer tok;
    LONG networkCount;
    struct Network *net = NULL;
    BOOL keyIsHex;

    CombinePath2P(filename, filenamelen, directory, "Wireless.prefs");
    OpenTokenFile(&tok, filename);

    SetNetworkCount(0);
    networkCount = 0;

    while (!tok.fend && (networkCount < MAXNETWORKS))
    {
        GetNextToken(&tok, " \n\t");
        if (tok.token)
        {
            if (tok.newline) comment = FALSE;
            if (strncmp(tok.token, "#", 1) == 0) comment = TRUE;

            if (!comment)
            {
                if (strncmp(tok.token, "network=", 8) == 0)
                {
                    net = GetNetwork(networkCount);
                    net->adHoc = FALSE;
                    net->hidden = FALSE;
                    networkCount++;
                    SetNetworkCount(networkCount);
                }
                else if (strncmp(tok.token, "ssid=", 5) == 0)
                {
                    tstring = strchr(tok.token, '=') + 2;
                    *strchr(tstring, '\"') = '\0';
                    SetNetworkName(net, tstring);
                }
                else if (strncmp(tok.token, "psk=", 4) == 0
                    || strncmp(tok.token, "wep_key0=", 9) == 0)
                {
                    tstring = strchr(tok.token, '=') + 1;
                    if (*tstring == '\"')
                    {
                        keyIsHex = FALSE;
                        tstring++;
                        *strchr(tstring, '\"') = '\0';
                    }
                    else
                        keyIsHex = TRUE;
                    SetKey(net, tstring, keyIsHex);
                    SetEncType(net, (*tok.token == 'p') ? 2 : 1);
                }
                else if (strncmp(tok.token, "scan_ssid=", 10) == 0)
                {
                    tstring = strchr(tok.token, '=') + 1;
                    SetHidden(net, *tstring == '1');
                }
                else if (strncmp(tok.token, "mode=", 5) == 0)
                {
                    tstring = strchr(tok.token, '=') + 1;
                    SetAdHoc(net, *tstring == '1');
                }
            }
        }
    }
    CloseTokenFile(&tok);
}


void ReadMobilePrefs(CONST_STRPTR directory)
{
    ULONG filenamelen = strlen(directory) + 4 + 30;
    TEXT filename[filenamelen];
    struct Tokenizer tok;
    LONG command=0;

    CombinePath2P(filename, filenamelen, directory, "MobileBroadband.prefs");
    OpenTokenFile(&tok, filename);
    while (!tok.fend)
    {
        GetNextToken(&tok, " \n");
        if (tok.token)
        {
            if ( tok.newline && tok.token[0] == '#' ) continue;

            if (tok.newline)
            {
                if (strcasecmp( tok.token, "SEND" ) == 0)
                {
                    GetNextToken(&tok, "\n");
                    if ( tok.token && ! tok.newline )
                    {
                        SetMobile_atcommand( command++ , tok.token );
                    }
                }
                else if (strcasecmp( tok.token, "DEVICE" ) == 0)
                {
                    GetNextToken(&tok, " \n");
                    if ( tok.token && ! tok.newline )
                    {
                        SetMobile_devicename( tok.token );
                    }
                }
                else if (strcasecmp( tok.token, "USERNAME" ) == 0)
                {
                    GetNextToken(&tok, " \n");
                    if ( tok.token && ! tok.newline )
                    {
                        SetMobile_username( tok.token );
                    }
                }
                else if (strcasecmp( tok.token, "PASSWORD" ) == 0)
                {
                    GetNextToken(&tok, " \n");
                    if ( tok.token && ! tok.newline )
                    {
                        SetMobile_password( tok.token );
                    }
                }
                else if (strcasecmp( tok.token, "UNIT" ) == 0)
                {
                    GetNextToken(&tok, " \n");
                    if ( tok.token && ! tok.newline )
                    {
                        SetMobile_unit( atoi( tok.token ) );
                    }
                }
            }
        }
    }
    CloseTokenFile(&tok);
}


void InitNetworkPrefs(CONST_STRPTR directory, BOOL use, BOOL save)
{
    SetDefaultNetworkPrefsValues();
    SetDefaultWirelessPrefsValues();
    SetDefaultMobilePrefsValues();

    ReadNetworkPrefs(directory);
    ReadWirelessPrefs(WIRELESS_PATH_ENV);
    ReadMobilePrefs(MOBILEBB_PATH_ENV);

    if (save)
    {
        SaveNetworkPrefs();
        return; /* save equals to use */
    }

    if (use)
    {
        UseNetworkPrefs();
    }
}

// check if 'str' contains only characters from 'accept'
BOOL IsLegal(STRPTR str, STRPTR accept)
{
    int i, len;

    if ((str == NULL) || (accept == NULL) || (str[0] == '\0'))
    {
        return FALSE;
    }

    len = strlen(str);
    for (i = 0; i < len; i++)
    {
        if (strchr(accept, str[i]) == NULL)
        {
            return FALSE;
        }
    }
    return TRUE;
}


/* Getters */

struct Interface * GetInterface(LONG index)
{
    return &prefs.interface[index];
}

STRPTR GetName(struct Interface *iface)
{
    return iface->name;
}

BOOL GetIfDHCP(struct Interface *iface)
{
    return iface->ifDHCP;
}

STRPTR GetIP(struct Interface *iface)
{
    return iface->IP;
}

STRPTR GetMask(struct Interface *iface)
{
    return iface->mask;
}

STRPTR GetDevice(struct Interface *iface)
{
    return iface->device;
}

LONG GetUnit(struct Interface *iface)
{
    return iface->unit;
}

BOOL GetUp(struct Interface *iface)
{
    return iface->up;
}


STRPTR GetGate(void)
{
    return prefs.gate;
}

STRPTR GetDNS(LONG m)
{
    return prefs.DNS[m];
}

STRPTR GetHost(void)
{
    return prefs.host;
}

STRPTR GetDomain(void)
{
    return prefs.domain;
}

LONG GetInterfaceCount(void)
{
    return prefs.interfacecount;
}

BOOL GetAutostart(void)
{
    return prefs.autostart;
}

BOOL GetDHCP(void)
{
    return prefs.DHCP;
}


/* Setters */

void SetInterface
(
    struct Interface *iface, STRPTR name, BOOL dhcp, STRPTR IP, STRPTR mask,
    STRPTR device, LONG unit, BOOL up
)
{
    SetName(iface, name);
    SetIfDHCP(iface, dhcp);
    SetIP(iface, IP);
    SetMask(iface, mask);
    SetDevice(iface, device);
    SetUnit(iface, unit);
    SetUp(iface, up);
}

void SetName(struct Interface *iface, STRPTR w)
{
    if (!IsLegal(w, NAMECHARS))
    {
        w = DEFAULTNAME;
    }
    strlcpy(iface->name, w, NAMEBUFLEN);
}

void SetIfDHCP(struct Interface *iface, BOOL w)
{
    iface->ifDHCP = w;
}

void SetIP(struct Interface *iface, STRPTR w)
{
    if (!IsLegal(w, IPCHARS))
    {
        w = DEFAULTIP;
    }
    strlcpy(iface->IP, w, IPBUFLEN);
}

void SetMask(struct Interface *iface, STRPTR  w)
{
    if (!IsLegal(w, IPCHARS))
    {
        w = DEFAULTMASK;
    }
    strlcpy(iface->mask, w, IPBUFLEN);
}

void SetDevice(struct Interface *iface, STRPTR w)
{
    if (w == NULL || w[0] == '\0')
    {
        w = DEFAULTDEVICE;
    }
    strlcpy(iface->device, w, NAMEBUFLEN);
}

void SetUnit(struct Interface *iface, LONG w)
{
    iface->unit = w;
}

void SetUp(struct Interface *iface, BOOL w)
{
    iface->up = w;
}


void SetGate(STRPTR  w)
{
    if (!IsLegal(w, IPCHARS))
    {
        w = DEFAULTGATE;
    }
    strlcpy(prefs.gate, w, IPBUFLEN);
}

void SetDNS(LONG m, STRPTR w)
{
    if (!IsLegal(w, IPCHARS))
    {
        w = DEFAULTDNS;
    }
    strlcpy(prefs.DNS[m], w, IPBUFLEN);
}

void SetHost(STRPTR w)
{
    if (!IsLegal(w, NAMECHARS))
    {
        w = DEFAULTHOST;
    }
    strlcpy(prefs.host, w, NAMEBUFLEN);
}

void SetDomain(STRPTR w)
{
    if (!IsLegal(w, NAMECHARS))
    {
        w = DEFAULTDOMAIN;
    }
    strlcpy(prefs.domain, w, NAMEBUFLEN);
}

void SetInterfaceCount(LONG w)
{
    prefs.interfacecount = w;
}

void SetAutostart(BOOL w)
{
    prefs.autostart = w;
}

void SetDHCP(BOOL w)
{
    prefs.DHCP = w;
}

void InitNetwork(struct Network *net)
{
    SetNetworkName(net, "");
    SetKey(net, "", FALSE);
    SetEncType(net, 0);
    SetAdHoc(net, FALSE);
}


/* Getters */

struct Network *GetNetwork(LONG index)
{
    return &prefs.networks[index];
}

STRPTR GetNetworkName(struct Network *net)
{
    return net->name;
}

UWORD GetEncType(struct Network *net)
{
    return net->encType;
}

STRPTR GetKey(struct Network *net)
{
    return net->key;
}

BOOL GetHidden(struct Network *net)
{
    return net->hidden;
}

BOOL GetAdHoc(struct Network *net)
{
    return net->adHoc;
}

LONG GetNetworkCount(void)
{
    return prefs.networkCount;
}

STRPTR GetWirelessDevice(void)
{
    return prefs.wirelessDevice;
}

LONG GetWirelessUnit(void)
{
    return prefs.wirelessUnit;
}

BOOL GetMobile_Autostart(void)
{
    return prefs.mobile.autostart;
}

STRPTR GetMobile_atcommand(ULONG i)
{
    if( i < MAXATCOMMANDS )
        return prefs.mobile.atcommand[i];
    else return "";
}

LONG GetMobile_atcommandcount(void)
{
    ULONG count=0;
    ULONG i;
    for (i=0;i<MAXATCOMMANDS;i++)
    {
        if( prefs.mobile.atcommand[i][0] != 0 ) count++;
    }
    return count;
}

STRPTR GetMobile_devicename(void)
{
    return prefs.mobile.devicename;
}

STRPTR GetMobile_username(void)
{
    return prefs.mobile.username;
}

STRPTR GetMobile_password(void)
{
    return prefs.mobile.password;
}

LONG GetMobile_unit(void)
{
    return prefs.mobile.unit;
}

LONG GetMobile_timeout(void)
{
    return prefs.mobile.timeout;
}

/* Setters */

void SetNetwork
(
    struct Network *net, STRPTR name, UWORD encType, STRPTR key,
    BOOL keyIsHex, BOOL hidden, BOOL adHoc
)
{
    SetNetworkName(net, name);
    SetEncType(net, encType);
    SetKey(net, key, keyIsHex);
    SetHidden(net, hidden);
    SetAdHoc(net, adHoc);
}

void SetNetworkName(struct Network *net, STRPTR w)
{
    strlcpy(net->name, w, SSIDBUFLEN);
}

void SetEncType(struct Network *net, UWORD w)
{
    net->encType = w;
}

void SetKey(struct Network *net, STRPTR w, BOOL keyIsHex)
{
    strlcpy(net->key, w, KEYBUFLEN);
    net->keyIsHex = keyIsHex;
}

void SetHidden(struct Network *net, BOOL w)
{
    net->hidden = w;
}

void SetAdHoc(struct Network *net, BOOL w)
{
    net->adHoc = w;
}

void SetNetworkCount(LONG w)
{
    prefs.networkCount = w;
}

void SetWirelessDevice(STRPTR w)
{
    prefs.wirelessDevice = w;
}

void SetWirelessUnit(LONG w)
{
    prefs.wirelessUnit = w;
}

void SetMobile_Autostart(BOOL w)
{
    prefs.mobile.autostart =w;
}

void SetMobile_atcommand(ULONG i,STRPTR w)
{
    if( strlen(w) < NAMEBUFLEN && i >= 0 && i < MAXATCOMMANDS ){
        strcpy(prefs.mobile.atcommand[i], w);
    }
}

void SetMobile_devicename(STRPTR w)
{
    if( strlen(w) < NAMEBUFLEN ) strcpy( prefs.mobile.devicename , w );
}

void SetMobile_username(STRPTR w)
{
    if( strlen(w) < NAMEBUFLEN ) strcpy( prefs.mobile.username , w );
}

void SetMobile_password(STRPTR w)
{
    if( strlen(w) < NAMEBUFLEN ) strcpy( prefs.mobile.password , w );
}

void SetMobile_unit(LONG w)
{
    prefs.mobile.unit = w;
}

void SetMobile_timeout(LONG w)
{
    prefs.mobile.timeout = w;
}






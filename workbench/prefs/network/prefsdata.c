/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <proto/dos.h>
#include <proto/exec.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "prefsdata.h"

#define DEFAULTIP "192.168.0.188"
#define DEFAULTMASK "255.255.255.0"
#define DEFAULTGATE "192.168.0.1"
#define DEFAULTDNS "192.168.0.1"
#define DEFAULTDEVICE "DEVS:networks/pcnet32.device"
#define DEFAULTHOST "arosbox"
#define DEFAULTDOMAIN "arosnet"

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
    SetIP(DEFAULTIP);
    SetMask(DEFAULTMASK);
    SetGate(DEFAULTGATE);
    SetDNS(0, DEFAULTDNS);
    SetDNS(1, DEFAULTDNS);
    SetDevice(DEFAULTDEVICE);
    SetHost(DEFAULTHOST);
    SetDomain(DEFAULTDOMAIN);
    SetDHCP(FALSE);
    SetAutostart(FALSE);
}

/* Returns TRUE if directory has been created or already existed */
BOOL RecursiveCreateDir(CONST_STRPTR dirpath)
{
    /* Will create directory even if top level directory does not exist */
    
    BPTR lock = NULL;
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
        if (lock != NULL)
        {
            UnLock(lock);
            lock = NULL;
        }

        /* Check if directory exists */
        lock = Lock(tmpdirpath, SHARED_LOCK);
        if (lock == NULL)
        {
            lock = CreateDir(tmpdirpath);
            if (lock == NULL)
                break; /* Error with creation */
        }
    
        tmpdirpath[lastdirseparator] = '/'; /* restore */
        lastdirseparator++;
    }
    
    FreeVec(tmpdirpath);
    
    if (lock == NULL)
        return FALSE;
    else
    {
        UnLock(lock);
        lock = NULL;
        return TRUE;
    }
}

/* Returns TRUE if selected device needs to use NOTRACKING option */
BOOL GetNoTracking()
{
    STRPTR devicename = NULL;
    LONG pos = 0;
    TEXT devicepath[strlen(GetDevice()) + 1];
    strcpy(devicepath, GetDevice());
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
    ULONG filenamelen = strlen(destdir) + 4 + 20;
    TEXT filename[filenamelen];
    ULONG destdbdirlen = strlen(destdir) + 3 + 1;
    TEXT destdbdir[destdbdirlen];
    
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
    fprintf(ConfFile,"eth0 DEV=%s UNIT=0 %s IP=%s NETMASK=%s UP\n", GetDevice(),
        (GetNoTracking() ? (CONST_STRPTR)"NOTRACKING" : (CONST_STRPTR)""),
        (GetDHCP() ? (CONST_STRPTR)"DHCP" : GetIP()), 
        GetMask());

    fclose(ConfFile);

    CombinePath2P(filename, filenamelen, destdbdir, "netdb-myhost");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;
    fprintf(ConfFile, "HOST %s %s.%s %s\n", GetIP(), GetHost(), GetDomain(), GetHost());
    fprintf(ConfFile, "HOST %s gateway\n", GetGate());
    if (!GetDHCP())
    {
        fprintf(ConfFile, "; Domain names\n");
        fprintf(ConfFile, "; Name servers\n");
        fprintf(ConfFile, "NAMESERVER %s\n", GetDNS(0));
        fprintf(ConfFile, "NAMESERVER %s\n", GetDNS(1));
    }
    fclose(ConfFile);

    CombinePath2P(filename, filenamelen, destdbdir, "static-routes");
    ConfFile = fopen(filename, "w");
    if (!ConfFile) return FALSE;
    fprintf(ConfFile, "DEFAULT GATEWAY %s\n", GetGate());
    fclose(ConfFile);

    return TRUE;
}

#define BUFSIZE 2048
BOOL CopyFile(CONST_STRPTR srcfile, CONST_STRPTR dstfile)
{   
    BPTR from = NULL, to = NULL;
    TEXT buffer[BUFSIZE];

    if ((from = Open(srcfile, MODE_OLDFILE)))
    {
        if ((to = Open(dstfile, MODE_NEWFILE)))
        {
            LONG	s=0;

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
    struct Library * socketlib = NULL;

    if ((socketlib = OpenLibrary("bsdsocket.library", 0L)) != NULL)
    {
        CloseLibrary(socketlib);
        return TRUE;
    }
    
    return FALSE;
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

/* This is not a general use function! It assumes destinations directory exists */
BOOL AddFileFromDefaultStackLocation(CONST_STRPTR filename, CONST_STRPTR dstdir)
{
    /* Build paths */
    CONST_STRPTR srcdir = GetDefaultStackLocation();
    ULONG srcfilelen = strlen(srcdir) + 4 + strlen(filename) + 1;
    TEXT srcfile[srcfilelen];
    ULONG dstfilelen = strlen(dstdir) + 4 + strlen(filename) + 1;
    TEXT dstfile[dstfilelen];
    BPTR dstlock = NULL;

    CombinePath3P(srcfile, srcfilelen, srcdir, "db", filename);
    CombinePath3P(dstfile, dstfilelen, dstdir, "db", filename);

    /* Check if the destination file already exists. If yes, do not copy */
    dstlock = Lock(dstfile, SHARED_LOCK);
    if (dstlock != NULL)
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
    return UseNetworkPrefs();
}

enum ErrorCode UseNetworkPrefs()
{
    if (!CopyDefaultConfiguration(PREFS_PATH_ENV)) return NOT_COPIED_FILES_ENV;
    if (!WriteNetworkPrefs(PREFS_PATH_ENV)) return NOT_SAVED_PREFS_ENV;
    if (!RestartStack()) return NOT_RESTARTED_STACK;
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

    /* This function will not fail. It will load as much data as possible. Rest will be default values */

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
    /* Reads only first uncommented interface */
    while (!tok.fend)
    {
        GetNextToken(&tok, " \n");
        if (tok.token)
        {
            if (tok.newline) comment = FALSE;
            if (strncmp(tok.token, "#", 1) == 0) comment = TRUE;

            if (!comment)
            {
                if (strncmp(tok.token, "DEV=", 4) == 0)
                {
                    tstring = strchr(tok.token, '=');
                    SetDevice(tstring + 1);
                }
                if (strncmp(tok.token, "IP=", 3) == 0)
                {
                    tstring = strchr(tok.token, '=');
                    if (strncmp(tstring + 1, "DHCP", 4) == 0)
                    {
                        SetDHCP(TRUE);
                        SetIP("192.168.0.188");
                    }
                    else
                    {
                        SetIP(tstring + 1);
                        SetDHCP(FALSE);
                    }
                }
                if (strncmp(tok.token, "NETMASK=", 8) == 0)
                {
                    tstring = strchr(tok.token, '=');
                    SetMask(tstring + 1);
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
            if (strncmp(tok.token, "NAMESERVER", 4) == 0)
            {
                GetNextToken(&tok, " \n");
                SetDNS(dnsc, tok.token);
                dnsc++;
                if (dnsc > 1) dnsc = 1;
            }
        }
    }
    CloseTokenFile(&tok);

    CombinePath3P(filename, filenamelen, directory, "db", "static-routes"); 
    OpenTokenFile(&tok, filename);
    while (!tok.fend)
    {
        GetNextToken(&tok, " \n");
        if (tok.token)
        {
            if (strncmp(tok.token, "DEFAULT", 4) == 0)
            {
                GetNextToken(&tok, " \n");
                if (strncmp(tok.token, "GATEWAY", 4) == 0) 
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
}

void InitNetworkPrefs(CONST_STRPTR directory, BOOL use, BOOL save)
{
    SetDefaultNetworkPrefsValues();

    ReadNetworkPrefs(directory);
    
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

    if ((str == NULL) || (accept == NULL))
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

BOOL IsName(STRPTR w)
{
    if ((w == NULL) || (w[0] == '\0') || strspn(w, NAMECHARS))
    {
        return FALSE;
    }
    return TRUE;
}

/* Getters */

STRPTR GetIP()
{
    return prefs.IP;
}

STRPTR GetMask()
{
    return prefs.mask;
}

STRPTR GetGate()
{
    return prefs.gate;
}

STRPTR GetDNS(LONG m)
{
    return prefs.DNS[m];
}

BOOL GetDHCP()
{
    return prefs.DHCP;
}

STRPTR GetDevice()
{
    return prefs.device;
}

STRPTR GetHost()
{
    return prefs.host;
}

STRPTR GetDomain()
{
    return prefs.domain;
}

BOOL GetAutostart()
{
    return prefs.autostart;
}

/* Setters */

void SetIP(STRPTR w)
{
    if (!IsLegal(w, IPCHARS))
    {
        w = DEFAULTIP;
    }
    strlcpy(prefs.IP, w, IPBUFLEN);
}

void SetMask(STRPTR  w)
{
    if (!IsLegal(w, IPCHARS))
    {
        w = DEFAULTMASK;
    }
    strlcpy(prefs.mask, w, IPBUFLEN);
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

void SetDHCP(BOOL w)
{
    prefs.DHCP = w;
}

void SetDevice(STRPTR w)
{
    if (w == NULL || w[0] == '\0')
    {
        w = DEFAULTDEVICE;
    }
    strlcpy(prefs.device, w, NAMEBUFLEN);
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

void SetAutostart(BOOL w)
{
    prefs.autostart = w;
}

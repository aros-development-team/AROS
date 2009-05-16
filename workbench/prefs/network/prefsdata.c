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

#define PREFS_PATH_ENV              "ENV:AROSTCP"
#define PREFS_PATH_ENVARC           "ENVARC:AROSTCP"
#define AROSTCP_PACKAGE_VARIABLE    "SYS/Packages/AROSTCP"

static struct TCPPrefs prefs;

struct Tokenizer
{
    STRPTR tokenizerLine;
    STRPTR token;
    FILE * tokenizedFile;
    BOOL newline;
    BOOL fend;
};

void OpenTokenFile(struct Tokenizer * tok, STRPTR FileName)
{
	tok->tokenizedFile = fopen(FileName, "r");
	tok->token = NULL;
	tok->newline = TRUE;
	if (!tok->tokenizedFile)
		tok->fend = TRUE;
	else{
		tok->tokenizerLine = malloc(8192);
		tok->fend = FALSE;
		tok->newline = TRUE;
	}
}

void CloseTokenFile(struct Tokenizer * tok)
{
	if (tok->tokenizedFile) {
		fclose(tok->tokenizedFile);
		free(tok->tokenizerLine);
		tok->fend = TRUE;
		tok->token = NULL;
	}
}

void GetNextToken(struct Tokenizer * tok, STRPTR tk)
{
	tok->newline = FALSE;
	if (tok->token != NULL) {
		tok->token = strtok(NULL, tk);
		if (!tok->token)
			GetNextToken(tok, tk);
	}else {
		tok->newline = TRUE;
		if (!feof(tok->tokenizedFile)) {
			tok->tokenizerLine[0] = 0;
			fgets(tok->tokenizerLine, 8192, tok->tokenizedFile);
			if (tok->tokenizerLine == NULL) {
				tok->token = NULL;
				GetNextToken(tok, tk);
			}else
				tok->token = strtok(tok->tokenizerLine, tk);
		}else
			tok->fend = TRUE;
	}
}

void SetDefaultValues()
{
	SetIP("192.168.0.188");
	SetMask("255.255.255.0");
	SetGate("192.168.0.1");
	SetDNS(0, "192.168.0.1");
	SetDNS(1, "192.168.0.1");
	SetDevice("DEVS:networks/pcnet32.device");
	SetHost("arosbox");
	SetDomain("arosnet");
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

BOOL WriteConfigVariables()
{
    FILE * configfile = NULL;

    /* Create necessary directories */
    if(!RecursiveCreateDir(PREFS_PATH_ENV)) return FALSE;
    if(!RecursiveCreateDir(PREFS_PATH_ENVARC)) return FALSE;
    
    /* Write configuration files */

    /* Config */
    configfile = fopen(PREFS_PATH_ENV"/Config", "w");
    if (!configfile) return FALSE;
    fprintf(configfile, "%s/db", PREFS_PATH_ENV);
    fclose(configfile);

    configfile = fopen(PREFS_PATH_ENVARC"/Config", "w");
    if (!configfile) return FALSE;
    fprintf(configfile, "%s/db", PREFS_PATH_ENV);
    fclose(configfile);

    return TRUE;
}

BOOL WriteNetworkPrefs(CONST_STRPTR  DestDir)
{
    FILE *ConfFile;
    TEXT FileName[strlen(DestDir) + 4 + 20];
    TEXT destdbdir[strlen(DestDir) + 3 + 1];
    sprintf(destdbdir, "%s/db", DestDir);

    /* Create necessary directories */
    if(!RecursiveCreateDir(DestDir)) return FALSE;
    if(!RecursiveCreateDir(destdbdir)) return FALSE;

    /* Write configuration files */
    sprintf(FileName, "%s/AutoRun", DestDir);
    ConfFile = fopen(FileName, "w");
    if (!ConfFile) return FALSE;
    fprintf(ConfFile, "%s", (GetAutostart()) ? "True" : "False");
    fclose(ConfFile);

    sprintf(FileName, "%s/db/DHCP", DestDir);
    ConfFile = fopen(FileName, "w");
    if (!ConfFile) return FALSE;
    fprintf(ConfFile, "%s", (GetDHCP()) ? "True" : "False");
    fclose(ConfFile);

    sprintf(FileName, "%s/db/general.config", DestDir);
    ConfFile = fopen(FileName, "w");
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

    sprintf(FileName, "%s/db/interfaces", DestDir);
    ConfFile = fopen(FileName, "w");
    if (!ConfFile) return FALSE;
    fprintf(ConfFile,"eth0 DEV=%s UNIT=0 NOTRACKING IP=%s NETMASK=%s UP\n", GetDevice(), GetIP(), GetMask());

    fclose(ConfFile);

    sprintf(FileName, "%s/db/netdb-myhost", DestDir);
    ConfFile = fopen(FileName, "w");
    if (!ConfFile) return 0;
    fprintf(ConfFile, "HOST %s %s.%s %s\n", GetIP(), GetHost(), GetDomain(), GetHost());
    fprintf(ConfFile, "HOST %s gateway\n", GetGate());
    fprintf(ConfFile, "; Domain names\n");
    fprintf(ConfFile, "; Name servers\n");
    fprintf(ConfFile, "NAMESERVER %s\n", GetDNS(0));
    fprintf(ConfFile, "NAMESERVER %s\n", GetDNS(1));
    fclose(ConfFile);

    sprintf(FileName, "%s/db/static-routes", DestDir);
    ConfFile = fopen(FileName, "w");
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

    if((from = Open(srcfile, MODE_OLDFILE)))
    {
        if((to = Open(dstfile, MODE_NEWFILE)))
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
    
    /* Load path if needed */
    if (path[0] == '\0')
    {
        GetVar(AROSTCP_PACKAGE_VARIABLE, path, 1024, LV_VAR);
    }

    return path;
}

/* This is not a general use function! It assumes destinations directory exists */
BOOL CopyFileFromDefaultStackLocation(CONST_STRPTR filename, CONST_STRPTR dstdir)
{
    /* Build paths */
    CONST_STRPTR srcdir = GetDefaultStackLocation();
    TEXT srcfile[strlen(srcdir) + 4 + strlen(filename) + 1];
    TEXT dstfile[strlen(dstdir) + 4 + strlen(filename) + 1];

    sprintf(srcfile, "%s/db/%s", srcdir, filename);
    sprintf(dstfile, "%s/db/%s", dstdir, filename);

    return CopyFile(srcfile, dstfile);
}

/* Copies files not created by prefs but needed to start stack */
BOOL CopyDefaultConfiguration(CONST_STRPTR destdir)
{
    TEXT destdbdir[strlen(destdir) + 3 + 1];
    sprintf(destdbdir, "%s/db", destdir);

    /* Create necessary directories */
    if (!RecursiveCreateDir(destdir)) return FALSE;
    if (!RecursiveCreateDir(destdbdir)) return FALSE;
    
    /* Copy files */
    if (!CopyFileFromDefaultStackLocation("hosts", destdir)) return FALSE;
    if (!CopyFileFromDefaultStackLocation("inet.access", destdir)) return FALSE;
    if (!CopyFileFromDefaultStackLocation("netdb", destdir)) return FALSE;
    if (!CopyFileFromDefaultStackLocation("networks", destdir)) return FALSE;
    if (!CopyFileFromDefaultStackLocation("protocols", destdir)) return FALSE;
    if (!CopyFileFromDefaultStackLocation("services", destdir)) return FALSE;

    return TRUE;
}

BOOL SaveNetworkPrefs()
{
    // TODO: restart AROSTCP
    if (!WriteConfigVariables()) return FALSE;
    if (!CopyDefaultConfiguration(PREFS_PATH_ENVARC)) return FALSE;
    if (!WriteNetworkPrefs(PREFS_PATH_ENVARC)) return FALSE;
    if (!CopyDefaultConfiguration(PREFS_PATH_ENV)) return FALSE;
    if (!WriteNetworkPrefs(PREFS_PATH_ENV)) return FALSE;
    return TRUE;
}

BOOL UseNetworkPrefs()
{
    // TODO: restart AROSTCP
    if (!WriteConfigVariables()) return FALSE;
    if (!CopyDefaultConfiguration(PREFS_PATH_ENV)) return FALSE;
    if (!WriteNetworkPrefs(PREFS_PATH_ENV)) return FALSE;
    return TRUE;
}

void ReadNetworkPrefs()
{
    TEXT FileName[strlen(PREFS_PATH_ENV) + 4 + 20];
    BOOL comment = FALSE;
    STRPTR tstring;
    struct Tokenizer tok;

    sprintf(FileName, "%s/db/general.config", PREFS_PATH_ENV);
    OpenTokenFile(&tok, FileName);
    while (!tok.fend) {
	    if (tok.newline) { // read tokens from the beginning of line
		    if (tok.token) {
			    if (strcmp(tok.token, "HOSTNAME") == 0) {
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

    sprintf(FileName, "%s/db/interfaces", PREFS_PATH_ENV);
    OpenTokenFile(&tok, FileName);
    // reads only first uncommented interface
    while (!tok.fend) {
	    GetNextToken(&tok, " \n");
	    if (tok.token) {
		    if (tok.newline) comment = FALSE;
		    if (strncmp(tok.token, "#", 1) == 0) comment = TRUE;

		    if (!comment) {
			    if (strncmp(tok.token, "DEV=", 4) == 0) {
				    tstring = strchr(tok.token, '=');
                    SetDevice(tstring + 1);
			    }
			    if (strncmp(tok.token, "IP=", 3) == 0) {
				    tstring = strchr(tok.token, '=');
                    SetIP(tstring + 1);
			    }
			    if (strncmp(tok.token, "NETMASK=", 8) == 0) {
				    tstring = strchr(tok.token, '=');
                    SetMask(tstring + 1);
			    }
		    }
	    }
    }
    CloseTokenFile(&tok);

    sprintf(FileName, "%s/db/netdb-myhost", PREFS_PATH_ENV);
    OpenTokenFile(&tok, FileName);
    int dnsc = 0;
    while (!tok.fend) {
	    GetNextToken(&tok, " \n");
	    if (tok.token) {
		    if (strncmp(tok.token, "NAMESERVER", 4) == 0) {
			    GetNextToken(&tok, " \n");
                SetDNS(dnsc, tok.token);
			    dnsc++;
			    if (dnsc > 1) dnsc = 1;
		    }
	    }
    }
    CloseTokenFile(&tok);

    sprintf(FileName, "%s/db/static-routes", PREFS_PATH_ENV);
    OpenTokenFile(&tok, FileName);
    while (!tok.fend) {
	    GetNextToken(&tok, " \n");
	    if (tok.token) {
		    if (strncmp(tok.token, "DEFAULT", 4) == 0) {
			    GetNextToken(&tok, " \n");
			    if (strncmp(tok.token, "GATEWAY", 4) == 0) {
				    GetNextToken(&tok, " \n");
                    SetGate(tok.token);
			    }
		    }
	    }
    }
    CloseTokenFile(&tok);

    sprintf(FileName, "%s/Autorun", PREFS_PATH_ENV);
    OpenTokenFile(&tok, FileName);
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

    sprintf(FileName, "%s/db/DHCP", PREFS_PATH_ENV);
    OpenTokenFile(&tok, FileName);
    while (!tok.fend) {
	    GetNextToken(&tok, " \n");
	    if (tok.token) {
		    if (strncmp(tok.token, "True", 4) == 0) {
			    SetDHCP(TRUE);
			    break;
		    }
		    else {
                SetDHCP(FALSE);
			    break;

		    }
	    }
    }
    CloseTokenFile(&tok);
}

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

void SetIP(STRPTR w)
{
	strlcpy(prefs.IP, w,63);
}

void SetMask(STRPTR  w)
{
	strlcpy(prefs.mask, w,63);
}

void SetGate(STRPTR  w)
{
	strlcpy(prefs.gate, w,63);
}

void SetDNS(LONG m, STRPTR w)
{
	strlcpy(prefs.DNS[m], w,63);
}

void SetDHCP(BOOL w)
{
	prefs.DHCP = w;
}

void SetDevice(STRPTR w)
{
	strlcpy(prefs.device, w,511);
}

void SetHost(STRPTR w)
{
	strlcpy(prefs.host, w,511);
}

void SetDomain(STRPTR w)
{
	strlcpy(prefs.domain, w,511);
}
void SetAutostart(BOOL w)
{
    prefs.autostart = w;
}


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus

class Tokenizer
{
public:
char *tokenizerLine;
char *token;
FILE *tokenizedFile;
bool newline;
bool fend;

void openTokenFile(char * FileName)
{
	tokenizedFile = fopen(FileName, "r");
	token = NULL;
	newline = true;
	if (!tokenizedFile)
		fend = true;
	else{
		tokenizerLine = new char[8192];
		fend = false;
		newline = true;
	}
}

void closeTokenFile()
{
	if (tokenizedFile) {
		fclose(tokenizedFile);
		delete tokenizerLine;
		fend = true;
		token = NULL;
	}
}

void getNextToken(char* tk)
{
	char *tstring;

	newline = false;
	if (token != NULL) {
		token = strtok(NULL, tk);
		if (!token)
			getNextToken(tk);
	}else {
		newline = true;
		if (!feof(tokenizedFile)) {
			tokenizerLine[0] = 0;
			fgets(tokenizerLine, 8192, tokenizedFile);
			if (tokenizerLine == NULL) {
				token = NULL;
				getNextToken(tk);
			}else
				token = strtok(tokenizerLine, tk);
		}else
			fend = true;
	}
}
};

extern "C"
{
class TCPPrefs
{
public:
char IP[1000];
char mask[1000];
char gate[1000];
char DNS[2][1000];
bool DHCP;
char interf[4096];
char host[1000];
char domain[1000];
char config[8192];     // arostcp/db path here
Tokenizer tok;

TCPPrefs()
{
	strcpy(IP, "10.1.0.1");
	strcpy(mask, "255.255.255.0");
	strcpy(gate, "10.1.0.250");
	strcpy(DNS[0], "10.1.0.251");
	strcpy(DNS[1], "10.1.0.252");
	strcpy(interf, "DEVS:networks/prm-rtl8029.device");
	strcpy(host, "aros1");
	strcpy(domain, "arosnet.com");
	DHCP = false;
	strcpy(config, "ENV:AROSTCP/db");
}

void readPrefs()
{
	char *FileName;
	char *temp;
	bool comment = false;
	//bool NewLine;
	char *tstring;

	temp = new char[30];
	strcpy(temp, "ENV:AROSTCP/Config");
	tok.openTokenFile(temp);
	if (tok.tokenizedFile) {
		tok.getNextToken(" \n");
		strlcpy(config, tok.token,8191);
		tok.closeTokenFile();
	}else
		strcpy(config, "ENV:AROSTCP/db");

	FileName = new char[strlen(config) + 20];

	sprintf(FileName, "%s/general.config", config);
	tok.openTokenFile(FileName);
	while (!tok.fend) {
		if (tok.newline) { // read tokens from the beginning of line
			if (tok.token) {
				if (strcmp(tok.token, "HOSTNAME") == 0) {
					tok.getNextToken("=\n");
					tstring = strchr(tok.token, '.');
					strlcpy(domain, tstring + 1,999);
					tstring[0] = 0;
					strlcpy(host, tok.token,999);
				}
			}
		}
		tok.getNextToken("=\n");
	}
	tok.closeTokenFile();

	sprintf(FileName, "%s/interfaces", config);
	tok.openTokenFile(FileName);
	// reads only first uncommented interface
	while (!tok.fend) {
		tok.getNextToken(" \n");
		if (tok.token) {
			if (tok.newline) comment = false;
			if (strncmp(tok.token, "#", 1) == 0) comment = true;

			if (!comment) {
				if (strncmp(tok.token, "DEV=", 4) == 0) {
					tstring = strchr(tok.token, '=');
					strlcpy(interf, tstring + 1,4095);
				}
				if (strncmp(tok.token, "IP=", 3) == 0) {
					tstring = strchr(tok.token, '=');
					strlcpy(IP, tstring + 1,999);
				}
				if (strncmp(tok.token, "MASK=", 5) == 0) {
					tstring = strchr(tok.token, '=');
					strlcpy(mask, tstring + 1,999);
				}
			}
		}
	}
	tok.closeTokenFile();

	sprintf(FileName, "%s/netdb-myhost", config);
	tok.openTokenFile(FileName);
	int dnsc = 0;
	while (!tok.fend) {
		tok.getNextToken(" \n");
		if (tok.token) {
			if (strncmp(tok.token, "NAMESERVER", 4) == 0) {
				tok.getNextToken(" \n");
				strlcpy(DNS[dnsc], tok.token,999);
				dnsc++;
				if (dnsc > 1) dnsc = 1;
			}
		}
	}
	tok.closeTokenFile();

	sprintf(FileName, "%s/static-routes", config);
	tok.openTokenFile(FileName);
	while (!tok.fend) {
		tok.getNextToken(" \n");
		if (tok.token) {
			if (strncmp(tok.token, "DEFAULT", 4) == 0) {
				tok.getNextToken(" \n");
				if (strncmp(tok.token, "GATEWAY", 4) == 0) {
					tok.getNextToken(" \n");
					strlcpy(gate, tok.token,999);
				}
			}
		}
	}
	tok.closeTokenFile();

	sprintf(FileName, "%s/DHCP", config);
	tok.openTokenFile(FileName);
	while (!tok.fend) {
		tok.getNextToken(" \n");
		if (tok.token) {
			if (strncmp(tok.token, "True", 4) == 0) {
				DHCP = true;
				break;
			}
			else {
				DHCP = false;
				break;

			}
		}
	}
	tok.closeTokenFile();

	delete FileName;
	delete temp;
}

int writePrefs(char* dokad)
{
	FILE *ConfFile;
	char *FileName;

	FileName = new char[strlen(dokad) + 20];

	ConfFile = fopen("ENV:AROSTCP/Config", "w");
	if (!ConfFile) return 0;
	fprintf(ConfFile, "ENV:AROSTCP/db");
	fclose(ConfFile);

	ConfFile = fopen("ENVARC:AROSTCP/Config", "w");
	if (ConfFile) {
		fprintf(ConfFile, "ENV:AROSTCP/db");
		fclose(ConfFile);
	};
	
	sprintf(FileName, "%s/DHCP", dokad);
	ConfFile = fopen(FileName, "w");
	if (!ConfFile) return 0;
	fprintf(ConfFile, "%s\n", (DHCP) ? "True" : "False");
	fclose(ConfFile);

	sprintf(FileName, "%s/general.config", dokad);
	ConfFile = fopen(FileName, "w");
	if (!ConfFile) return 0;
	fprintf(ConfFile, "USELOOPBACK=YES\n");
	fprintf(ConfFile, "DEBUGSANA=NO\n");
	fprintf(ConfFile, "USENS=SECOND\n");
	fprintf(ConfFile, "GATEWAY=NO\n");
	fprintf(ConfFile, "HOSTNAME=%s.%s\n", host, domain);
	fprintf(ConfFile, "LOG FILTERFILE=5\n");
	fprintf(ConfFile, "GUI PANEL=MUI\n");
	fprintf(ConfFile, "OPENGUI=YES\n");
	fclose(ConfFile);

	sprintf(FileName, "%s/interfaces", dokad);
	ConfFile = fopen(FileName, "w");
	if (!ConfFile) return 0;
	fprintf(ConfFile,"eth0 DEV=%s UNIT=0 NOTRACKING IP=%s NETMASK=%s UP\n",interf,IP,mask);

	fclose(ConfFile);

	sprintf(FileName, "%s/netdb-myhost", dokad);
	ConfFile = fopen(FileName, "w");
	if (!ConfFile) return 0;
	fprintf(ConfFile, "HOST %s %s.%s %s\n", IP, host, domain, host);
	fprintf(ConfFile, "HOST %s gateway\n", gate);
	fprintf(ConfFile, "; Domain names\n");
	// DOMAIN dupa.com.pl
	//fprintf(ConfFile,"DOMAIN %d.%d.%d. %s\n",IP[0]&mask[0],IP[1]&mask[1],IP[2]&mask[2],domain);
	//fprintf(ConfFile,"DOMAIN %d.%d.%d.%d %s\n",IP[0]&mask[0],IP[1]&mask[1],IP[2]&mask[2],IP[3]&mask[3],domain);
	fprintf(ConfFile, "; Name servers\n");
	fprintf(ConfFile, "NAMESERVER %s\n", DNS[0]);
	fprintf(ConfFile, "NAMESERVER %s\n", DNS[1]);
	fclose(ConfFile);

	sprintf(FileName, "%s/static-routes", dokad);
	ConfFile = fopen(FileName, "w");
	if (!ConfFile) return 0;
	fprintf(ConfFile, "DEFAULT GATEWAY %s\n", gate);
	fclose(ConfFile);

	delete FileName;
	return 1;
}


};
}
#else
void ReadTCPPrefs();
int WriteTCPPrefs(char* DestDir);

char* GetIP();
char* GetMask();
char* GetGate();
char* GetDNS(int m);
int GetDHCP();
char* GetInterf();
char* GetHost();
char* GetDomain();

void SetIP(char * w);
void SetMask(char * w);
void SetGate(char * w);
void SetDNS(int m, char * w);
void SetDHCP(int w);
void SetInterf(char* w);
void SetHost(char* w);
void SetDomain(char* w);

#endif


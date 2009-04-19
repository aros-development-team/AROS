#include "AROSTCPPrefs.h"

static TCPPrefs prefs;

extern "C" int WriteTCPPrefs(char* dokad)
{
	return prefs.writePrefs(dokad);
}

extern "C" void ReadTCPPrefs()
{
	prefs.readPrefs();
}

extern "C"      char * GetIP()
{
	return prefs.IP;
}

extern "C"      char * GetMask()
{
	return prefs.mask;
}

extern "C"      char * GetGate()
{
	return prefs.gate;
}

extern "C"      char * GetDNS(int m)
{
	return prefs.DNS[m];
}

extern "C"      int GetDHCP()
{
	return int(prefs.DHCP);
}

extern "C"      char* GetInterf()
{
	return prefs.interf;
}

extern "C"      char* GetHost()
{
	return prefs.host;
}

extern "C"      char* GetDomain()
{
	return prefs.domain;
}

extern "C"      void SetIP(char * w)
{
	//if (prefs.IP) delete prefs.IP;
	//prefs.IP=new char[strlen(w)];
	strlcpy(prefs.IP, w,999);
}

extern "C"      void SetMask(char * w)
{
	//if (prefs.mask) delete prefs.mask;
	//prefs.mask=new char[strlen(w)];
	strlcpy(prefs.mask, w,999);
}

extern "C"      void SetGate(char * w)
{
	//if (prefs.gate) delete prefs.gate;
	//prefs.gate=new char[strlen(w)];
	strlcpy(prefs.gate, w,999);
}

extern "C"      void SetDNS(int m, char * w)
{
	//if (prefs.DNS[m]) delete prefs.DNS[m];
	//prefs.DNS[m]=new char[strlen(w)];
	strlcpy(prefs.DNS[m], w,999);
}

extern "C"      void SetDHCP(int w)
{
	prefs.DHCP = bool(w);
}

extern "C"      void SetInterf(char* w)
{
	strlcpy(prefs.interf, w,4095);
}

extern "C"      void SetHost(char* w)
{
	strlcpy(prefs.host, w,999);
	//printf("gotowe");
}

extern "C"      void SetDomain(char* w)
{
	strlcpy(prefs.domain, w,999);
	//printf("gotowe");
}

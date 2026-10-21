#!/usr/bin/env python3
"""Native drop targets unregister before a device can close its window."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[4]
s=(root/'rom/filesys/console_handler/con_handler.c').read_text()
def function(name):
 a=s.index('static void '+name+'(');b=s.index('\n}\n',a)+3;return s[a:b]
add=function('add_con_appwindow');remove=function('remove_con_appwindow')
a=s.index('    /* Remove the drop target before CloseDevice');b=s.index('    close_con_window(fh);',a)
close=s[a:b]
assert 'add_con_appwindow(fh);' in s[s.index('static LONG MakeConDevice'):s.index('static BOOL MakeSureWinIsOpen')]
program=r'''
#include <assert.h>
#include <stddef.h>
#define D(x) do{}while(0)
#define FHFLG_ICONIFIED 1
#define FHFLG_CONSOLEDEVICEOPEN 2
struct Window {int id;};struct IORequest{int id;};
struct filehandle{unsigned flags;void *appwindow,*workbenchbase,*appmsgport;struct Window *window,*borrowed;struct IORequest *conreadio;};
static struct Window native={1};static int adds,removes,closes,port_ok,add_ok;
static struct Window *con_window(struct filehandle *f){return f->window?f->window:f->borrowed;}
static int ensure_appmsgport(struct filehandle *f){if(port_ok)f->appmsgport=(void*)1;return port_ok;}
static void *AddAppWindow(int a,int b,struct Window *w,void *port,void *tags){(void)a;(void)b;(void)tags;assert(w==&native&&port);adds++;return add_ok?(void*)2:NULL;}
static void RemoveAppWindow(void *w){assert(w&&!closes);removes++;}
static void CloseDevice(struct IORequest *io){(void)io;assert(removes==1);closes++;}
ADD
REMOVE
static void close_device(struct filehandle *fh){CLOSE}
int main(void){
 for(int available=0;available<2;available++)for(int iconified=0;iconified<2;iconified++)for(int lib=0;lib<2;lib++)for(int port=0;port<2;port++)for(int ok=0;ok<2;ok++){
  struct filehandle f={0};f.borrowed=available?&native:NULL;f.flags=iconified;f.workbenchbase=lib?(void*)1:NULL;
  adds=removes=closes=0;port_ok=port;add_ok=ok;
  add_con_appwindow(&f);assert(adds==(available&&!iconified&&lib&&port));assert(!f.window);
  assert(!!f.appwindow==(available&&!iconified&&lib&&port&&ok));
  if(f.appwindow){add_con_appwindow(&f);assert(adds==1);f.flags|=FHFLG_CONSOLEDEVICEOPEN;close_device(&f);assert(removes==1&&closes==1&&!f.appwindow);}
  remove_con_appwindow(&f);assert(!f.appwindow);
 }
}
'''.replace('ADD',add).replace('REMOVE',remove).replace('CLOSE',close)
with tempfile.TemporaryDirectory() as d:
 p=Path(d);src=p/'test.c';exe=p/'test';src.write_text(program)
 subprocess.run(['cc','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
 # Omit only the early removal; registration remains valid, but closing the
 # device while its drop target is registered must fail the lifetime check.
 src.write_text(program.replace('static void close_device(struct filehandle *fh){'+close,'static void close_device(struct filehandle *fh){'+close.replace('remove_con_appwindow(fh);','')))
 subprocess.run(['cc','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
 assert subprocess.run([str(exe)],capture_output=True).returncode!=0
print('AppWindow availability/failure/ownership/close-order cases PASS; negative control rejected')

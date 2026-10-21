#!/usr/bin/env python3
"""Completion borrows only a known open console window, never a serial unit."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[4]
source=(root/'rom/filesys/console_handler/support.c').read_text()
start=source.index('struct Window *con_window(')
end=source.index('\n}\n',start)+3
block=source[start:end].replace('con_window','get')
program=r'''
#include <assert.h>
#include <string.h>
#include <stdint.h>
struct Window {int id;};
struct ConUnit {struct Window *cu_Window;};
struct IO {void *io_Unit;};
struct filehandle {struct Window *window;unsigned flags;char *devname;struct IO *conreadio;};
struct completioninfo {struct Window *window;};
#define FHFLG_DEVICEMODE 1
#define FHFLG_CONSOLEDEVICEOPEN 2
BLOCK
int main(void){
 struct Window classic={1},native={2};struct ConUnit unit={&native};struct IO io={&unit};
 struct filehandle fh={&classic,0,"console.device",NULL};assert(get(&fh)==&classic);
 fh.window=NULL;assert(!get(&fh));
 fh.flags=3;fh.devname="serial.device";fh.conreadio=(void*)(uintptr_t)1;assert(!get(&fh));
 fh.devname="consoleng.device";fh.flags=1;assert(!get(&fh));
 fh.flags=3;fh.conreadio=NULL;assert(!get(&fh));
 fh.conreadio=&io;io.io_Unit=NULL;assert(!get(&fh));
 io.io_Unit=&unit;assert(get(&fh)==&native);assert(!fh.window);
 unit.cu_Window=NULL;assert(!get(&fh));
 fh.window=&classic;assert(get(&fh)==&classic);
}
'''.replace('BLOCK',block)
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(program)
 subprocess.run(['cc','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('Completion window: classic/native/headless/closed ownership cases PASS')

#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Require independent contract failures from mutations of a captured coupled poll fixture."""
import argparse
import json
import os
from pathlib import Path
import subprocess
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--fixture',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
source=a.fixture.read_text()
mutations={
 'lost-error':('if (so->so_error) events |= AROS_POLL_ERR;', 'if (0) events |= AROS_POLL_ERR;'),
 'consumed-error':('if (so->so_error) events |= AROS_POLL_ERR;', 'if (so->so_error) events |= AROS_POLL_ERR; so->so_error=0;'),
 'out-on-hup':('!(events & AROS_POLL_HUP) && ', ''),
 'ignored-invalid':('fds[i].revents = AROS_POLL_NVAL;', 'fds[i].revents = 0;'),
 'empty-success-stub':('count = request->length;', 'count = request->length; if (!count) {request->result=0;return 0;}'),
 'unaligned-selection':('select_offset = (3 * obitsize + __alignof__(struct newselbuf) - 1) &\n                    ~(__alignof__(struct newselbuf) - 1);', 'select_offset = 3 * obitsize;'),
 'deadline-restarted':('while(TRUE) {', 'while(TRUE) { if(timeout) tsleep_send_timeout(libPtr, timeout);'),
 'lost-lease':('result = __WaitNativePoll(count,', 'for (i=0;i<count;i++) if(leases[i].private_record) FD_Release(&leases[i]);\n    result = __WaitNativePoll(count,'),
}
records=[]
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
for name,(before,after) in mutations.items():
    if source.count(before)!=1:raise RuntimeError('mutation anchor must match exactly once: '+name)
    path=out/(name+'.c');binary=out/name
    path.write_text(source.replace(before,after,1))
    cmd=['clang','-std=c11','-O1','-g','-pthread','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(path),'-o',str(binary)]
    c=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
    (out/(name+'-compile.log')).write_text(c.stdout)
    if c.returncode:raise RuntimeError('mutation must compile: '+name+' '+c.stdout)
    r=subprocess.run([str(binary)],env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=10)
    (out/(name+'-run.log')).write_text(r.stdout)
    if r.returncode==0:raise RuntimeError('surviving mutation: '+name)
    records.append({'mutation':name,'compiled':True,'runtime_rejected':True,'returncode':r.returncode})
(out/'result.json').write_text(json.dumps({'result':'PASS','mutants':records,'scope':'captured actual production fixture; native transport remains modeled'},indent=2)+'\n')
print('PASS eight compiled contract mutants rejected at runtime')

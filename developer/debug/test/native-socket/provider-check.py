#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Cross-build/link the native provider in private output using an explicit SDK.

SDK and generated inputs are read-only. No Make dependency regeneration or
installation is performed. Existing legacy C warnings are retained in logs.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[4]
HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
for name in ('sdk','generated','crosstools','collect-aros','output'):
 p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args()
out=a.output.resolve();sdk=a.sdk.resolve();gen=a.generated.resolve();tools=a.crosstools.resolve()/'bin'
if out.exists():p.error('output exists')
if any(x==out or x in out.parents for x in (ROOT,sdk,gen,tools.parent,a.collect_aros.resolve().parent)):
 p.error('output must be outside source/SDK/tools')
out.mkdir(parents=True)
subprocess.run([sys.executable,str(HERE/'cross-check.py'),'--sdk',str(sdk),
 '--generated',str(gen),'--crosstools',str(tools.parent),'--output',str(out/'objects')],check=True)
base=json.loads((out/'objects/commands.json').read_text())[0][:-4]
stack=ROOT/'workbench/network/stacks/AROSTCP/bsdsocket'
make=(stack/'mmakefile.src').read_text()
files=[]
for name in ['NET_C','NETINET_C','NETINET6_C','KERN_C','API_C']:
 match=re.search(r'^'+name+r'\s*:?=\s*',make,re.M)
 lines=make[match.end():].splitlines()
 for line in lines:
  files.extend(line.rstrip('\\').split())
  if not line.endswith('\\'):break
files.extend(['netinet/in_cksum_neon','netinet6/in6_cksum_neon'])
if len(files)!=len(set(files)):raise RuntimeError('duplicate provider source')
(out/'provider').mkdir()
def compile_one(name):
 source=stack/(name+'.c');target=out/'provider'/(name.replace('/','-')+'.o')
 command=base+['-Wno-error=implicit-function-declaration','-c',str(source),'-o',str(target)]
 result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 (target.with_suffix('.log')).write_text(result.stdout)
 if result.returncode:raise RuntimeError(name+'\n'+result.stdout)
 return {'source':str(source),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
         'object':str(target),'object_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),
         'command':command}
with ThreadPoolExecutor(max_workers=4) as executor:
 records=list(executor.map(compile_one,files))
(out/'provider-objects.json').write_text(json.dumps(records,indent=2)+'\n')
shim=out/'linkshim';shim.mkdir();(shim/'ld').symlink_to(tools/'ld.lld')
command=[str(a.collect_aros.resolve()),'--eh-frame-hdr','-o',str(out/'AROSTCP'),
 '-L'+str(sdk/'lib'),str(sdk/'lib/startup.o')]+[x['object'] for x in records]+[
 '-(','-lposixc','-lstdcio','-lstdc','-lamiga','-larossupport','-llibinit','-lautoinit',
 '-ldos','-lexec','-lutility','-lintuition','-lgraphics','-lmuimaster','-lkeymap','-lcommodities','-)']
environment=os.environ.copy();environment['COMPILER_PATH']=str(shim)
result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=environment)
(out/'provider-link-command.json').write_text(json.dumps(command,indent=2)+'\n')
(out/'provider-link.log').write_text(result.stdout)
if result.returncode:raise RuntimeError(result.stdout)
undefined=subprocess.check_output([str(tools/'llvm-nm'),'--undefined-only',str(out/'AROSTCP')],text=True)
(out/'undefined.txt').write_text(undefined)
if undefined.strip():raise RuntimeError('provider has undefined symbols')
(out/'result.json').write_text(json.dumps({'native_provider_link':'PASS','objects':len(records),
 'runtime':'UNTESTED','binary_sha256':hashlib.sha256((out/'AROSTCP').read_bytes()).hexdigest()},indent=2)+'\n')
print(f'PASS: {len(records)} source-bound AArch64 provider objects and full link; runtime UNTESTED')

#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Distributed under the AROS Public License 1.1.
from pathlib import Path
import argparse,subprocess,json,hashlib
p=argparse.ArgumentParser();p.add_argument('--libc',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);here=Path(__file__).resolve().parent
source=(a.libc/'src/unix/aros/mod.rs').read_text();start=source.index('mod aros_cmsg {');brace=source.index('{',start);level=1;end=brace+1
while level: level+={'{':1,'}':-1}.get(source[end],0);end+=1
helper=source[start:end]+'\npub use aros_cmsg::*;'
structs=[]
for name in ['msghdr','cmsghdr']:
 start=source.index('    pub struct '+name+' {');end=source.index('}',start)+1;structs.append('#[repr(C)]\n'+source[start:end])
unit='#![allow(non_camel_case_types)]\nextern crate self as libc;\ntype c_void=core::ffi::c_void;type c_int=i32;type socklen_t=u32;\n#[repr(C)]pub struct iovec { pub iov_base:*mut c_void,pub iov_len:usize }\n'+ '\n'.join(structs)+helper+'\nmod cases {\n'+(here/'cases.rs').read_text()+'\n}\nfn main(){println!("PASS CMSG pure memory checks={}",cases::checks());}';(out/'test.rs').write_text(unit)
cmd=['rustc','--edition=2021',str(out/'test.rs'),'-o',str(out/'test')];r=subprocess.run(cmd,capture_output=True,text=True);(out/'compile.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr;r=subprocess.run([str(out/'test')],capture_output=True,text=True);(out/'run.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr
(out/'result.json').write_text(json.dumps({'result':'PASS','scope':'actual extracted CMSG helpers and selected AArch64 type declarations; real host memory, not native transport','source_sha256':hashlib.sha256(source.encode()).hexdigest(),'output':r.stdout,'command':cmd},indent=2)+'\n');print(r.stdout,end='')

// Copyright (C) 2026, The AROS Development Team. All rights reserved.
// Execute a WASI SDK command with 32-bit pointers and eight-byte MemChunks.
import { readFile } from 'node:fs/promises';
import { WASI } from 'node:wasi';

const wasi = new WASI({ version: 'preview1', args: process.argv.slice(2) });
const module = await WebAssembly.compile(await readFile(process.argv[2]));
const instance = await WebAssembly.instantiate(module, wasi.getImportObject());
process.exitCode = wasi.start(instance);

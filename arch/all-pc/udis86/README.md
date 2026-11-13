# Udis86

Udis86 is a disassembler for the x86 and x86-64 class of instruction set
architectures. It consists of a C library called `libudis86` which
provides a clean and simple interface to decode and inspect a stream of 
raw binary data as disassembled instructions in a structured manner, and
a command line tool called `udcli` that incorporates the library.

## libudis86

- Supports all x86 and x86-64 (AMD64) General purpose and
    System instructions.
- Supported ISA extensions:
    - MMX, FPU (x87), AMD 3DNow
    - SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, SSE4a
    - AMD-V, INTEL-VMX, SMX, AVX, BMI, FMA4, FMA, F16C
    - ADX, MPX, SGX, RTM, AES, SHA, CET  
- Instructions are defined in an XML document, with opcode
    tables generated for performance.
- Supports output in both INTEL (NASM) as well as AT&T (GNU as) style
    assembly language syntax.
- Supports a variety of input methods: Files, Memory Buffers, and
    Function Callback hooks.
- Re-entrant, no dynamic memory allocation.
- Fully documented API

### usage example

```c
    ud_t u;
    
    ud_init(&u);
    ud_set_input_file(&u, stdin);
    ud_set_mode(&u, 64);
    ud_set_syntax(&u, UD_SYN_INTEL);
    
    while (ud_disassemble(&u)) {
      printf("\t%s\n", ud_insn_asm(&u));
    }
```

## udcli

udcli is a small command-line tool for your quick disassembly needs.

### usage example

```shell
    echo "65 67 89 87 76 65 54 56 78 89 09 00 90" | udcli -32 -x 
```
will result in output such as this:
```
    0000000080000800 656789877665     mov [gs:bx+0x6576], eax
    0000000080000806 54               push esp
    0000000080000807 56               push esi
    0000000080000808 7889             js 0x80000793
    000000008000080a 0900             or [eax], eax
    000000008000080c 90               nop
```

## Documentation

The libudis86 api is fully documented. The package distribution contains
a Texinfo file which can be installed by invoking "make install-info".
You can also find an online html version of the documentation available
at http://udis86.sourceforge.net/.

## Building

You need autotools if building from sources cloned from version control
system, or if you need to regenerate the build system. The wrapper
script 'autogen.sh' is provided that'll generate the build system.

Alternatively you can use cmake to build the library.

## License

Udis86 is distributed under the terms of the 2-clause "Simplified BSD
License". A copy of the license is included with the source in LICENSE.

## Author and contributors

Udis86 was originally written by [Vivek Thampi](https://github.com/vmt/).

Further fixes and additions have been merged from these forks:
- https://github.com/relyze-ltd/udis86
- https://github.com/matthewfl/udis86
- https://github.com/jwilk-forks/udis86
- https://github.com/TuileriesMac/Udis86
- https://github.com/falconkirtaran/udis86
- https://github.com/Fonger/udis86
- https://github.com/hasherezade/udis86
- https://github.com/bSr43/udis86
- https://github.com/frida/udis86
- https://github.com/el2ro/udis86
- https://github.com/radare/udis86 

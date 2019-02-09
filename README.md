# rop-tools: `gadgets` and `ropc`
### Two utilities for 64-bit return-oriented programming.

## gadgets
A tool that finds ROP gadgets in ELF executables. It can be configured to find different kinds of gadgets -- currently, gadgets that end in `ret` and `jmp reg` are supported. It dumps the discovered gadgets to a file.

## ropc
A compiler from pseudo-assembly to shellcode (x86-64 machine code). Included is a proof-of-concept implementation of a made-up instruction set -- see the "gadasm" (<i>gad</i>get <i>as</i>se<i>m</i>bly) directory for the definitions of the pseudo-instructions. "fib.gds" contains the source code in gadget assembly for a ROP exploit that computes the first 30 terms of the Fibonacci sequence and exits.

The distinguishing feauture of this ROP compiler and "gadget assembly" is that it uses a _second_ stack. This allows for recursion within shellcode _as well as calling C functions without mangling the shellcode_. It does this by repurposing `%rbp` as a second stack pointer.

(A formal specification for the made-up source language will be uploaded later. The source code and tests may not compile without intervention -- this is a very recent project, and I'm working on packaging it up better.)
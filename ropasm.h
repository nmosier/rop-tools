/* ropasm.h
 * Nicholas Mosier 2019
 */

#ifndef __ROPASM_H
#define __ROPASM_H

#include <stdint.h>
#include <libelf.h>

#include <llvm-c/Disassembler.h>

#define INSTR_DISASM_MAXLEN 64

#define INSTR_PRINT_HEX    0
#define INSTR_PRINT_DISASM 1

typedef struct instr {
  uint8_t *mc; // machine code bytes
  size_t mclen; // length of machine code
  Elf64_Off mcoff; // offset of machine code
  char disasm[INSTR_DISASM_MAXLEN];  // disassembled string
} instr_t;

typedef struct instrs {
  instr_t *arr;
  size_t cnt;
  size_t len;
} instrs_t;



LLVMDisasmContextRef ropasm_init();
void ropasm_end(LLVMDisasmContextRef dcr);

void instr_init(instr_t *instr);
void instr_delete(instr_t *instr);
int instr_disasm(instr_t *instr, LLVMDisasmContextRef dcr);
int instr_eq(const instr_t *lhs, const instr_t *rhs);
int instr_print(const instr_t *instr, FILE *f, int mode);

#endif

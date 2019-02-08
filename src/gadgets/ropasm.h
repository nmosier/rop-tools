/* ropasm.h
 * Nicholas Mosier 2019
 */

#ifndef __ROPASM_H
#define __ROPASM_H

#include <stdint.h>
#include <libelf.h>

#include <llvm-c/Disassembler.h>

#define INSTR_DISASM_MAXLEN 64 // this is just a guess...
#define INSTR_MC_MAXLEN     15 // is only 15, but just to be safe

enum instr_retv { INSTR_OK, INSTR_BAD, INSTR_ERR };

/* can be OR'ed together */
#define INSTR_PRINT_HEX    1
#define INSTR_PRINT_DISASM 2
#define INSTR_PRINT_ADDR   4

typedef struct instr {
  uint8_t mc[INSTR_MC_MAXLEN]; // machine code bytes
  size_t mclen; // length of machine code
  Elf64_Off mcoff; // offset of machine code
  char disasm[INSTR_DISASM_MAXLEN];  // disassembled string
} instr_t;

typedef struct instrs {
  instr_t *arr;
  size_t cnt;
  size_t len;
} instrs_t;


typedef struct instr_class {
  size_t mclen;
  uint8_t mcmask[INSTR_MC_MAXLEN];
  uint8_t mc[INSTR_MC_MAXLEN];
} instr_class_t;


LLVMDisasmContextRef ropasm_init();
void ropasm_end(LLVMDisasmContextRef dcr);

/* `instr' prototypes */
void instr_init(instr_t *instr);
int instr_disasm(instr_t *instr, LLVMDisasmContextRef dcr);
int instr_eq(const instr_t *lhs, const instr_t *rhs);
int instr_match(const instr_t *instr, const struct instr_class *iclass);
int instr_print(const instr_t *instr, FILE *f, int mode);
int instr_create(uint8_t *mc, size_t mclen, Elf64_Off mcoff,
		 LLVMDisasmContextRef dcr, instr_t *instr);

/* `instrs' prototypes */
void instrs_init(instrs_t *instrs);
int instrs_push(instr_t *instr, instrs_t *instrs);
int instrs_pop(instr_t *instr, instrs_t *instrs);
int instrs_add(instr_t *instr, instrs_t *instrs);

#endif

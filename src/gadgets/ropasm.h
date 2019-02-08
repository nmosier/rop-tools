/* ropasm.h
 * Nicholas Mosier 2019
 */

#ifndef __ROPASM_H
#define __ROPASM_H

#include <stdint.h>
#include <libelf.h>
#include <llvm-c/Disassembler.h>
#include "ropbank.h"

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
  int flags;
  // instruction-dependent info
  union {
    Elf64_Off jmpdst; // JMP: destination machine code offset
  } spec;
} instr_t;
#define INSTR_FLAGS_SPEC 1 // special info is set

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

/* instruction class declarations */
const struct instr_class CLASS_JUMP_INDIRECT;
const struct instr_class CLASS_JUMP_RELATIVE8;
const struct instr_class CLASS_JUMP_RELATIVE32;

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
ssize_t instr_class_find_inbank(rop_bank_t *bank, const instr_class_t *iclass,
				instrs_t *instrs, LLVMDisasmContextRef dcr,
				int (*op)(instr_t *));
/* instruction class functions */
int rjmps_find_inbank(rop_bank_t *bank, instrs_t *rjmps, LLVMDisasmContextRef dcr);
int rjmp_offset8(instr_t *rjmp8);
int rjmp_offset32(instr_t *rjmp32);
int rjmps_dstcmp(const instr_t *lhs, const instr_t *rhs);
void rjmps_dump(const instrs_t *rjmps, FILE *f);

#endif

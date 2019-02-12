#ifndef __ROPALG_H
#define __ROPALG_H

#include "ropasm.h"
#include "trie.h"
#include "ropbank.h"
#include "gadgets.h"
#include "ropaddr.h"

#define OPCODE_RET 0xc3
//#define MASK_RET   0xf7
#define OPCODE_RETF 0xcb
#define OPCODE_RETFA 0xca
#define OPCODE_LEAVE 0xc9
#define OPCODE_POP_RBP 0x5d

#define DISASM_JMP "\tjmp"

/* gadget find modes */
#define GADGETS_FIND_RETS  1
#define GADGETS_FIND_IJMPS 2

#define GADGETS_MAX_SUFFIXES 4

int gadgets_find(rop_banks_t *banks, trie_t gadtrie, LLVMDisasmContextRef dcr,
		 const struct gadgets_config *conf);
int gadgets_find_inbank(rop_bank_t *bank, trie_t gadtrie, LLVMDisasmContextRef dcr,
			struct rop_addrs *ok_addrs,
			const struct gadgets_config *conf);
int gadget_boundary(instr_t *instr);
void gadget_trunc(instrs_t *gadget);
int gadget_boring(instrs_t *gadget);

int gadgets_buildfrom(uint8_t *ret_it, uint8_t *start, Elf64_Off offset,
		      instr_t *suffix, trie_t gadtrie, instrs_t *rjmps,
		      LLVMDisasmContextRef dcr, struct rop_addrs *ok_addrs,
		      const struct gadgets_config *conf);
int gadgets_buildfrom_aux(uint8_t *instr_it, uint8_t *start, Elf64_Off offset,
			  trie_t gadtrie, instrs_t *rjmps, LLVMDisasmContextRef dcr,
			  struct rop_addrs *ok_addrs,
			  const struct gadgets_config *conf, instrs_t *gadget);
int gadgets_buildfrom_rjmps(uint8_t *dst, uint8_t *start, Elf64_Off offset,
			    trie_t gadtrie, instrs_t *rjmps,
			    LLVMDisasmContextRef dcr, struct rop_addrs *ok_addrs,
			    const struct gadgets_config *conf,
			    instrs_t *gadget);

#endif

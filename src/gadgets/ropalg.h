#ifndef __ROPALG_H
#define __ROPALG_H

#include "ropbank.h"

#define OPCODE_RET 0xc3
#define OPCODE_RETF 0xcb
#define OPCODE_RETFA 0xca
#define OPCODE_LEAVE 0xc9
#define OPCODE_POP_RBP 0x5d

#define DISASM_JMP "\tjmp"

#define GADGET_MAXLEN 4

int gadgets_find(rop_banks_t *banks, trie_t gadtrie, LLVMDisasmContextRef dcr,
		 int maxlen);
int gadgets_find_inbank(rop_bank_t *bank, trie_t gadtrie, LLVMDisasmContextRef dcr,
			int maxlen);
int gadget_boundary(instr_t *instr);
void gadget_trunc(instrs_t *gadget);
int gadget_boring(instrs_t *gadget);

#endif

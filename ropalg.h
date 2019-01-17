#ifndef __ROPALG_H
#define __ROPALG_H

#define OPCODE_RET 0xc3

int gadgets_find(rop_banks_t *banks, trie_t gadtrie, LLVMDisasmContextRef dcr);
int gadgets_find_inbank(rop_bank_t *bank, trie_t gadtrie, LLVMDisasmContextRef dcr);
int gadget_boundary(instr_t *instr);

#endif

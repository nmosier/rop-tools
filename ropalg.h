#ifndef __ROPALG_H
#define __ROPALG_H

#define OPCODE_RET 0xc3
#define OPCODE_RETF 0xcb
#define OPCODE_RETFA 0xca
#define OPCODE_JMP 0xea
#define OPCODE_LEAVE 0xc9
#define OPCODE_POP_RBP 0x5d


#define GADGET_MAXLEN 16

int gadgets_find(rop_banks_t *banks, trie_t gadtrie, LLVMDisasmContextRef dcr);
int gadgets_find_inbank(rop_bank_t *bank, trie_t gadtrie, LLVMDisasmContextRef dcr);
int gadget_boundary(uint8_t *ptr);
void gadget_trunc(instrs_t *gadget);
int gadget_boring(instrs_t *gadget);

#endif

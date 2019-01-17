#include <stdlib.h>
#include <string.h>
#include <llvm-c/Disassembler.h>

#include "ropelf.h" // need to rename this
#include "trie.h"
#include "ropasm.h"
#include "ropalg.h"

// gadget trie
int gadgets_find(rop_banks_t *banks, trie_t gadtrie, LLVMDisasmContextRef dcr) {
  rop_bank_t *bank_it;
  size_t nbanks = banks->len;
  size_t i;

  for (i = 0, bank_it = banks->arr; i < nbanks; ++i, ++bank_it) {
    if (gadgets_find_inbank(bank_it, gadtrie, dcr) < 0) {
      return -1;
    }
  }
  
  return 0;
}

int gadgets_find_inbank(rop_bank_t *bank, trie_t gadtrie, LLVMDisasmContextRef dcr) {

  /* start from end of bank
     repeat:
     (a) find 'ret' (0xc3)
     (b) repeat:
         (i) consider previous byte
	 (ii) if bytes are well-formed instruction, add to gadtrie; break out of loop.
  */

  uint8_t *ret_it; // finds ret opcodes 
  uint8_t *start;

    /* outer loop: find "ret" (0xc3) bytes */
  start = bank->b_start;
  for (ret_it = start + bank->b_len - 1; ret_it >= start; --ret_it) {
    /* find "ret" opcode */
    while (ret_it >= start && *ret_it != OPCODE_RET) {
      --ret_it;
    }
    if (ret_it == start) {
      /* ret not found -- done */
      break;
    }

    /* find gadgets starting at the found "ret" opcode */
    uint8_t *instr_rbegin_it; // invariant: always reverse beginning
                              // of instruction upon loop entry
    instrs_t gadget;
    instr_t instr;

    instr_init(&instr);
    instrs_init(&gadget);
    /* instructions loop */
    for (instr_rbegin_it = ret_it;
	 instr_rbegin_it >= start && !gadget_boundary(&instr); ) {
      /* instruction loop */
      /* note: instr_begin_it points to reverse beginning of instruction */
      size_t instr_len;
      for (instr_len = 1; instr_len <= INSTR_MC_MAXLEN; ++instr_len) {
	memcpy(instr.mc, instr_rbegin_it - instr_len, instr_len);
	instr.mclen = instr_len;
	instr.mcoff = instr_rbegin_it - instr_len - start;

	/* attempt disassembly */
	if (instr_disasm(&instr, dcr) == INSTR_OK) {
	  break; // found valid instruction
	}
      }
      if (instr_len > INSTR_MC_MAXLEN) {
	break; // found no valid instructions (?)
      }

      /* append instruction to instructionst list */
      if (instrs_insert(&instr, &gadget) < 0) {
	return -1; // internal error
      }

      /* add new gadget to trie */
      if (trie_addval(gadget.arr, gadget.cnt, gadtrie) < 0) {
	return -1; // internal error 
      }

      /* advance instruction rbegin poiner */
      instr_rbegin_it += instr_len;
      
    }

  }
  
  return -1;
}

/* returns 1 if instruction is boundary, 0 if not */
int gadget_boundary(instr_t *instr) {
  return instr->mclen == 1 && *instr->mc == OPCODE_RET;
}

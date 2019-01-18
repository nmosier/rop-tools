#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <llvm-c/Disassembler.h>


#include "ropelf.h" // need to rename this
#include "trie.h"
#include "util.h"
#include "ropasm.h"
#include "ropalg.h"

// gadget trie
int gadgets_find(rop_banks_t *banks, trie_t gadtrie, LLVMDisasmContextRef dcr,
		 int maxlen) {
  rop_bank_t *bank_it;
  size_t nbanks = banks->len;
  size_t i;

  for (i = 0, bank_it = banks->arr; i < nbanks; ++i, ++bank_it) {
    if (gadgets_find_inbank(bank_it, gadtrie, dcr, maxlen) < 0) {
      return -1;
    }
  }
  
  return trie_width(gadtrie);
}

int gadgets_buildfrom(uint8_t *ret, uint8_t *start, Elf64_Off offset,
		      trie_t gadtrie, LLVMDisasmContextRef dcr, int maxlen);
int gadgets_find_inbank(rop_bank_t *bank, trie_t gadtrie, LLVMDisasmContextRef dcr,
			int maxlen) {
  uint8_t *ret_it; // finds ret opcodes 
  uint8_t *start;

    /* outer loop: find "ret" (0xc3) bytes */
  start = bank->b_start;
  fprintf(stderr, "bank length = %zu\n", bank->b_len);
  for (ret_it = start + bank->b_len - 1; ret_it >= start; --ret_it) {
    /* find "ret" opcode */
    while (ret_it >= start && *ret_it != OPCODE_RET) {
      --ret_it;
    }
    if (ret_it < start) {
      /* ret not found -- done */
      break;
    }

    /* checks */
    assert (*ret_it == OPCODE_RET);

    if (gadgets_buildfrom(ret_it, start, bank->b_off, gadtrie, dcr, maxlen) < 0) {
      fprintf(stderr, "gadgets_buildfrom: internal error\n");
      return -1;
    }    
  }
  
  return 0;
}

/* returns 1 if instruction is boundary, 0 if not */
/*
int gadget_boundary(uint8_t *ptr) {
  return *ptr == OPCODE_RET;
}
*/


static const instr_t INSTR_RET = {{[0] = OPCODE_RET}, 1, 0, {0}};

int gadget_boundary(instr_t *instr) {
  return instr_eq(instr, &INSTR_RET);
}

void gadget_trunc_prefixes(instrs_t *gadget);
void gadget_trunc_sequences(instrs_t *gadget);
void gadget_trunc(instrs_t *gadget) {
  gadget_trunc_prefixes(gadget);
  gadget_trunc_sequences(gadget);
}

void gadget_trunc_prefixes(instrs_t *gadget) {
  const uint8_t boring_prefixes[] = {OPCODE_RET,
				     OPCODE_RETF,
				     OPCODE_RETFA,
				     //OPCODE_JMP,
				     //OPCODE_JNE1,
				     //OPCODE_JNE2,
  };
  size_t instr_i;
  instr_t *instr_it;

  if (gadget->cnt == 0) {
    return;
  }

  for (instr_i = gadget->cnt - 1, instr_it = gadget->arr + instr_i;
       instr_i >= 1; --instr_i, --instr_it) {
    for (const uint8_t *prefix = boring_prefixes;
	 prefix < ARREND(boring_prefixes); ++prefix) {
      if (instr_it->mc[0] == *prefix) {
	/* truncate up to this boring instruction */
	gadget->cnt = instr_i;
	return;
      }
    }
  }
}

void gadget_trunc_sequences(instrs_t *gadget) {
  instr_t *instr;

  if (gadget->cnt <= 1) {
    return;
  }

  /* check for "leave; ret" sequence */
  instr = &gadget->arr[1];
  if (instr->mclen == 1 && instr->mc[0] == OPCODE_LEAVE) {
    /* truncate to empty sequence */
    gadget->cnt = 0;
    return;
  }
}

/* assumes gadget_trunc() has been called on gadget */
int gadget_boring(instrs_t *gadget) {
  if (gadget->cnt <= 1) {
    return 1;
  }
  return 0;
}

/* build from one RET command (to which the ptr points */

int gadgets_buildfrom_aux(uint8_t *ret_it, uint8_t *start, Elf64_Off offset,
			  trie_t gadtrie, LLVMDisasmContextRef dcr,
			  int maxlen, instrs_t *gadget);
int gadgets_buildfrom(uint8_t *ret_it, uint8_t *start, Elf64_Off offset,
		      trie_t gadtrie, LLVMDisasmContextRef dcr, int maxlen) {
  instrs_t gadget;
  instrs_init(&gadget);

  /* checks */
  assert (ret_it[0] == 0xc3);
  if (ret_it == start) {
    return 0;
  }

  /* init gadget with ret */
  instr_t ret = {{[0] = OPCODE_RET}, 1, ret_it - start + offset, {0}};
  instr_disasm(&ret, dcr);
  instrs_push(&ret, &gadget);
  
  return gadgets_buildfrom_aux(ret_it - 1, start, offset, gadtrie, dcr, maxlen,
			       &gadget);
}

int gadgets_buildfrom_aux(uint8_t *instr_it, uint8_t *start, Elf64_Off offset,
			  trie_t gadtrie, LLVMDisasmContextRef dcr, int maxlen,
			  instrs_t *gadget) {
  instr_t instr;
  size_t instr_len;
  instr_init(&instr);

  size_t savcnt = gadget->cnt;
  gadget_trunc(gadget);
  if (!gadget_boring(gadget)) {
    if (trie_addval(gadget->arr, gadget->cnt, gadtrie) < 0) {
      return -1; // internal error 
    }
  }
  gadget->cnt = savcnt;
  
  if (gadget->cnt >= maxlen) {
    return 0;
  }

  
  for (instr_len = 1; instr_len <= INSTR_MC_MAXLEN
	 && instr_it - instr_len + 1 >= start; ++instr_len) {
    memcpy(instr.mc, instr_it - instr_len + 1, instr_len);
      instr.mclen = instr_len;
      /* compute the instruction offset using this mathematical mess */
      instr.mcoff = instr_it - start - instr_len + offset + 1;

      /* check if instruction is boundary */
      if (gadget_boundary(&instr)) {
	continue;
      }
      
      /* attempt disassembly */
      if (instr_disasm(&instr, dcr) != INSTR_OK
	  || instr.disasm[0] == 0) {
	continue;
      }
      
      /* found valid instruction;
       * generate all subgadgets with this instruction */
      /* append instruction to instructions list */
      if (instrs_push(&instr, gadget) < 0) {
	return -1; // internal error
      }
      
      /* find all subgadgets */
      if (gadgets_buildfrom_aux(instr_it - instr_len, start, offset, gadtrie,
				dcr, maxlen, gadget) < 0) {
	return -1; // internal error
      }
      
      /* pop off instruction and continue search */
      instrs_pop(NULL, gadget);
  }
    
  return 0;
}

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
int gadgets_find(rop_banks_t *banks, trie_t gadtrie, LLVMDisasmContextRef dcr) {
  rop_bank_t *bank_it;
  size_t nbanks = banks->len;
  size_t i;

  for (i = 0, bank_it = banks->arr; i < nbanks; ++i, ++bank_it) {
    if (gadgets_find_inbank(bank_it, gadtrie, dcr) < 0) {
      return -1;
    }
  }
  
  return trie_width(gadtrie);
}

int gadgets_buildfrom(uint8_t *ret, uint8_t *start, Elf64_Off offset,
		      trie_t gadtrie, LLVMDisasmContextRef dcr);
int gadgets_find_inbank(rop_bank_t *bank, trie_t gadtrie, LLVMDisasmContextRef dcr) {
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

    if (gadgets_buildfrom(ret_it, start, bank->b_off, gadtrie, dcr) < 0) {
      fprintf(stderr, "gadgets_buildfrom: internal error\n");
      return -1;
    }    
  }
  
  return 0;
}

/* returns 1 if instruction is boundary, 0 if not */
int gadget_boundary(uint8_t *ptr) {
  return *ptr == OPCODE_RET;
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
				     OPCODE_JMP,};
  size_t instr_i;
  instr_t *instr_it;

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

  /* check for "pop %rbp; ret" sequence */
  //  instr = &gadget->arr[1];
  //if (instr->mclen == 1 && instr->mc[0] == OPCODE_POP_RBP) {
    /* truncate to empty sequence */
  //gadget->cnt = 0;
  //return;
  //}  
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
			  instrs_t *gadget);
int gadgets_buildfrom(uint8_t *ret_it, uint8_t *start, Elf64_Off offset,
		      trie_t gadtrie, LLVMDisasmContextRef dcr) {
  instrs_t gadget;
  instrs_init(&gadget);

  return gadgets_buildfrom_aux(ret_it, start, offset, gadtrie, dcr, &gadget);
}

int gadgets_buildfrom_aux(uint8_t *ret_it, uint8_t *start, Elf64_Off offset,
			  trie_t gadtrie, LLVMDisasmContextRef dcr,
			  instrs_t *gadget) {
  uint8_t *instr_it = ret_it; // invariant: always points to beginning of instruction
  instr_t instr;
  instr_init(&instr);

  /* instructions loop */
  do {
    size_t instr_len;
    for (instr_len = 1; instr_len <= INSTR_MC_MAXLEN
	   && instr_it - instr_len + 1 >= start; ++instr_len) {
      memcpy(instr.mc, instr_it - instr_len + 1, instr_len);
      instr.mclen = instr_len;
      /* compute the instruction offset using this mathematical mess */
      instr.mcoff = instr_it - start - instr_len + offset + 1;

      /* attempt disassembly */
      if (instr_disasm(&instr, dcr) == INSTR_OK) {
	/* make sure it's not empty */
	if (instr.disasm[0]) {
	  break; // found valid instruction
	}
      }
    }
    if (instr_len > INSTR_MC_MAXLEN) {
      break; // found no valid instructions (?)
    }

    /* append instruction to instructionst list */
    if (instrs_push(&instr, gadget) < 0) {
      return -1; // internal error
    }

    instr_it -= instr_len;
  } while (!gadget_boundary(instr_it) && gadget->cnt <= GADGET_MAXLEN);

  gadget_trunc(gadget);
  if (!gadget_boring(gadget)) {
    if (trie_addval(gadget->arr, gadget->cnt, gadtrie) < 0) {
      return -1; // internal error 
    }
  }  
  
  return 0;
}

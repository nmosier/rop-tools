/* jmpalg.c
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <llvm-c/Disassembler.h>

#include "trie.h"
#include "ropbank.h"
#include "ropasm.h"
#include "jmpalg.h"

// ALGORITHM
// 1. find all `jmp r[abcd]x' instructions
// 2. find all `jmp imm16' instructions
// 3. 
//
//
//
//


int springs_find(rop_banks_t *banks, trie_t gadtrie, LLVMDisasmContextRef dcr,
		 int maxlen) {
  rop_bank_t *bank_it;
  size_t nbanks = banks->len;
  size_t i;

  for (i = 0, bank_it = banks->arr; i < nbanks; ++i, ++bank_it) {
    if (springs_find_inbank(bank_it, gadtrie, dcr, maxlen) < 0) {
      return -1;
    }
  }

  return trie_width(gadtrie);
}

int springs_find_inbank(rop_bank_t *bank, trie_t gadtrie, LLVMDisasmContextRef dcr,
			int maxlen) {
  ssize_t cnt, cnt2;
  instrs_t ijmps, rjmps;

  /* initialization */
  instrs_init(&ijmps);
  instrs_init(&rjmps);

  if ((cnt = instr_class_find_inbank(bank, &CLASS_JUMP_INDIRECT, &ijmps, dcr))
      < 0) {
    return -1;
  }
  fprintf(stderr, "found %ld indirect jumps\n", cnt);

  if ((cnt = instr_class_find_inbank(bank, &CLASS_JUMP_RELATIVE8, &rjmps, dcr))
      < 0) {
    return -1;
  }
  if ((cnt2 = instr_class_find_inbank(bank, &CLASS_JUMP_RELATIVE32, &rjmps, dcr))
      < 0) {
    return -1;
  }
  fprintf(stderr, "found %ld relative jumps\n", cnt + cnt2);

  return 0;
}

/* jmpalg.h
 * Nicholas Mosier 2019
 */

#ifndef __JMPALG_H
#define __JMPALG_H

#include <llvm-c/Disassembler.h>
#include "ropbank.h"
#include "ropasm.h"

ssize_t springs_find_indirect_jumps(rop_bank_t *bank, instrs_t *instrs,
				    LLVMDisasmContextRef dcr);
int springs_find_inbank(rop_bank_t *bank, trie_t gadtrie, LLVMDisasmContextRef dcr,
			int maxlen);
int springs_find(rop_banks_t *banks, trie_t gadtrie, LLVMDisasmContextRef dcr,
		 int maxlen);
ssize_t instr_class_find_inbank(rop_bank_t *bank, const instr_class_t *iclass,
				instrs_t *instrs, LLVMDisasmContextRef dcr);

#endif

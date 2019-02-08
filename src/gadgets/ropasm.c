#include <llvm-c/Disassembler.h>
#include <llvm-c/Target.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "ropasm.h"
#include "ropalg.h"
#include "util.h"

#define ROPASM_TRIPLENAME "x86_64-unknown-linux-gnu"

LLVMDisasmContextRef ropasm_init() {
  LLVMDisasmContextRef dcr;
  
  LLVMInitializeAllAsmPrinters();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllDisassemblers();

  if ((dcr = LLVMCreateDisasm(ROPASM_TRIPLENAME, NULL, 0, NULL, NULL)) == NULL) {
    return NULL;
  }
  
  if (LLVMSetDisasmOptions(dcr, LLVMDisassembler_Option_AsmPrinterVariant) != 1) {
    /* cannot set options  */
    LLVMDisasmDispose(dcr);
    return NULL;
  }

  return dcr;

}

void ropasm_end(LLVMDisasmContextRef dcr) {
  if (dcr) {
    LLVMDisasmDispose(dcr);
  }
}


void instr_init(instr_t *instr) {
  memset(instr, 0, sizeof(*instr));
}

/* allocates new instruction; initialzies with given params; attempts to disasm
  (if dcr is non-null) */
int instr_create(uint8_t *mc, size_t mclen, Elf64_Off mcoff,
		   LLVMDisasmContextRef dcr, instr_t *instr) {
  assert (mclen <= INSTR_MC_MAXLEN);
  memcpy(instr->mc, mc, mclen);
  instr->mclen = mclen;
  instr->mcoff = mcoff;

  if (dcr) {
    return instr_disasm(instr, dcr);
  }
  return INSTR_OK;
}

int instr_disasm(instr_t *instr, LLVMDisasmContextRef dcr) {
  if (LLVMDisasmInstruction(dcr, instr->mc, instr->mclen, instr->mcoff,
			     instr->disasm, INSTR_DISASM_MAXLEN)
      != instr->mclen) {
    return INSTR_BAD;
  }

  return INSTR_OK;
}

int instr_eq(const instr_t *lhs, const instr_t *rhs) {
  size_t lsize, rsize;

  if ((lsize = lhs->mclen) != (rsize = rhs->mclen)) {
    return 0;
  }
  if (memcmp(lhs->mc, rhs->mc, lsize)) {
    return 0;
  }
  return 1; // equal
}

int instr_match(const instr_t *instr, const struct instr_class *iclass) {
  size_t mclen = instr->mclen;
  if (mclen != iclass->mclen) {
    return 0;
  }

  for (size_t i = 0; i < mclen; ++i) {
    uint8_t instr_masked, class_masked;
    instr_masked = instr->mc[i] & iclass->mcmask[i];
    class_masked = iclass->mc[i] & iclass->mcmask[i];
    if (instr_masked != class_masked) {
      return 0;
    }
  }

  return 1;
}

int instr_print(const instr_t *instr, FILE *f, int mode) {
  if (instr) {
    if (mode & INSTR_PRINT_ADDR) {
      fprintf(f, "%p: ", (void *) instr->mcoff);
    }
    if (mode & INSTR_PRINT_HEX) {
      for (size_t i = 0; i < instr->mclen; ++i) {
	fprintf(f, "%1.1hx ", instr->mc[i]);
      }
    }
    if (mode & INSTR_PRINT_DISASM) {
      fprintf(f, "%s", instr->disasm);
    }
  }
  
  return 0;
}

void instrs_init(instrs_t *instrs) {
  memset(instrs, 0, sizeof(*instrs));
}

#define INSTRS_ARR_MINLEN 16
int instrs_push(instr_t *instr, instrs_t *instrs) {
  if (instrs->cnt == instrs->len) {
    /* resize */
    instr_t *newarr;
    size_t newlen = MAX(INSTRS_ARR_MINLEN, instrs->len * 2);
    if ((newarr = realloc(instrs->arr, newlen * sizeof(*instrs->arr))) == NULL) {
      return -1;
    }
    instrs->arr = newarr;
    instrs->len = newlen;
  }

  memcpy(instrs->arr + instrs->cnt++, instr, sizeof(*instr));
  return 0;
}

int instrs_add(instr_t *instr, instrs_t *instrs) {
  return instrs_push(instr, instrs);
}


int instrs_pop(instr_t *instr, instrs_t *instrs) {
  if (instrs->cnt == 0) {
    return -1;
  }
  
  if (instr) {
    memcpy(instr, &instrs->arr[instrs->cnt - 1], sizeof(*instr));
  }

  --instrs->cnt;
  
  return 0;
}



ssize_t instr_class_find_inbank(rop_bank_t *bank, const instr_class_t *iclass,
				instrs_t *instrs, LLVMDisasmContextRef dcr,
				int (*op)(instr_t *)) {
  uint8_t *start, *end, *it;
  instr_t instr;
  int disasm_result;
  Elf64_Off offset = bank->b_off;

  start = bank->b_start;
  end = start + bank->b_len;
  instr_init(&instr);
  instr.mclen = iclass->mclen;

  for (it = start; it + instr.mclen <= end; ++it) {
    memcpy(instr.mc, it, instr.mclen);
    instr.mcoff = offset + it - start;
    if (instr_match(&instr, iclass)) {
      /* found instruction that matches class */
      disasm_result = instr_disasm(&instr, dcr);
      assert(disasm_result == INSTR_OK);

      /* call operation (if non-NULL) */
      if (op && op(&instr) < 0) {
	fprintf(stderr, "instr_class_find_inbank: instruction-wise operation " \
		"encountered an error.\n");
	return -1;
      }

      /* add instruction to list */
      if (instrs_add(&instr, instrs) < 0) {
	perror("instrs_add");
	return -1; // internal error
      }
    }
  }

  return instrs->cnt;
}

int rjmps_find_inbank(rop_bank_t *bank, instrs_t *rjmps, LLVMDisasmContextRef dcr) {
  /* find 8-bit jumps */
  if (instr_class_find_inbank(bank, &CLASS_JUMP_RELATIVE8, rjmps, dcr,
			      rjmp_offset8) < 0) {
    return -1;
  }
  /* find 32-bit jumps */
  if (instr_class_find_inbank(bank, &CLASS_JUMP_RELATIVE32, rjmps, dcr,
			      rjmp_offset32) < 0) {
    return -1;
  }

  return 0;
}

int rjmp_offset8(instr_t *rjmp8) {
  assert(instr_match(rjmp8, &CLASS_JUMP_RELATIVE8));
  rjmp8->spec.jmpdst = rjmp8->mcoff + *((int8_t *) (rjmp8->mc + 1))
    + rjmp8->mclen;
  rjmp8->flags |= INSTR_FLAGS_SPEC;
  //  printf("0x%lx:\n", rjmp8->spec.jmpdst);
  return 0;
}

int rjmp_offset32(instr_t *rjmp32) {
  assert(instr_match(rjmp32, &CLASS_JUMP_RELATIVE32));
  rjmp32->spec.jmpdst  = rjmp32->mcoff + *((int32_t *) (rjmp32->mc + 1))
    + rjmp32->mclen;
  rjmp32->flags |= INSTR_FLAGS_SPEC;
  //printf("0x%lx:\n", rjmp32->spec.jmpdst);
  return 0;
}

int rjmps_dstcmp(const instr_t *lhs, const instr_t *rhs) {
  assert((lhs->flags & rhs->flags & INSTR_FLAGS_SPEC));
  if (lhs->spec.jmpdst == rhs->spec.jmpdst) {
    return 0;
  } else if (lhs->spec.jmpdst > rhs->spec.jmpdst) {
    return 1;
  } else {
    return -1;
  }
}

void rjmps_dump(const instrs_t *rjmps, FILE *f) {
  instr_t *rjmp;

  for (rjmp = rjmps->arr; rjmp < rjmps->arr + rjmps->cnt; ++rjmp) {
    fprintf(f, "0x%lx -> 0x%lx\t%s\n", rjmp->mcoff, rjmp->spec.jmpdst, rjmp->disasm);
  }
}

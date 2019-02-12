#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <llvm-c/Disassembler.h>


#include "ropelf.h" // need to rename this
#include "trie.h"
#include "util.h"
#include "ropasm.h"
#include "ropalg.h"
#include "ropaddr.h"

static const instr_class_t GADGET_SUFFIX_RET = {1, {[0] = 0xff},
						  {[0] = OPCODE_RET}};
// todo: get rid of this later
static const instr_class_t GADGET_SUFFIX_JMPREG =
  { 2, {[0] = 0xff, [1] = 0xf8}, {[0] = 0xff, [1] = 0xe0} };

const struct instr_class CLASS_JUMP_INDIRECT =
  {2, {[0] = 0xff, [1] = 0xf8}, {[0] = 0xff, [1] = 0xe0}};

const struct instr_class CLASS_JUMP_RELATIVE8 =
  {2, {[0] = 0xff}, {[0] = 0xeb}};

const struct instr_class CLASS_JUMP_RELATIVE32 =
 {5, {[0] = 0xff}, {[0] = 0xe9}};


// should be null-terminated
typedef struct masked_instr_class {
  const instr_class_t *suffix;
  int mask;
} masked_instr_class_t;
static const masked_instr_class_t gadget_suffixes[] =
  {
   {&GADGET_SUFFIX_RET, GADGETS_FIND_RETS},
   {&GADGET_SUFFIX_JMPREG, GADGETS_FIND_IJMPS},
   {0}
  };



int gadgets_find(rop_banks_t *banks, trie_t gadtrie, LLVMDisasmContextRef dcr,
		 const struct gadgets_config *conf) {
  rop_bank_t *bank_it;
  size_t nbanks = banks->len;
  size_t i;
  struct rop_addrs *ok_addrs = NULL;

  /* initialize set of allowable addresses, if given */
  if (conf->addr_path) {
    if ((ok_addrs = rop_addrs_new(conf->addr_path)) == NULL) {
      fprintf(stderr, "gadgets: %s: %s\n", conf->addr_path, strerror(errno));
      return -1;
    }
  }

  for (i = 0, bank_it = banks->arr; i < nbanks; ++i, ++bank_it) {
    if (gadgets_find_inbank(bank_it, gadtrie, dcr, ok_addrs, conf) < 0) {
      return -1;
    }
  }
  
  return trie_width(gadtrie);
}

int gadgets_find_inbank(rop_bank_t *bank, trie_t gadtrie, LLVMDisasmContextRef dcr,
			struct rop_addrs *ok_addrs,
			const struct gadgets_config *conf) {
  uint8_t *mc_it; // finds ret opcodes
  uint8_t *start, *end;
  instrs_t rjmps;

  /* init */
  instrs_init(&rjmps);

  /* setup -- build rjmp list */
  if (rjmps_find_inbank(bank, &rjmps, dcr) < 0) {
    fprintf(stderr, "rjmps_find_inbank: error.\n");
    return -1;
  }
  fprintf(stderr, "found %ld rjmps\n", rjmps.cnt);
  /* sort rjmps */
  qsort(rjmps.arr, rjmps.cnt, sizeof(*rjmps.arr),
	(int (*)(const void *, const void *)) rjmps_dstcmp);
  
  start = bank->b_start;
  end = start + bank->b_len;
  fprintf(stderr, "bank length = %zu\n", bank->b_len);
  for (mc_it = start; mc_it < end; ++mc_it) {
    instr_t suffix_instr;
    const masked_instr_class_t *suffix_class;
    
    /* find interesting suffix */
    for (suffix_class = gadget_suffixes; suffix_class->suffix; ++suffix_class) {
      size_t class_mclen;
      
      if ((suffix_class->mask & conf->gadgets_find_mode)) {
	class_mclen = suffix_class->suffix->mclen;
	suffix_instr.mclen = MIN(class_mclen, end - mc_it);
	memcpy(suffix_instr.mc, mc_it, suffix_instr.mclen);
	
	if (instr_match(&suffix_instr, suffix_class->suffix)) {
	  break;
	}
      }
    }
    if (suffix_class->suffix == NULL) {
      /* no matching suffix instruction found; continuing */
      continue;
    }
    
    /* finish initializing suffix instruction */
    suffix_instr.mcoff = mc_it - start + bank->b_off;
    int disasm_status = instr_disasm(&suffix_instr, dcr);
    assert(disasm_status == INSTR_OK);
    
    if (gadgets_buildfrom(mc_it, start, bank->b_off, &suffix_instr, gadtrie, &rjmps,
			  dcr, ok_addrs, conf) < 0) {
      fprintf(stderr, "gadgets_buildfrom: internal error\n");
      return -1;
    }    
  }
  
  return 0;
}



int gadget_boundary(instr_t *instr) {
  return instr_match(instr, &GADGET_SUFFIX_RET);
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

/* build from one gadget suffix */
int gadgets_buildfrom(uint8_t *ret_it, uint8_t *start, Elf64_Off offset,
		      instr_t *suffix, trie_t gadtrie, instrs_t *rjmps,
		      LLVMDisasmContextRef dcr, struct rop_addrs *ok_addrs,
		      const struct gadgets_config *conf) {
  instrs_t gadget;
  instrs_init(&gadget);

  /* checks */
  if (ret_it == start) {
    return 0;
  }

  /* initialize gadget with given suffix */
  instrs_push(suffix, &gadget);

  return gadgets_buildfrom_aux(ret_it - 1, start, offset, gadtrie, rjmps, dcr,
			       ok_addrs, conf, &gadget);
}

int gadgets_buildfrom_aux(uint8_t *instr_it, uint8_t *start, Elf64_Off offset,
			  trie_t gadtrie, instrs_t *rjmps, LLVMDisasmContextRef dcr,
			  struct rop_addrs *ok_addrs,
			  const struct gadgets_config *conf, instrs_t *gadget) {
  instr_t instr;
  size_t instr_len;
  instr_init(&instr);

  size_t savcnt = gadget->cnt;
  gadget_trunc(gadget);
  if (!gadget_boring(gadget)) {
    /* check if instruction address is allowed */
    int ok = 1;
    if (ok_addrs) {
      Elf64_Off addr = gadget->arr[gadget->cnt-1].mcoff;
      ok = rop_addrs_has(addr, ok_addrs);
    }

    /* add address to trie */
    if (ok) {
      if (trie_addval(gadget->arr, gadget->cnt, gadtrie) < 0) {
	return -1; // internal error 
      }
    }
  }
  gadget->cnt = savcnt;
  
  if (gadget->cnt >= conf->gadget_len) {
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
      
    /* find all subgadgets (contiguous) */
    if (gadgets_buildfrom_aux(instr_it - instr_len, start, offset, gadtrie,
			      rjmps, dcr, ok_addrs, conf, gadget) < 0) {
      return -1; // internal error
    }

    /* find all subgadgets (relative jumps to this instruction) */
    if (gadgets_buildfrom_rjmps(instr_it - instr_len + 1, start, offset, gadtrie,
    				rjmps, dcr, ok_addrs, conf, gadget) < 0) {
      return -1;
    }
    
    /* pop off instruction and continue search */
    instrs_pop(NULL, gadget);
  }
  
  return 0;
}


// dst is pointer to 1st byte of instruction that should be jump destination
int gadgets_buildfrom_rjmps(uint8_t *dst, uint8_t *start, Elf64_Off offset,
			    trie_t gadtrie, instrs_t *rjmps,
			    LLVMDisasmContextRef dcr, struct rop_addrs *ok_addrs,
			    const struct gadgets_config *conf,
			    instrs_t *gadget) {
  instr_t dst_instr;
  instr_t *src_rjmp;
  
  /* find relative jump(s) whose destination are before dst */
  instr_init(&dst_instr);
  dst_instr.spec.jmpdst = dst - start + offset;
  dst_instr.flags |= INSTR_FLAGS_SPEC;
  src_rjmp = bsearch(&dst_instr, rjmps->arr, rjmps->cnt, sizeof(*rjmps->arr),
		     (int (*)(const void *, const void *)) rjmps_dstcmp);
  if (src_rjmp == NULL) {
    /* no relative jump with destination found */
    return 0;
  }
  
  assert(instr_match(src_rjmp, &CLASS_JUMP_RELATIVE8) ||
	 instr_match(src_rjmp, &CLASS_JUMP_RELATIVE32));
  
  /* build from the soucre rjump instruction */
  return gadgets_buildfrom_aux(start + (src_rjmp->mcoff - offset) - 1, start,
			       offset, gadtrie, rjmps, dcr, ok_addrs, conf, gadget);
}



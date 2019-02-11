/* elfints_alg.cpp
 * Nicholas Mosier 2019
 */

#include <unordered_set>

#include <stdlib.h>
#include <stdio.h>

#include "ropelf.h"
#include "ropbank.h"
#include "util.h"
#include "elfints.h"

elfint elfints_getint(const uint8_t *ptr, const struct elfints_config *conf);
void elfints_printints(std::unordered_set<elfint> &ints, const elfints_config *conf,
		       const elfints_state *st);
void elfints_showprogress(const uint8_t *ptr, const rop_bank_t *bank);

extern "C"  int elfints_findints(const struct elfints_config *conf,
				 const struct elfints_state *st) {
  if (st->banks.len == 0) {
    fprintf(stderr, "elfints: ELF file contains no executable segments\n");
    return -1;
  }    
  if (st->banks.len > 1) {
    fprintf(stderr, "elfints: warning: only looking for ints "\
	    "in first bank of %d\n", st->banks.len);    
  }

  const rop_bank_t *bank = &st->banks.arr[0];
  std::unordered_set<elfint> intset;
  const uint8_t *start = bank->b_start,
    *end = start + bank->b_len,
    *it;
  for (it = start + conf->offset; it < end; it += conf->align) {
    elfint val = elfints_getint(it, conf);
    intset.insert(val);

    if (conf->verbose) {
      elfints_showprogress(it, bank);
    }
  }

  if (conf->verbose) {
    fprintf(stderr, "elfint: found %zu unique integers\n", intset.size());
  }

  elfints_printints(intset, conf, st);
  
  return 0;
}



elfint elfints_getint(const uint8_t *ptr, const struct elfints_config *conf) {
  switch (conf->intbits) {
  case 8:  return *ptr;
  case 16: return *((const int16_t *) ptr);
  case 32: return *((const int32_t *) ptr);
  case 64: return *((const int64_t *) ptr);
  default:
    abort(); // should have verified intbits earlier
  }
}

void elfints_printints(std::unordered_set<elfint> &ints, const elfints_config *conf,
		      const elfints_state *st) {
  const char *fmt = conf->intsigned ? "%zd\n" : "%zu\n";
  for (elfint val : ints) {
    fprintf(st->ints_f, fmt, val);

  }
}

#define PROGRESS_MSGS  10
void elfints_showprogress(const uint8_t *ptr, const rop_bank_t *bank) {
  size_t increment = bank->b_len / PROGRESS_MSGS;
  size_t current = ptr - bank->b_start;
  
  if (current % increment == 0) {
    fprintf(stderr, "[%3d%]\n", current / increment * PROGRESS_MSGS);
  }
}

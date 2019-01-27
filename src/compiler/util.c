/* util.c
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

void *memdup_f(void *ptr, size_t size) {
  void *newptr = malloc(size);
  if (newptr) {
    memcpy(newptr, ptr, size);
  }
  return newptr;
}



void libc_syms_init_bare(struct libc_syms *syms) {
  memset(syms, 0, sizeof(*syms));
}

int libc_syms_cmp(const struct libc_sym *lhs, const struct libc_sym *rhs) {
  return strcmp(lhs->name, rhs->name);
}

int libc_syms_init(struct libc_syms *syms, FILE *symf) {
  struct libc_sym sym;

  libc_syms_init_bare(syms);

  while (!feof(symf)) {
    if (fscanf(symf, "%lx", &sym.offset) < 1) {
      /* use previous offset */
      if (syms->symc == 0) {
	return -1; // syntax error
      }
      sym.offset = syms->symv[syms->symc-1].offset;
    } else {
      fscanf(symf, "%*c");
    }
    if (fscanf(symf, "%127s\n", sym.name) < 1) {
      return -1; // syntax error
    }
    if (libc_syms_add(&sym, syms) < 0) {
      return -1;
    }
  }

  /* sort */
  qsort(syms->symv, syms->symc, sizeof(*syms->symv),
	(int (*)(const void *, const void *))libc_syms_cmp);

  return 0;
}

int libc_syms_add(struct libc_sym *sym, struct libc_syms *syms) {
  if (syms->symc == syms->maxc) {
    /* resize */
    struct libc_sym *symv;
    int newc = MAX(syms->maxc * 2, ARR_MINLEN);
    if ((symv = realloc(syms->symv, sizeof(*symv)*newc)) == NULL) {
      return -1;
    }
    syms->symv = symv;
    syms->maxc = newc;
  }
  memcpy(&syms->symv[syms->symc++], sym, sizeof(*sym));
  return 0;
}

uint64_t libc_syms_getaddr(const char *name, const struct libc_syms *syms) {
  const struct libc_sym *sym;
  struct libc_sym key;

  if (strlen(name) >= LIBC_SYM_NAME_MAXLEN) {
    return -1;
  }
  strcpy(key.name, name);
  if ((sym = bsearch(&key, syms->symv, syms->symc, sizeof(*sym),
		     (int (*)(const void *, const void *)) libc_syms_cmp))
      == NULL) {
    return 0; // = NULL
  }

  return sym->offset;
}


/* libc_anchor2base: convert anchor symbol & address to base address of libc */
uint64_t libc_anchor2base(const char *anchor_sym, uint64_t anchor_addr) {
  return -1; // not yet implemented
}

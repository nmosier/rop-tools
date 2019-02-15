/* util.c
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

void *memdup_f(const void *ptr, size_t size) {
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
  int offset_present;
  char line[LIBC_SYM_LINE_MAXLEN];

  libc_syms_init_bare(syms);

  while (fgets(line, LIBC_SYM_LINE_MAXLEN, symf)) {
    offset_present = (line[0] == ' ') ? 0 : 1; // whether offset is present
    
    /* parse offset */
    if (offset_present) {
      if (sscanf(line, "%lx %*c %127s\n", &sym.offset, sym.name) < 2) {
	return -1; // syntax error
      }
    } else {
      /* use previous offset */
      if (syms->symc == 0) {
	return -1; // syntax error
      }
      sym.offset = syms->symv[syms->symc-1].offset;
      if (sscanf(line, "%*s %127s\n", sym.name) < 1) {
	return -1; // syntax error
      }
    }
    if (ferror(symf)) {
      return -1;
    }

    /* add symbol to table */
    if (libc_syms_add(&sym, syms) < 0) {
      return -1;
    }

    /* debug */
    //fprintf(stderr, "%lx %s\n", sym.offset, sym.name);
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
    return -1; // = NULL
  }

  return sym->offset;
}


/* libc_anchor2base: convert anchor symbol & address to base address of libc */
uint64_t libc_anchor2base(const char *anchor_sym, uint64_t anchor_addr) {
  return -1; // not yet implemented
}


/* roundpage: rounds size up to nearest page multiple. */
#define PAGE_SIZE 0x1000
uint64_t roundpage(uint64_t size) {
  uint64_t newsize;
  if (size % PAGE_SIZE) {
    newsize = (size/PAGE_SIZE + 1) * PAGE_SIZE;
  } else {
    newsize = size;
  }

  return newsize;
}

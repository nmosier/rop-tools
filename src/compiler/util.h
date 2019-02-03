/* util.h
 * Nicholas Mosier 2019
 */

#ifndef __UTIL_H
#define __UTIL_H

#include <stdio.h>

/* types */
#define LIBC_SYM_NAME_MAXLEN 128
#define LIBC_SYM_LINE_MAXLEN 128
struct libc_sym {
  char name[LIBC_SYM_NAME_MAXLEN];
  uint64_t offset;
};

struct libc_syms {
  struct libc_sym *symv;
  int symc;
  int maxc;
};


void libc_syms_init_bare(struct libc_syms *syms);
int libc_syms_init(struct libc_syms *syms, FILE *symf);
int libc_syms_add(struct libc_sym *sym, struct libc_syms *syms);  
uint64_t libc_syms_getaddr(const char *name, const struct libc_syms *syms);

#define memdup(ptr) (memdup_f(ptr, sizeof(*ptr)))
void *memdup_f(const void *ptr, size_t size);

#define MAX(i1, i2) ((i1) < (i2) ? (i2) : (i1))
#define ARR_MINLEN 16
#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define GENERIC_ERROR(name, srcinfo, desc, ...)				\
  fprintf(stderr, "%s: %s:%d: " desc "\n", name, (srcinfo).filename,	\
	  (srcinfo).lineno, __VA_ARGS__)


#endif

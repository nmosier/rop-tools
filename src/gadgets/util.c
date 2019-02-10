/* util.c
 * Nicholas Mosier 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libelf.h>
#include "util.h"

void pelferror(const char *s) {
  fprintf(stderr, "%s: %s\n", s, elf_errmsg(-1));
}

Elf64_Off phoffset(uint16_t phnum, uint16_t phentsize) {
  return (Elf64_Off) phnum * (Elf64_Off) phentsize;
}

void *memdup(void *ptr, size_t size) {
  void *newptr;
  if ((newptr = malloc(size)) == NULL) {
    return NULL;
  }
  memcpy(newptr, ptr, size);
  return newptr;
}

int parse_optarg_int(const char *optarg, int base, const char *prog, int optc,
		     int *val) {
  int optvalid = 0;
  long val_tmp;
  char *endptr;
  val_tmp = strtol(optarg, &endptr, base);
  if (*endptr != '\0') {
    /* invalid string */
    fprintf(stderr, "%s: -%c: requires integer\n", prog, optc);
    optvalid = -1;
  } else if (val_tmp > INT_MAX) {
    fprintf(stderr, "%s: -%c: integer too large\n", prog, optc);
    optvalid = -1;
  } else {
    *val = (int) val_tmp;
  }

  return optvalid;
}

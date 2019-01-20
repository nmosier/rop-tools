/* util.c
 * Nicholas Mosier 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

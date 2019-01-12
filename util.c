/* util.c
 * Nicholas Mosier 2019
 */

#include <stdio.h>
#include <libelf.h>
#include "util.h"

void pelferror(const char *s) {
  fprintf(stderr, "%s: %s\n", s, elf_errmsg(-1));
}

Elf64_Off phoffset(uint16_t phnum, uint16_t phentsize) {
  return (Elf64_Off) phnum * (Elf64_Off) phentsize;
}

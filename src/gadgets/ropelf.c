/* ropelf.c
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <libelf.h>
#include <gelf.h>

#include "ropelf.h"
#include "util.h"

Elf *ropelf_begin(int fd) {
  Elf *elf;

  /* check elf version (?) */
  if (elf_version(EV_CURRENT) == EV_NONE) {
    pelferror("elf_version");
    return NULL;
  }
  
  /* begin parsing elf */
  if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
    pelferror("elf_begin");
    return NULL;
  }

  /* make sure elf kind is correct */
  if (elf_kind(elf) != ELF_K_ELF) {
    elf_end(elf);
    return NULL;
  }

  return elf;
}


void ropelf_end(Elf *elf) {
  if (elf) {
    elf_end(elf);
  }
}

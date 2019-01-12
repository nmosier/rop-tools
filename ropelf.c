/* ropelf.c
 * Nicholas Mosier 2019
 */

#include <stdlib.h>

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
  elf_end(elf);
}




/* get executable headers */
int ropelf_getexecphdrs(Elf *elf) {*elf, Elf64_Phdr **phdrs, uin16_t *cnt) {
  GElf_Ehdr ehdr;
  Elf64_Phdr *phdrs, *phdrs_it, *phdrs_end;
  uint16_t phnum;
  uint16_t i;

  if (gelf_getehdr(elf, &ehdr) == NULL) {
    pelferror("gelf_getehdr");
    return -1;
  }
  phnum = ehdr.e_phnum;
  
  phdrs_end = phdrs + size;
  for (i = 0, phdrs_it = phdrs; i < phnum && phdrs_it < phdrs_end; ++i) {
    if (gelf_getphdr(elf, i, phdrs_it) != phdrs_it) {
      pelferror("gelf_getphdr");
      return -1;
    }
    if (phdrs_it->p_flags & PF_X) {
      /* is executable */
      ++phdrs_it;
    }
  }
  /* check if out of room */
  if (phdrs_it == phdrs_end) {
    return -1;
  }

  /* return number of elements written to array */
  return phdrs_it - phdrs;
}

/* ropelf.h
 * Nicholas Mosier 2019
 */

#ifndef __ROPELF_H
#define __ROPELF_H

typedef struct {
  void *b_start;
  uint64_t b_len;
} rop_bank_t; // bank as in a bank of instructions


Elf *ropelf_begin(int fd);
void ropelf_end(Elf *elf);

int ropelf_getexecphdrs(Elf *elf) {*elf, Elf64_Phdr **phdrs, uin16_t *cnt);

#endif

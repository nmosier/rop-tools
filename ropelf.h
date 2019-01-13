/* ropelf.h
 * Nicholas Mosier 2019
 */

#ifndef __ROPELF_H
#define __ROPELF_H

typedef struct {
  void *b_start;
  uint64_t b_len;
} rop_bank_t; // bank as in a bank of instructions

typedef struct {
  rop_bank_t *arr;
  size_t len;
} rop_banks_t;


Elf *ropelf_begin(int fd);
void ropelf_end(Elf *elf);

void banks_init(rop_banks_t *banks);
int bank_create(int fd, Elf64_Phdr *phdr, rop_bank_t *bank);
void bank_delete(rop_bank_t *bank);

int banks_create(int fd, Elf *elf, rop_banks_t *banks);
void banks_delete(rop_banks_t *banks);

void bank_hexdump(rop_bank_t *bank, FILE *f);

#endif

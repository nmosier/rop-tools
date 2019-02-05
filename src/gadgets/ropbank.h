/* ropbank.h
 * Nicholas Mosier 2019
 */

#ifndef __ROPBANK_H
#define __ROPBANK_H

typedef struct {
  uint8_t *b_start;
  uint64_t b_len;
  Elf64_Off b_off;
} rop_bank_t; // bank as in a bank of instructions

typedef struct {
  rop_bank_t *arr;
  size_t len;
} rop_banks_t;


void banks_init(rop_banks_t *banks);
int bank_create(int fd, Elf64_Phdr *phdr, rop_bank_t *bank);
void bank_delete(rop_bank_t *bank);

int banks_create(int fd, Elf *elf, rop_banks_t *banks);
void banks_delete(rop_banks_t *banks);

void bank_hexdump(rop_bank_t *bank, FILE *f);


#endif

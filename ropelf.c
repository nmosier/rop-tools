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
  elf_end(elf);
}



int bank_create(int fd, Elf64_Phdr *phdr, rop_bank_t *bank) {
  void *addr;

  assert (phdr->p_flags & PF_X); // must be executable
  
  if (lseek(fd, phdr->p_offset, SEEK_SET) < 0) {
    perror("lseek");
    return -1;
  }
  
  if ((addr = malloc(phdr->p_filesz)) == NULL) {
    perror("malloc");
    return -1;
  }

  if (read(fd, addr, phdr->p_filesz) < 0) {
    perror("read");
    free(addr);
    return -1;
  }

  bank->b_start = addr;
  bank->b_len = phdr->p_filesz;

  return 0;
}

void bank_delete(rop_bank_t *bank) {
  free(bank->b_start);
}

void banks_init(rop_banks_t *banks) {
  memset(banks, 0, sizeof(*banks));
}

int banks_create(int fd, Elf *elf, rop_banks_t *banks) {
  size_t nphdrs;
  rop_bank_t *arr;
  size_t size, i;
  int retv;

  /* init */
  retv = -1;
  arr = NULL;
  
  /* get number of program headers */
  if (elf_getphdrnum(elf, &nphdrs) != 0) {
    pelferror("elf_getphdrnum");
    goto cleanup;
  }

  /* allocate array */
  if ((arr = calloc(nphdrs, sizeof(rop_bank_t))) == NULL) {
    perror("calloc");
    goto cleanup;
  }

  /* add all executable program headers */
  for (i = size = 0; i < nphdrs; ++i) {
    Elf64_Phdr phdr;

    if (gelf_getphdr(elf, i, &phdr) != &phdr) {
      pelferror("elf_getphdr");
      goto cleanup;
    }
    
    if (phdr.p_flags & PF_X) {
      /* is executable, so get bank */
      rop_bank_t *bank = arr + size;
      if (bank_create(fd, &phdr, bank) < 0) {
	goto cleanup;
      }
      ++size;
    }
  }

  /* success: set values in banks struct */
  banks->arr = arr;
  banks->len = size;
  retv = 0;

  /* cleanup */
 cleanup:
  if (retv < 0 && arr) {
    /* delete all member banks */
    for (i = 0; i < size; ++i) {
      bank_delete(&arr[i]);
    }
    /* free banks array */
    free(arr);
  }

  return retv;
}

void banks_delete(rop_banks_t *banks) {
  for (size_t i = 0; i < banks->len; ++i) {
    bank_delete(&banks->arr[i]);
  }
  free(banks->arr);
}

#define BANK_HEXDUMP_WIDTH 8
void bank_hexdump(rop_bank_t *bank, FILE *f) {
  uint8_t *ptr, *end;
  int counter;

  end = (uint8_t *) bank->b_start + bank->b_len;
  counter = 0;
  for (ptr = bank->b_start; ptr < end; ++ptr) {
    fprintf(f, "0x%2.2x", *ptr);

    ++counter;
    fprintf(f, counter == 8 ? "\n" : " ");
    counter %= 8;
  }
}

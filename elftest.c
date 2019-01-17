#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include <libelf.h>
#include <gelf.h>

#include "ropelf.h"
#include "trie.h"
#include "util.h"
#include "ropasm.h"

#define VERBOSE 1

#define EXIT_FAILED 3 // exit status

#define PHDRS_MAXSIZE 10

int main(int argc, char *argv[]) {
  int fd;
  int exitno;
  const char *path;

  Elf *elf;
  rop_banks_t banks;
  trie_t trie;
  LLVMDisasmContextRef dcr;

  /* initialization */
  fd = -1;
  exitno = EXIT_FAILED;
  banks_init(&banks);
  trie = TRIE_ERROR;
  dcr = NULL;
  
  if (argc != 2) {
    fprintf(stderr, "usage: %s elf_file\n", argv[0]);
    exit(1);
  }
  path = argv[1];


  /* open file to parse */
  if ((fd = open(path, O_RDONLY, 0)) < 0) {
    perror("open");
    exit(3);
  }

  if ((elf = ropelf_begin(fd)) == NULL) {
    goto cleanup;
  }

  if (banks_create(fd, elf, &banks) < 0) {
    perror("banks_create");
    goto cleanup;
  }

  if (VERBOSE) {
    fprintf(stderr, "successfully created %zu banks.\n", banks.len);
  }

  /* dump exec hex */
  //bank_hexdump(&banks.arr[0], stdout);

  /* test trie */
  
  if ((trie = trie_init()) == TRIE_ERROR) {
    perror("trie_init");
    goto cleanup;
  }

  /* test disasm init */
  if ((dcr = ropasm_init()) == NULL) {
    goto cleanup;
  }
  
  
  // test: 6 bytes for 1st instruction
  //instr_t *instr = malloc(sizeof(*instr));
  instr_t instr;
  instr_init(&instr);
  memcpy(instr.mc, banks.arr[0].b_start, 6);
  instr.mclen = 6; // known
  instr.mcoff = 0; // need to work on later?
  if (instr_disasm(&instr, dcr) == INSTR_BAD) {
    fprintf(stderr, "instr_disasm: not an instruction.\n");
  } else {
    instr_print(&instr, stdout, INSTR_PRINT_HEX);
    printf("\n");
    instr_print(&instr, stdout, INSTR_PRINT_DISASM);
    printf("\n");
  }

  instr_t *instrp = memdup(&instr, sizeof(instr));
  trie_addval(&instrp, 1, trie);

  /* test nonistruction */
  instr_t badinstr;
  instr_init(&badinstr);
  memcpy(badinstr.mc, banks.arr[0].b_start, 5);
  badinstr.mclen = 5;
  badinstr.mcoff = 0;
  if (instr_disasm(&badinstr, dcr)) {
    instr_print(&badinstr, stdout, INSTR_PRINT_DISASM);
  } else {
    fprintf(stderr, "instr_disasm: not an instructino.\n");
  }
  //instr_delete(&instr);
  
  /*
  for (size_t i = 1; i <= 5; ++i) {
    //uint8_t *instr = banks.arr[0].b_start;
    if 
    
        if (trie_addinstr(instr, i, 0, trie) < 0) {
      perror("trie_addinstr");
      goto cleanup;
    }
    }*/

  /* print trie */
  if (trie_print(trie, stdout) < 0) {
    perror("trie_print");
    goto cleanup;
  }
  
  
  /* success */
  fprintf(stderr, "success!\n");
  exitno = 0;
  
  /* cleanup */
 cleanup:
  ropelf_end(elf);
  if (fd >= 0) {
    if (close(fd) < 0) {
      perror("close");
      exitno = EXIT_FAILED;
    }
  }
  trie_delete(trie);
  banks_delete(&banks);
  ropasm_end(dcr);

  exit(exitno);
}




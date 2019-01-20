#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>

#include <libelf.h>
#include <gelf.h>

#include "ropelf.h"
#include "trie.h"
#include "util.h"
#include "ropasm.h"
#include "ropalg.h"


#define VERBOSE 1
#define EXIT_FAILED 3 // exit status
#define PHDRS_MAXSIZE 10

/* defaults */
#define GADGETS_OUTPATH_DEFAULT "gadgets.asm"
#define HEXDUMP_OUTPATH_DEFAULT "bank.dmp"

int main(int argc, char *argv[]) {
  int fd;
  int exitno;
  const char *path;
  const char *optstring = "o:d:hn:";
  const char *usage = "usage: %s [-o outpath] [-d dumppath]" \
                      "[-n gadgetlen] elf_file\n";
  char *gadgets_outpath = GADGETS_OUTPATH_DEFAULT;
  char *hexdump_outpath = HEXDUMP_OUTPATH_DEFAULT;
  int gadget_len = GADGET_MAXLEN;

  Elf *elf;
  rop_banks_t banks;
  trie_t trie;
  LLVMDisasmContextRef dcr;

  /* parse arguments */
  int optc;
  int optvalid = 1;
  char *endptr;
  long int gadget_len_tmp;
  while ((optc = getopt(argc, argv, optstring)) >= 0) {
    switch (optc) {
    case 'o':
      gadgets_outpath = optarg;
      break;
    case 'd':
      hexdump_outpath = optarg;
      break;
    case 'n':
      gadget_len_tmp = strtol(optarg, &endptr, 10);
      if (*endptr != '\0' || gadget_len_tmp <= 0) {
	fprintf(stderr, "%s: -n: gadget length must be a positive integer\n",
		argv[0]);
	optvalid = 0;
      } else {
	gadget_len = gadget_len_tmp;
      }
      break;
    case 'h':
      fprintf(stderr, usage, argv[0]);
      optvalid = 0;
      break;
    default:
      optvalid = 0;
      break;
    }
  }
  if (!optvalid) {
    exit(1);
  }
  /* set src file */
  path = argv[optind];
  
  /* initialization */
  fd = -1;
  exitno = EXIT_FAILED;
  banks_init(&banks);
  trie = TRIE_ERROR;
  dcr = NULL;

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

  /* test trie */  
  if ((trie = trie_init()) == TRIE_ERROR) {
    perror("trie_init");
    goto cleanup;
  }

  /* test disasm init */
  if ((dcr = ropasm_init()) == NULL) {
    goto cleanup;
  }  

  /* test algorithm */
  int gadgets_found;
  if ((gadgets_found = gadgets_find(&banks, trie, dcr, gadget_len)) < 0) {
    perror("gadgets_find");
    goto cleanup;
  }
  fprintf(stderr, "gadgets found: %d\n", gadgets_found);

  /* dump results */ 
  FILE *gadgetf, *hexdumpf;
  
  gadgetf = fopen(gadgets_outpath, "w");
  trie_print(trie, gadgetf, INSTR_PRINT_HEX | INSTR_PRINT_DISASM);
  fclose(gadgetf);

  hexdumpf = fopen(hexdump_outpath, "w");
  trie_print(trie, hexdumpf, INSTR_PRINT_HEX | INSTR_PRINT_DISASM);
  fclose(hexdumpf);
  
  
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




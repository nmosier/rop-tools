#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include <libelf.h>
#include <gelf.h>

#include "ropelf.h"
#include "trie.h"
#include "util.h"
#include "ropasm.h"
#include "ropalg.h"
#include "ropbank.h"

#define VERBOSE 1
#define EXIT_FAILED 3 // exit status
#define PHDRS_MAXSIZE 10
#define GADGET_DEFAULTLEN 4

/* defaults */
#define GADGETS_OUTPATH_DEFAULT "gadgets.asm"

int main(int argc, char *argv[]) {
  /* argument variables */
  int exitno;
  const char *input_path;
  const char *usage = "usage: %s [-o outpath] [-n gadgetlen] elf_file\n";
  char *gadgets_outpath = GADGETS_OUTPATH_DEFAULT;
  int gadget_len = GADGET_DEFAULTLEN;
  int gadgets_find_mode = GADGETS_FIND_RETS;

  /* infrastrutcural variables */
  Elf *elf;
  rop_banks_t banks;
  trie_t trie;
  LLVMDisasmContextRef dcr;

  /* files */
  int input_fd;
  FILE *gadgetf;
  
  /* parse arguments */
  const char *optstring = "o:d:hn:rj";
  int optc;
  int optvalid = 1;
  char *endptr;
  long int gadget_len_tmp;
  while ((optc = getopt(argc, argv, optstring)) >= 0) {
    switch (optc) {
    case 'o':
      gadgets_outpath = optarg;
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

    case 'r': // find gadgets ending with `ret'
      gadgets_find_mode |= GADGETS_FIND_RETS;
      break;
    case 'j': // find gadgets ending with `jmp r*x'
      gadgets_find_mode |= GADGETS_FIND_IJMPS;
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

  /* check validity of options */
  if (!optvalid) {
    exit(1);
  }
  if (optind == argc) {
    fprintf(stderr, "%s: no input file specified.\n", argv[0]);
    exit(2);
  }

  /* set source file */
  input_path = argv[optind];
  
  /* initialization */
  input_fd = -1;
  gadgetf = NULL;
  exitno = EXIT_FAILED;
  banks_init(&banks);
  trie = TRIE_ERROR;
  dcr = NULL;

  /* open file to parse */
  if ((input_fd = open(input_path, O_RDONLY, 0)) < 0) {
    fprintf(stderr, "%s: %s: %s\n", argv[0], input_path, strerror(errno));
    exit(3);
  }

  if ((elf = ropelf_begin(input_fd)) == NULL) {
    goto cleanup;
  }

  if (banks_create(input_fd, elf, &banks) < 0) {
    perror("banks_create");
    goto cleanup;
  }
  
  if (VERBOSE) {
    fprintf(stderr, "successfully created %zu banks.\n", banks.len);
  }

  /* initialize objects */
  if ((trie = trie_init()) == TRIE_ERROR) {
    perror("trie_init");
    goto cleanup;
  }
  if ((dcr = ropasm_init()) == NULL) {
    goto cleanup;
  }  

  /* find gadgets -- main algorithm */
  int gadgets_found;
  if ((gadgets_found = gadgets_find(&banks, trie, dcr, gadget_len,
				    gadgets_find_mode)) < 0) {
    perror("gadgets_find");
    goto cleanup;
  }
  fprintf(stderr, "gadgets found: %d\n", gadgets_found);

  /* dump results */ 
  if ((gadgetf = fopen(gadgets_outpath, "w")) == NULL) {
    fprintf(stderr, "%s: %s: %s\n", argv[0], gadgets_outpath, strerror(errno));
    goto cleanup;
  }
  trie_print(trie, gadgetf, INSTR_PRINT_HEX|INSTR_PRINT_DISASM);

  /* find indirect jumps */
  //springs_find(&banks, trie, dcr, gadget_len);
  
  /* success */
  if (VERBOSE) {
    fprintf(stderr, "success!\n");
  }
  exitno = 0;
  
  /* cleanup */
 cleanup:
  ropelf_end(elf);
  if (input_fd >= 0) {
    if (close(input_fd) < 0) {
      perror("close");
      exitno = EXIT_FAILED;
    }
  }
  if (gadgetf) {
    if (fclose(gadgetf) < 0) {
      perror("fclose");
      exitno = EXIT_FAILED;
    }
  }
  trie_delete(trie);
  banks_delete(&banks);
  ropasm_end(dcr);

  exit(exitno);
}




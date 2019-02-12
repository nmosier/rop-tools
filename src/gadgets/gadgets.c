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
#include "gadgets.h"

#define VERBOSE 1
#define EXIT_FAILED 3 // exit status
#define PHDRS_MAXSIZE 10

/* defaults */
#define GADGET_LEN_DEFAULT 4
#define GADGETS_PATH_DEFAULT "gadgets.asm"


struct gadgets_state {
  Elf *elf;
  rop_banks_t banks;
  LLVMDisasmContextRef dcr;
  int elf_fd;
  FILE *gadget_f;
};


void gadgets_config_setdefaults(struct gadgets_config *conf);
int gadgets_getopts(int argc, char *argv[], struct gadgets_config *conf);

int main(int argc, char *argv[]) {
  int exitno;
  struct gadgets_config conf;

  /* get configuration */
  gadgets_config_setdefaults(&conf);
  if (gadgets_getopts(argc, argv, &conf) < 0) {
    exit(1); // invalid options; exiting
  }
  
  /* infrastrutcural variables */
  Elf *elf;
  rop_banks_t banks;
  trie_t trie;
  LLVMDisasmContextRef dcr;

  /* files */
  int elf_fd;
  FILE *gadget_f;
  
  
  /* initialization */
  elf_fd = -1;
  gadget_f = NULL;
  exitno = EXIT_FAILED;
  banks_init(&banks);
  trie = TRIE_ERROR;
  dcr = NULL;

  /* open file to parse */
  if ((elf_fd = open(conf.elf_path, O_RDONLY, 0)) < 0) {
    fprintf(stderr, "%s: %s: %s\n", argv[0], conf.elf_path, strerror(errno));
    exit(3);
  }

  if ((elf = ropelf_begin(elf_fd)) == NULL) {
    goto cleanup;
  }

  if (banks_create(elf_fd, elf, &banks) < 0) {
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
  if ((gadgets_found = gadgets_find(&banks, trie, dcr, &conf)) < 0) {
    perror("gadgets_find");
    goto cleanup;
  }
  fprintf(stderr, "gadgets found: %d\n", gadgets_found);

  /* dump results */ 
  if ((gadget_f = fopen(conf.gadgets_path, "w")) == NULL) {
    fprintf(stderr, "%s: %s: %s\n", argv[0], conf.gadgets_path, strerror(errno));
    goto cleanup;
  }
  trie_print(trie, gadget_f, INSTR_PRINT_HEX|INSTR_PRINT_DISASM);

  /* success */
  if (VERBOSE) {
    fprintf(stderr, "success!\n");
  }
  exitno = 0;
  
  /* cleanup */
 cleanup:
  ropelf_end(elf);
  if (elf_fd >= 0) {
    if (close(elf_fd) < 0) {
      perror("close");
      exitno = EXIT_FAILED;
    }
  }
  if (gadget_f) {
    if (fclose(gadget_f) < 0) {
      perror("fclose");
      exitno = EXIT_FAILED;
    }
  }
  trie_delete(trie);
  banks_delete(&banks);
  ropasm_end(dcr);

  exit(exitno);
}


void gadgets_config_setdefaults(struct gadgets_config *conf) {
  conf->elf_path = NULL;
  conf->gadgets_path = GADGETS_PATH_DEFAULT;
  conf->addr_path = NULL;
  conf->gadget_len = GADGET_LEN_DEFAULT;
  conf->gadgets_find_mode = GADGETS_FIND_RETS;
}


int gadgets_getopts(int argc, char *argv[], struct gadgets_config *conf) {
  const char *usage =
    "usage: %s [option...] elf_file\n"		\
    "Options:\n"							\
    "-o <outpath>     where to write the gadget dump to\n"		\
    "-n <length>      maximum length of the gadgets found\n"		\
    "-x <addrs_path>  path to list of addresses to exclusively consider\n" \
    "-r               find gadgets that end in `ret'\n"			\
    "-j               find gadgets that end in `jmp <reg>'\n"		\
    "-h               print this description\n";    
  
  /* parse arguments */
  const char *optstring = "o:d:hn:rjx:";
  int optc;
  int optvalid = 1;
  while ((optc = getopt(argc, argv, optstring)) >= 0) {
    switch (optc) {
    case 'o':
      conf->gadgets_path = optarg;
      break;

    case 'n':
      if (parse_optarg_int(optarg, BASE_10, argv[0], optc, &conf->gadget_len) < 0) {
	fprintf(stderr, "%s: -n: gadget length must be a positive integer\n",
		argv[0]);
	optvalid = 0;
      }
      break;

    case 'r': // find gadgets ending with `ret'
      conf->gadgets_find_mode |= GADGETS_FIND_RETS;
      break;

    case 'j': // find gadgets ending with `jmp r*x'
      conf->gadgets_find_mode |= GADGETS_FIND_IJMPS;
      break;

    case 'x': // ignore path
      conf->addr_path = optarg;
      break;
      
    case 'h':
    case '?':
    default:
      fprintf(stderr, usage, argv[0]);
      optvalid = 0;
      break;
    }
  }

  /* return if parsing failed */
  if (!optvalid) {
    return -1;
  }

  /* set input file */
  if (optind == argc) {
    fprintf(stderr, "%s: no input file specified.\n", argv[0]);
    return -1;
  }
  conf->elf_path = argv[optind];

  return optvalid ? 0 : -1;
}

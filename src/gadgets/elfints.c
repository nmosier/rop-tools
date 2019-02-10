/* elfints.c
 * finds integers in ELF executables
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "ropelf.h"
#include "ropbank.h"
#include "util.h"

#define INTBITS_DEFAULT  32
#define INTSIGNED_DEFAULT 0
#define BASE_10          10

struct elfints_config {
  int intbits;
  int intsigned;
  int align;
  int offset;
  int verbose;
  const char *inpath;
  const char *outpath;
};

struct elfints_state {
  int elf_fd;
  FILE *ints_f;
  Elf *elf;
  rop_banks_t banks;
};

int elfints_getopts(int argc, char *argv[], struct elfints_config *config);
void elfints_state_init(struct elfints_state *state);
int elfints_state_cleanup(struct elfints_state *state);

int main(int argc, char *argv[]) {
  int exitno = 2;

  /* parse arguments */
  struct elfints_config config;
  memset(&config, 0, sizeof(config));
  if (elfints_getopts(argc, argv, &config) < 0) {
    exit(1); // invalid options
  }

  struct elfints_state state;
  elfints_state_init(&state);

  /* open files */
  if ((state.elf_fd = open(config.inpath, O_RDONLY)) < 0) {
    fprintf(stderr, "%s: %s: %s\n", argv[0], config.inpath, strerror(errno));
    goto cleanup;
  }
  if (config.outpath) {
    if ((state.ints_f = fopen(config.outpath, "w")) == NULL) {
      fprintf(stderr, "%s: %s: %s\n", argv[0], config.outpath, strerror(errno));
      goto cleanup;
    }
  } else {
    /* use stdout */
    state.ints_f = stdout;
  }

  


  /* success */
  exitno = 0;
  
 cleanup:
  if (elfints_state_cleanup(&state) < 0) {
    exitno = 2;
  }
  
  exit(exitno);
}

int elfints_getopts(int argc, char *argv[], struct elfints_config *conf) {
  const char *usage = "usage: %s [-n bits] [-o outfile] [-v] elf_file\n";
  const char *optstring = "o:n:hvusa:";
  int optc;
  int optvalid = 0; // 0 if valid, -1 if invalid
  char *listp;
  const char *listarg;
  const char *listsep = ",";
  while ((optc = getopt(argc, argv, optstring)) >= 0) {
    switch (optc) {
    case 'o':
      conf->outpath = optarg;
      break;
      
    case 'n':
      if (parse_optarg_int(optarg, BASE_10, argv[0], optc, &conf->intbits) < 0) {
	optvalid = -1;
      }
      break;

    case 'u': // unsigned
      conf->intsigned = 0;
      break;
      
    case 's': // signed
      conf->intsigned = 1;
      break;

    case 'a': // alignment, optional offset
      listp = optarg;
      listarg = strsep(&listp, listsep);
      /* parse alignment */
      if (parse_optarg_int(listarg, BASE_10, argv[0], optc, &conf->align) < 0) {
	optvalid = -1;
      }
      if ((listarg = strsep(&listp, listsep))) {
	/* parse offset */
	if (parse_optarg_int(listarg, BASE_10, argv[0], optc, &conf->offset) < 0) {
	  optvalid = -1;
	}
      }
      break;
      
    case 'v':
      conf->verbose = 1;
      break;

    case 'h':
    case '?':
    default:
      fprintf(stderr, usage, argv[0]);
      optvalid = -1;
      break;
    }
  }

  /* set input file */
  if (optvalid >= 0) {
    if (optind == argc) {
      fprintf(stderr, "%s: elf executable argument required\n", argv[0]);
      fprintf(stderr, usage, argv[0]);
      optvalid = -1;
    } else {
      conf->inpath = argv[optind];
    }
  }
  
  return optvalid;
}


void elfints_state_init(struct elfints_state *state) {
  state->elf_fd = -1;
  state->ints_f = NULL;
  state->elf = NULL;
  banks_init(&state->banks);
}

int elfints_state_cleanup(struct elfints_state *state) {
  int retv = 0;
  
  if (state->elf) {
    ropelf_end(state->elf);
  }
  if (state->elf_fd >= 0) {
    if (close(state->elf_fd) < 0) {
      retv = -1;
    }
  }
  if (state->ints_f) {
    if (fclose(state->ints_f) < 0) {
      retv = -1;
    }
  }
  banks_delete(&state->banks);

  return retv;
}

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
#include "elfints.h"

int elfints_findints(const struct elfints_config *conf,
		     const struct elfints_state *st);
void elfints_confdefaults(struct elfints_config *conf);

int main(int argc, char *argv[]) {
  int exitno = 2;

  /* parse arguments */
  struct elfints_config config;
  elfints_confdefaults(&config);
  if (elfints_getopts(argc, argv, &config) < 0) {
    exit(1); // invalid options
  }

  struct elfints_state state;
  if (elfints_state_setup(&config, &state) < 0) {
    goto cleanup;
  }

  /* find integers */
  if (elfints_findints(&config, &state) < 0) {
    goto cleanup;
  }
  
  
  /* success */
  exitno = 0;

 cleanup:
  if (elfints_state_cleanup(&state) < 0) {
    exitno = 2;
  }
  
  exit(exitno);
}

void elfints_confdefaults(struct elfints_config *conf) {
  conf->intbits = INTBITS_DEFAULT;
  conf->intsigned = INTSIGNED_DEFAULT;
  conf->align = ALIGN_DEFAULT;
  conf->offset = OFFSET_DEFAULT;
  conf->verbose = 0;
  conf->inpath = NULL;
  conf->outpath = NULL;
}

int elfints_getopts(int argc, char *argv[], struct elfints_config *conf) {
  const char *usage =
    "usage: %s [option...] elf_file\n"		\
    "elfints -- find integers in ELF executables\n"	\
    "Options:\n"					\
    "  -o <outfile>        where to dump integers\n"		\
    "  -n <int>            size of integers to find, in bits\n"	\
    "  -u                  interpret integers as unsigned\n"		\
    "  -s                  interpret integers as signed (default)\n"	\
    "  -a <mult>[,<off>]   only find integers at _mult_ byte alignment\n"
    "                        at an offset of _off_ bytes\n"	\
    "                        (defaults: _mult_=1, _off_=0)\n"	\
    "  -v                  verbose mode\n"			\
    "  -h                  display this text\n";
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
      {
	int intbits;
	if (parse_optarg_int(optarg, BASE_10, argv[0], optc, &intbits) < 0) {
	  optvalid = -1;
	}
	if (intbits_valid(intbits)) {
	  conf->intbits = intbits;
	} else {
	  optvalid = -1;
	  fprintf(stderr, "%s: -n: %d-bit integers not supported\n",
		  argv[0], intbits);
	}
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


int elfints_state_setup(const struct elfints_config *config,
			struct elfints_state *state) {
    /* open files */
  if ((state->elf_fd = open(config->inpath, O_RDONLY)) < 0) {
    fprintf(stderr, "elfints: %s: %s\n", config->inpath, strerror(errno));
    return -1;
  }
  if (config->outpath) {
    if ((state->ints_f = fopen(config->outpath, "w")) == NULL) {
      fprintf(stderr, "elfints: %s: %s\n", config->outpath, strerror(errno));
      return -1;
    }
  } else {
    /* use stdout */
    state->ints_f = stdout;
  }

  /* setup elf & bank structures */
  if ((state->elf = ropelf_begin(state->elf_fd)) == NULL) {
    return -1;
  }
  if (banks_create(state->elf_fd, state->elf, &state->banks) < 0) {
    fprintf(stderr, "elfints: failed to create banks\n");
    return -1;
  }

  return 0;
}


int intbits_valid(int bits) {
  switch (bits) {
  case 8: return 1;
  case 16: return 1;
  case 32: return 1;
  case 64: return 1;
  default: return 0;
  }
}

/*
int elfints_findints(const rop_banks_t *banks) {
  if (banks->cnt != 1) {
    fprintf(stderr, "elfints: warning: printing results for %d banks", banks->cnt);
  }

  
}
*/

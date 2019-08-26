/* elfints.h
 * Nicholas Mosier 2019
 */

#ifndef __ELFINTS_H
#define __ELFINTS_H

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
#define INTSIGNED_DEFAULT 1
#define ALIGN_DEFAULT     1
#define OFFSET_DEFAULT    0
#define INTBITS_MAX      64
#define BASE_10          10

// bits of ELFINT_T >= INTBITS_MAX
typedef long long int elfint; 

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
int elfints_state_setup(const struct elfints_config *conf,
			struct elfints_state *state);
int elfints_state_cleanup(struct elfints_state *state);
int intbits_valid(int bits);

#endif

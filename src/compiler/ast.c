#include <stdlib.h>
#include <string.h>

#include "ast.h"

#define MAX(i1, i2) ((i1) < (i2) ? (i2) : (i1))

#define ARR_MINLEN 16

/* arguments functions */
void arguments_init(struct arguments *args) {
  memset(args, 0, sizeof(*args));
}

int arguments_add(struct argument *arg, struct arguments *args) {
  if (args->argc == args->maxc) {
    /* resize */
    struct argument *argv;
    if ((argv = realloc(args->argv,
			MAX(args->maxc*2, ARR_MINLEN) * sizeof(*args->argv)))
	== NULL) {
      return -1;
    }
    args->argv = argv;
    args->maxc *= 2;
  }
  memcpy(&args->argv[args->argc++], arg, sizeof(*arg));
  return 0;
}


/* instructions functions */
void instructions_init(struct instructions *instrs) {
  memset(instrs, 0, sizeof(*instrs));
}

int instructions_add(struct instruction *instr, struct instructions *instrs) {
  if (instrs->instrc == instrs->maxc) {
    /* resize */
    struct instruction *instrv;
    if ((instrv = realloc(instrs->instrv,
			  MAX(instrs->maxc*2, ARR_MINLEN)*sizeof(*instrv)))
	== NULL) {
      return -1;
    }
    instrs->instrv = instrv;
    instrs->maxc *= 2;
  }
  memcpy(&instrs->instrv[instrs->instrc++], instr, sizeof(*instr));
  return 0;
}

/* rules functions */
void rules_init(struct rules *rules) {
  memset(rules, 0, sizeof(*rules));
}

int rules_add(struct rule *rule, struct rules *rules) {
  if (rules->rulec == rules->maxc) {
    struct rule *rulev;
    if ((rulev = realloc(rules->rulev,
			 MAX(rules->maxc*2, ARR_MINLEN)*sizeof(*rulev)))
	== NULL) {
      return -1;
    }
    rules->rulev = rulev;
    rules->maxc *= 2;
  }
  memcpy(&rules->rulev[rules->rulec++], rule, sizeof(*rule));
  return 0;
}

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
    int newc = MAX(args->maxc*2, ARR_MINLEN);
    if ((argv = realloc(args->argv, newc * sizeof(*args->argv))) == NULL) {
      return -1;
    }
    args->argv = argv;
    args->maxc = newc;
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
    int newc = MAX(instrs->maxc*2, ARR_MINLEN);
    if ((instrv = realloc(instrs->instrv, newc*sizeof(*instrv))) == NULL) {
      return -1;
    }
    instrs->instrv = instrv;
    instrs->maxc = newc;
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
    int newc = MAX(rules->maxc*2, ARR_MINLEN);
    if ((rulev = realloc(rules->rulev, newc*sizeof(*rulev))) == NULL) {
      return -1;
    }
    rules->rulev = rulev;
    rules->maxc = newc;
  }
  memcpy(&rules->rulev[rules->rulec++], rule, sizeof(*rule));
  return 0;
}



const char *expression_kind2str(enum expression_kind kind) {
  switch (kind) {
  case EXPRESSION_SYM: return "EXPRESSION_SYM";
  case EXPRESSION_ID: return "EXPRESSION_ID";
  case EXPRESSION_INT: return "EXPRESSION_INT";
  case EXPRESSION_PLUS: return "EXPRESSION_PLUS";
  case EXPRESSION_MINUS: return "EXPRESSION_MINUS";
  default: return NULL;
  }
}

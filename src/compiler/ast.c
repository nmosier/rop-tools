#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "ast.h"
#include "symtab.h"

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

/* checks if rule is already present in rules using rule_cmp() */
int rules_has(struct rule *rule, struct rules *rules) {
  const struct rule *rule_it, *rule_end;

  for (rule_it = rules->rulev, rule_end = rule_it + rules->rulec;
       rule_it < rule_end; ++rule_it) {
    if (rule_cmp(rule_it, rule) == 0) {
      return 1;
    }
  }
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


int rule_cmp(const struct rule *r1, const struct rule *r2) {
  int cmp;

  if ((cmp = r1->kind - r2->kind)) {
    /* kind comparison suffices */
    return cmp;
  }
  if ((cmp = symbol_cmp(r1->sym, r2->sym))) {
    /* symbol comparison suffices */
    return cmp;
  }
  /* argument comparison is the final decider */
  return arguments_cmp(&r1->args, &r2->args);
}

int arguments_cmp(const struct arguments *a1, const struct arguments *a2) {
  int cmp;

  /* first compare lengths */
  if ((cmp = a1->argc - a2->argc)) {
    return cmp;
  }
  /* iterate through arguments */
  int argc = a1->argc;
  for (int argi = 0; argi < argc; ++argi) {
    if ((cmp = argument_cmp(&a1->argv[argi], &a2->argv[argi]))) {
      /* argument i differs */
      return cmp;
    }
  }
  /* must be equal */
  return 0;
}

int argument_cmp(const struct argument *a1, const struct argument *a2) {
  int cmp;

  /* first compare types */
  if ((cmp = a1->kind - a2->kind)) {
    return cmp;
  }
  /* kind-based comparison */
  switch (a1->kind) {
  case ARGUMENT_IMM64: return 0; /* terminal symbols are always equal */
  case ARGUMENT_MEM:
  case ARGUMENT_REG: return symbol_cmp(a1->reg, a2->reg);
  case ARGUMENT_EXPR: return 0;
    /* aruments really shouldn't be expressions at times we need to compare them */
  default:
    fprintf(stderr, "argument_cmp: interal error -- incorrect argument kind.\n");
    assert(0);
  }
}

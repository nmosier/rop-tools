#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "ast.h"
#include "symtab.h"
#include "vec.h"

#define MAX(i1, i2) ((i1) < (i2) ? (i2) : (i1))

#define ARR_MINLEN 16

/* vector function definitions */
vector_def(arguments, argument, arg);
vector_def(instructions, instruction, instr);
vector_def(rules, rule, rule);
vector_def(blocks, block, block);

void bytes_init(struct bytes *bytes) {
  memset(bytes, 0, sizeof(*bytes));
}

int bytes_add(uint8_t byte, struct bytes *bytes) {
  if (bytes->bytec == bytes->maxc) {
    /* resize */
    uint8_t *bytev;
    int newc = MAX(bytes->bytec*2, ARR_MINLEN);
    if ((bytev = realloc(bytes->bytev, newc * sizeof(*bytes->bytev))) == NULL) {
      return -1;
    }
    bytes->bytev = bytev;
    bytes->maxc = newc;
  }
  bytes->bytev[bytes->bytec++] = byte;
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
  case EXPRESSION_EXT: return "EXPRESSION_EXT";
  case EXPRESSION_ID: return "EXPRESSION_ID";
  case EXPRESSION_INT: return "EXPRESSION_INT";
  case EXPRESSION_PLUS: return "EXPRESSION_PLUS";
  case EXPRESSION_MINUS: return "EXPRESSION_MINUS";
  case EXPRESSION_ADDR: return "EXPRESSION_ADDR";
  case EXPRESSION_PC: return "EXPRESSION_PC";
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
  case ARGUMENT_REG:   return symbol_cmp(a1->reg, a2->reg);
  case ARGUMENT_EXPR:  return 0;

  case ARGUMENT_STR: // strings should never be passed as arguments, except to `db'
  default:
    fprintf(stderr, "argument_cmp: interal error -- incorrect argument kind.\n");
    abort();
  }
}

int block_cmp(struct block *blk1, struct block *blk2) {
  return symbol_cmp(blk1->sym, blk2->sym);
}

void program_init(struct program *prog) {
  rules_init(&prog->rules);
  blocks_init(&prog->blocks);
}

/* assumes that instruction is of type `INSTRUCTION_RULE' */
int instruction_match_rule(const struct instruction *instr, const struct rule *rule) {
  /* match symbols */
  if (symbol_cmp(instr->sym, rule->sym) != 0) {
    /* symbols don't match */
    return 0;
  }

  /* match arguments */
  if (arguments_match(&instr->args, &rule->args) == 0) {
    return 0;
  }

  /* instruction matches rule */
  return 1;
}

int arguments_match(const struct arguments *ref, const struct arguments *def) {
  const struct argument *ref_it, *def_it;
  int argi, argc;

  /* check if argument counts match */
  if (ref->argc != def->argc) {
    return 0;
  }

  /* iterate through arguments in lockstep */
  for (argi = 0, argc = ref->argc, ref_it = ref->argv, def_it = def->argv;
       argi < argc; ++argi, ++ref_it, ++def_it) {
    if (argument_match(ref_it, def_it) == 0) {
      /* argument pair does not match */
      return 0;
    }
  }

  /* arguments match */
  return 1;
}

int argument_match(const struct argument *ref, const struct argument *def) {
  enum argument_kind defk, refk;

  defk = def->kind;
  refk = ref->kind;

  switch (defk) {
  case ARGUMENT_IMM64:
    switch (refk) {
    case ARGUMENT_EXPR: // always resolves to type INT
    case ARGUMENT_IMM64: return 1;
    default: return 0;
    }
  case ARGUMENT_REG:
  case ARGUMENT_MEM:
    if (defk != refk) {
      return 0;
    }
    return symbol_cmp(ref->reg, def->reg) == 0;
  case ARGUMENT_EXPR:
  default:
    return 0; // only here if internal error or malformed rule
  }
}

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "ast.h"
#include "symtab.h"
#include "vec.h"
#include "util.h"

/* vector function definitions */
vector_def(arguments, argument, arg);
vector_def(instructions, instruction, instr);
vector_def(rules, rule, rule);
vector_def(blocks, block, block);
  

/* expression_kind2str: convert expression kind to string 
 * RETV: returns pointer to string if valid enum, otherwise NULL */

const char *expression_kind2str_arr[] = {"EXPRESSION_EXT",
					 "EXPRESSION_ID",
					 "EXPRESSION_INT,"
					 "EXPRESSION_PLUS",
					 "EXPRESSION_MINUS",
					 "EXPRESSION_ADDR",
					 "EXPRESSION_PC"};

const char *expression_kind2str(enum expression_kind kind) {
  if (kind >= 0 && kind < ARRLEN(expression_kind2str_arr)) {
    return expression_kind2str_arr[kind];
  } else {
    return NULL;
  }
}


/* comparison functions */

/* rule_cmp: compare two rules based on name and arguments
 * RETV: returns value less than, equal to, or greater than zero if
 *       r1 is `less than', equal to, or `greater than' r2.
 */
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

/* rules_has: check if rule contianed in rule vector
 * ARGS:
 *  - rule: rule to check for
 *  - rules: rule vector to check in
 * RETV: 1 if found, 0 if not found
 */
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

/* arguments_cmp: compare argument lists (not to be confused with matching)
 * RETV: same as all other C comparison functions.
 */
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

/* argument_cmp: compare two arguments */
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

/* block_cmp: compare two code blocks */
int block_cmp(struct block *blk1, struct block *blk2) {
  return symbol_cmp(blk1->sym, blk2->sym);
}


/* initialization functions */

/* program_init: initialize program AST node */
void program_init(struct program *prog) {
  rules_init(&prog->rules);
  blocks_init(&prog->blocks);
}

/* matching functions */

/* instruction_match_rule: check whether instruction instantiates a rule
 * RETV: 1 if match, 0 otherwise
 * NOTE: instruction kind should be `INSTRUCTION_RULE'
 */
int instruction_match_rule(const struct instruction *instr, const struct rule *rule) {
  assert(instr->kind == INSTRUCTION_RULE);
  
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

/* arguments_match: check whether the arguments of a rule reference (instruction) 
 *                  match the arguments of a rule definition 
 * RETV: returns 1 if match, 0 otherwise */
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

/* argument_match: check whether a single argument in a rule reference (instruction)
 *                 matches a single argument in a rule definition 
 * RETV: returns 1 if match, 0 otherwise */
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

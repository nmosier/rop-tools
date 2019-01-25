/* semant.c: semantic checker for ropc
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ast.h"
#include "symtab.h"

int semant_check_rules(struct rules *rules, struct symtab *tab) {
  struct rule *rule_it, *rule_end;
  struct symbol *rule_sym;
  int valid;

  valid = 1;
  for (rule_it = rules->rulev, rule_end = rule_it + rules->rulec;
       rule_it < rule_end; ++rule_it) {
    rule_sym = rule_it->sym;

    assert(rule_sym); // set at lexer stage

    if (rulek2symk(rule_it->kind) != rule_sym->kind) {
      fprintf(stderr, "semant: %s rule defined inconsistently.\n", rule_sym->name);
      valid = 0;
      continue;
    }

    /* add rule to symbol */
    switch (rule_it->kind) {
    case RULE_DEFINITION:
      if (rules_has(rule_it, &rule_sym->defs)) {
	fprintf(stderr, "semant: rule %s with same arguments was already defined.\n",
		rule_sym->name);
	valid = 0;
      } else {
	rules_add(rule_it, &rule_sym->defs);
      }
      break;
    case RULE_EQUATE:
      if (rule_it->args.argc != 0) {
	fprintf(stderr, "semant: equate %s should not take any arguments.\n",
		rule_sym->name);
	valid = 0;
	break;
      }
      if (rule_sym->equ) {
	fprintf(stderr, "semant: equate %s was already defined.\n", rule_sym->name);
	valid = 0;
	break;
      }
      rule_sym->equ = rule_it;

      break;
    default:
      // internal error
      fprintf(stderr, "semant: internal error: incorrect rule kind.\n");
      valid = 0;
      break;
    }
  }

  return valid ? 0 : -1;
}


/* add references to blocks into symbol table */
int semant_check_blocks(struct blocks *blocks, struct symtab *tab) {
  struct block *block_it, *block_end;
  int valid;

  valid = 1;
  for (block_it = blocks->blockv, block_end = block_it + blocks->blockc;
       block_it < block_end; ++block_it) {
    struct symbol *block_sym = block_it->sym;

    if (block_sym->kind != SYMBOL_BLOCK) {
      fprintf(stderr, "semant: symbol %s is defined incosnistently.\n",
	      block_sym->name);
      valid = 0;
      continue;
    }
    if (block_sym->blk) {
      fprintf(stderr, "semant: label %s defined more than once.\n", block_sym->name);
      valid = 0;
      continue;
    }
    block_sym->blk = block_it;
  }
  return valid ? 0 : -1;
}


int semant_check(struct program *prog, struct symtab *tab) {
  int valid = 1;

  if (semant_check_rules(&prog->rules, tab) < 0) {
    valid = 0;
  }
  if (semant_check_blocks(&prog->blocks, tab) < 0) {
    valid = 0;
  }

  return valid ? 0 : -1;
}

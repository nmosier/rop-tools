/* semant.c: semantic checker for ropc
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ast.h"
#include "symtab.h"

int semant_build_rules(struct rules *rules, struct symtab *tab) {
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

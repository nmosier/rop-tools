/* semant.c: semantic checker for ropc
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ast.h"
#include "util.h"
#include "symtab.h"
#include "stages.h"

#define SEMANT_ERROR(srcinfo, desc, ...)	\
  GENERIC_ERROR("semant", srcinfo, desc, __VA_ARGS__)

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
      //fprintf(stderr, "semant: %s rule defined inconsistently.\n", rule_sym->name);
      SEMANT_ERROR(rule_it->srcinfo, "rule %s defined inconsistently.",
		   rule_sym->name);
      valid = 0;
      continue;
    }

    /* add rule to symbol */
    switch (rule_it->kind) {
    case RULE_DEFINITION:
      if (rules_has(rule_it, &rule_sym->defs)) {
	SEMANT_ERROR(rule_it->srcinfo, "rule %s with same arguments was already "\
		     "defined.", rule_sym->name);
	/*
	fprintf(stderr, "semant: rule %s with same arguments was already defined.\n",
	rule_sym->name); */
	valid = 0;
      } else {
	rules_add(rule_it, &rule_sym->defs);
      }
      break;
    case RULE_EQUATE:
      if (rule_it->args.argc != 0) {
	SEMANT_ERROR(rule_it->srcinfo, "equate %s should not take any arguments.",
		     rule_sym->name);
	/* fprintf(stderr, "semant: equate %s should not take any arguments.\n",
	   rule_sym->name); */
	valid = 0;
	break;
      }
      if (rule_sym->equ) {
	SEMANT_ERROR(rule_it->srcinfo, "equate %s was already defined.",
		     rule_sym->name);
	SEMANT_ERROR(rule_sym->equ->srcinfo, "%s", "previous definition.");
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
      SEMANT_ERROR(block_it->srcinfo, "symbol %s is defined inconsistently.",
		   block_sym->name);
      valid = 0;
      continue;
    }
    if (block_sym->blk) {
      SEMANT_ERROR(block_it->srcinfo, "label %s defined more than once.",
		   block_sym->name);
      valid = 0;
      continue;
    }
    block_sym->blk = block_it;
  }
  return valid ? 0 : -1;
}


/* links blocks and rules to corresponding entries in the symbol table;
 * checks for consistency, etc
 */
int semant_pass1(struct program *prog, struct symtab *tab) {
  int valid = 1;

  if (semant_check_rules(&prog->rules, tab) < 0) {
    valid = 0;
  }
  if (semant_check_blocks(&prog->blocks, tab) < 0) {
    valid = 0;
  }

  return valid ? 0 : -1;
}


int semant_link_instrs(struct instructions *instrs, struct symtab *tab) {
  struct instruction *instr_it, *instr_end;
  int valid;

  /* link instructions in vector */
  valid = 1;
  for (instr_it = instrs->instrv, instr_end = instr_it + instrs->instrc;
       instr_it < instr_end; ++instr_it) {
    
    /* determine if instruction needs linking */
    if (instr_it->kind != INSTRUCTION_RULE) { continue; }

    /* get instruction prefix symbol */
    struct symbol *sym = instr_it->sym;

    /* get list of rule definition candidates from symbol */
    struct rules *defs = &sym->defs;

    /* iterate through definition candidates to find match */
    struct rule *def_it, *def_end;
    for (def_it = defs->rulev, def_end = def_it + defs->rulec;
	 def_it < def_end && instruction_match_rule(instr_it, def_it) == 0;
	 ++def_it)
      {}

    if (def_it == def_end) {
      /* didn't find a matching definition for instruction instantiation */
      SEMANT_ERROR(instr_it->srcinfo, "found no matching definition for " \
		   "instruction %s.", sym->name);
      valid = 0;
      continue;
    }

    /* link instruction to found definition */
    instr_it->ref = def_it;
  }

  return valid ? 0 : -1;
}

/* links instructions to rules that the reference  */
int semant_pass2(struct program *prog, struct symtab *tab) {
  struct rule *rule_it, *rule_end;
  struct block *block_it, *block_end;
  int valid;

  valid = 1;

  /* link rules' instructions */
  for (rule_it = prog->rules.rulev, rule_end = rule_it + prog->rules.rulec;
       rule_it < rule_end; ++rule_it) {
    switch (rule_it->kind) {
    case RULE_DEFINITION:
      if (semant_link_instrs(&rule_it->definition, tab) < 0) {
	valid = 0;
      }
      break;
    case RULE_EQUATE:
      break; // equates have no instructions
    default:
      fprintf(stderr, "semant: internal error: rule is of incorrect type.\n");
      valid = 0;
      break;
    }
  }

  /* link blocks' instructions */
  for (block_it = prog->blocks.blockv, block_end = block_it + prog->blocks.blockc;
       block_it < block_end; ++block_it) {
    if (semant_link_instrs(&block_it->instrs, tab) < 0) {
      valid = 0;
    }
  }

  if (!valid) {
    fprintf(stderr, "semant: pass 2 failed.\n");
  }

  return valid ? 0 : -1;
}


int semant(struct program *prog, struct symtab *tab, int stages) {
  int valid;

  valid = 1;
  /* install stage symbols */
  int result = semant_install_stagesyms(tab, stages);
  assert(result >= 0);

  /* do semantic passes */
  if (semant_pass1(prog, tab) < 0) {
    valid = 0;
  }
  if (semant_pass2(prog, tab) < 0) {
    valid = 0;
  }
  return valid ? 0 : -1;
}

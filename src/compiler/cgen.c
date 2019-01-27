/* cgen.h
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "cgen.h"
#include "symtab.h"
#include "ast.h"
#include "util.h"

#define PASS1 1
#define PASS2 2


/* bind_argument: binds argument to expression; returns pointer to expression upon
 * success, NULL if argument cannot be bound (e.g. is of kind ARGUMENT_REG)*/
struct expression *environment_bindarg(struct argument *arg,
				       struct environment *env) {
  switch (arg->kind) {
  case ARGUMENT_IMM64: return env->imm64;
  case ARGUMENT_EXPR:  return &arg->expr;
  case ARGUMENT_MEM:         return NULL;
  case ARGUMENT_REG:         return NULL;
  default:                     assert(0);
  }
}

void environment_init(struct environment *env, uint64_t *pc) {
  env->pc = pc;
  env->imm64 = NULL;
}

/* environment_construct: construct in context of rule reference */
void environment_construct(struct environment *newenv, struct environment *curenv,
			   struct arguments *def_args, struct arguments *ref_args) {
  struct argument *ref_arg_it, *ref_arg_end, *def_arg_it;
  struct expression *ref_expr;

  newenv->imm64 = NULL; // no binding of imm64 param by default 
  
  ref_arg_it = ref_args->argv;
  ref_arg_end = ref_arg_it + ref_args->argc;
  def_arg_it = def_args->argv;
  assert(def_args->argc == ref_args->argc);

  while (ref_arg_it < ref_arg_end) {
    if (def_arg_it->kind == ARGUMENT_IMM64) {
      /* bind ref_arg to expression */
      ref_expr = environment_bindarg(ref_arg_it, curenv);
      assert(ref_expr);
      /* assign expression to imm64 in new environment */
      newenv->imm64 = ref_expr;
      break;
    }
    
    ++ref_arg_it;
    ++def_arg_it;
  }
}


void codegen_1_block(struct block *block, struct environment *env,
		    struct expressions *exprlist) {
  struct instructions *instrs;
  struct instruction *instr_it, *instr_end;

  /* set address of block */
  block->addr = *env->pc;

  /* generate code for instructions */
  instrs = &block->instrs;
  for (instr_it = instrs->instrv, instr_end = instr_it + instrs->instrc;
       instr_it < instr_end; ++instr_it) {
    codegen_1_instruction(instr_it, env, exprlist);
  }
}

void codegen_1_defn(struct rule *defn, struct environment *env,
		   struct expressions *exprlist) {
  struct instructions *instrs;
  struct instruction *instr_it, *instr_end;

  /* generate code for instructions */
  instrs = &defn->definition;
  for (instr_it = instrs->instrv, instr_end = instr_it + instrs->instrc;
       instr_it < instr_end; ++instr_it) {
    codegen_1_instruction(instr_it, env, exprlist);
  }
}

struct expression resq_expr = { EXPRESSION_INT, { .num = 0 } };

void codegen_1_instruction(struct instruction *instr, struct environment *env,
			   struct expressions *exprlist) {
  struct arguments *args;
  struct argument *arg;
  struct expression *expr;
  uint64_t num;

  args = &instr->args;

  switch (instr->kind) {
  case INSTRUCTION_RET:
    assert(args->argc == 1);
    arg = &args->argv[0];
    expr = environment_bindarg(arg, env);
    assert(expr);
    expressions_add(expr, exprlist);
    ++(*env->pc);
    break;

  case INSTRUCTION_RESQ:
    /* add list of static expressions */
    assert(args->argc == 1);
    arg = &args->argv[0];
    expr = environment_bindarg(arg, env);
    assert(expr);
    num = compute_expression(expr, PASS1);
    for (uint64_t i = 0; i < num; ++i) {
      expressions_add(&resq_expr, exprlist);
    }
    *env->pc += num;
    break;
    
  case INSTRUCTION_DQ:
    assert(args->argc == 1);
    arg = &args->argv[0];
    expr = environment_bindarg(arg, env);
    assert(expr);
    expressions_add(expr, exprlist);
    ++(*env->pc);
    break;

  case INSTRUCTION_RULE:
    {
      struct environment subenv;
      struct arguments *def_args, *ref_args;
      /* init new environment */
      environment_init(&subenv, env->pc);
      
      /* set up environment with new imm64 binding */
      def_args = &instr->ref->args;
      ref_args = args;
      environment_construct(&subenv, env, def_args, ref_args);

      /* generate rule */
      codegen_1_defn(instr->ref, &subenv, exprlist);
      break;
    }

  default:
    assert(0);
  }
}


/* pass 1: construct expression list & set addresses of blocks */
void codegen_pass1(struct program *prog, uint64_t *pc, struct expressions *exprlist) {
  struct blocks *blocks;
  struct block *block_it, *block_end;
  struct environment env;

  /* initialize environment */
  environment_init(&env, pc);
  
  /* generate expressions for all code blocks */
  blocks = &prog->blocks;
  for (block_it = blocks->blockv, block_end = block_it + blocks->blockc;
       block_it < block_end; ++block_it) {
    codegen_1_block(block_it, &env, exprlist);
  }  
}


void codegen(struct program *prog, struct symtab *tab, uint64_t pc_origin,
	     uint64_t padding, uint64_t padding_val, FILE *outfile) {
  struct expressions exprlist; // intermediate list of expressions that will
                               // resolve to imm64's
  uint64_t pc;

  pc = pc_origin;
  expressions_init(&exprlist);

  /* produce padding */
  for (uint64_t i = 0; i < padding; ++i) {
    fwrite(&padding_val, sizeof(padding_val), 1, outfile);
  }
  
  /* pass 1 */
  codegen_pass1(prog, &pc, &exprlist);

  /* post-pass-1 assertions */
  assert(pc == pc_origin + exprlist.exprc);

  /* pass 2 */
  codegen_pass2(&exprlist, outfile);
  
}



/* note: `struct expressions' is actually a list of expression pointers */
void expressions_init(struct expressions *exprs) {
  memset(exprs, 0, sizeof(*exprs));
}

int expressions_add(struct expression *expr, struct expressions *exprs) {
  if (exprs->exprc == exprs->maxc) {
    /* resize */
    struct expression **exprv;
    int newc;
    newc = MAX(exprs->maxc*2, ARR_MINLEN);
    if ((exprv = realloc(exprs->exprv, sizeof(*exprv)*newc)) == NULL) {
      return -1; // internal error
    }
    exprs->exprv = exprv;
    exprs->maxc = newc;
  }
  exprs->exprv[exprs->exprc++] = expr;
  return 0;
}


/* computes expression (forces resolution to uint64_t) */
uint64_t compute_expression(const struct expression *expr, int pass) {
  switch (expr->kind) {
  case EXPRESSION_INT:
    return (uint64_t) expr->num;
    
  case EXPRESSION_PLUS:
    return compute_expression(expr->lhs, pass) + compute_expression(expr->rhs, pass);
    
  case EXPRESSION_MINUS:
    return compute_expression(expr->lhs, pass) - compute_expression(expr->rhs, pass);
    
  case EXPRESSION_EXT:
  case EXPRESSION_ID:
    return compute_symbol(expr->sym, pass);

  default:
    assert(0);
  }
}

/* returns 64-bit integer to which symbol resolves (symbol must be of type 
 * SYMBOL_BLOCK or SYMBOL_EQUATE) */
uint64_t compute_symbol(const struct symbol *sym, int pass) {
  const struct rule *equ;
  
  switch (sym->kind) {
  case SYMBOL_EQUATE:
    equ = sym->equ;
    assert(equ->kind == RULE_EQUATE);
    return compute_expression(&equ->equate, pass);
    
  case SYMBOL_BLOCK:
    assert (pass == PASS2); // block address will only be defined during PASS2
    return sym->blk->addr;
    
  case SYMBOL_EXTERN:
    fprintf(stderr, "cgen: not yet implemented.\n");
    assert(0);
    
  case SYMBOL_DEFINITION:
  case SYMBOL_REG:
  case SYMBOL_UNKNOWN:
  default:
    assert(0);
  }
}



/********************************************\
|******************* PASS 2 *****************|
\********************************************/

void codegen_pass2(struct expressions *exprlist, FILE *outfile) {
  struct expression **expr_it, **expr_end;
  uint64_t qword;

  /* evaluate expressions & write pseudo machine code to file */
  for (expr_it = exprlist->exprv, expr_end = expr_it + exprlist->exprc;
       expr_it < expr_end; ++expr_it) {
    qword = compute_expression(*expr_it, PASS2);
    fwrite(&qword, sizeof(qword), 1, outfile);
  }
}

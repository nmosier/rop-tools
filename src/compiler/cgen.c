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
#include "vec.h"
#include "stages.h"

#define PASS1 1
#define PASS2 2

#define CGEN_ERROR(srcinfo, desc, ...)		\
  GENERIC_ERROR("cgen", srcinfo, desc, __VA_ARGS__)

/* bind_argument: binds argument to expression; returns pointer to expression upon
 * success, NULL if argument cannot be bound (e.g. is of kind ARGUMENT_REG)*/
/* also replaces `$' (PC) with actual value */
struct expression *environment_bindarg(struct argument *arg,
				       const struct environment *env) {
  struct expression *boundexpr;

  switch (arg->kind) {
  case ARGUMENT_IMM64:
    return expression_dup(env->imm64);
    
  case ARGUMENT_EXPR:
    if ((boundexpr = expression_dup(&arg->expr)) == NULL) {
      return NULL;
    }
    expression_bindpc(boundexpr, env);
    return boundexpr;
    
  case ARGUMENT_MEM:
  case ARGUMENT_REG:
  case ARGUMENT_STR:
    return NULL;
    
  default:
    abort();
  }
}

void expression_bindpc(struct expression *expr, const struct environment *env) {
  switch (expr->kind) {
  case EXPRESSION_INT:
  case EXPRESSION_EXT:
  case EXPRESSION_ID:
  case EXPRESSION_ADDR:
    break;
    
  case EXPRESSION_PLUS:
  case EXPRESSION_MINUS:
  case EXPRESSION_DIV:
    expression_bindpc(expr->lhs, env);
    expression_bindpc(expr->rhs, env);
    break;
    
  case EXPRESSION_PC: // emplace current PC value
    expr->num = *env->pc;
    break;
    
  default:
    abort();
  }
}


/* expression_dup: perform deep-copy of expression expr */
struct expression *expression_dup(const struct expression *expr) {
  struct expression *newexpr;
  
  if ((newexpr = memdup(expr)) == NULL) {
    return NULL;
  }
  
  switch (expr->kind) {
  case EXPRESSION_PLUS:
  case EXPRESSION_MINUS:
  case EXPRESSION_DIV:
    if ((newexpr->lhs = expression_dup(expr->lhs)) == NULL) {
      free(newexpr);
      return NULL;
    }
    if ((newexpr->rhs = expression_dup(expr->rhs)) == NULL) {
      expression_free(newexpr->lhs);
      free(newexpr);
      return NULL;
    }
    break;

  default:
    break;
  }

  return newexpr;
}

void expression_free(struct expression *expr) {
  switch (expr->kind) {
  case EXPRESSION_PLUS:
  case EXPRESSION_MINUS:
  case EXPRESSION_DIV:
    expression_free(expr->lhs);
    expression_free(expr->rhs);
    break;
    
  default:
    break;
  }

  free(expr);
}

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

void environment_init(struct environment *env, uint64_t *pc,
		      uint64_t libc_base, const struct libc_syms *libc_syms) {
  env->pc = pc;
  env->imm64 = NULL;
  env->libc_base = libc_base;
  env->libc_syms = libc_syms;
}

/* environment_construct: construct in context of rule reference */
// note: only called during pass 1
void environment_construct(struct environment *newenv,
			   const struct environment *curenv,
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


void codegen_1_block(struct block *block, const struct environment *env,
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

void codegen_1_instruction(struct instruction *instr, const struct environment *env,
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
    *env->pc += QWORD_SIZE;
    break;

  case INSTRUCTION_RESQ:
    /* add list of static expressions */
    assert(args->argc == 1);
    arg = &args->argv[0];
    expr = environment_bindarg(arg, env);
    assert(expr);
    num = compute_expression(expr, env, PASS1);
    for (uint64_t i = 0; i < num; ++i) {
      expressions_add(&resq_expr, exprlist);
    }
    *env->pc += num * QWORD_SIZE;
    break;
    
  case INSTRUCTION_DQ:
    assert(args->argc == 1);
    arg = &args->argv[0];
    expr = environment_bindarg(arg, env);
    assert(expr);
    expressions_add(expr, exprlist);
    *env->pc += QWORD_SIZE;
    break;

  case INSTRUCTION_DB:
    codegen_1_instruction_db(instr, args, env, exprlist);
    break;

  case INSTRUCTION_ORG: // directive to set PC to given value
    assert(args->argc == 1);
    arg = &args->argv[0];
    expr = environment_bindarg(arg, env);
    *env->pc = compute_expression(expr, env, PASS1);
    fprintf(stderr, "cgen: new PC is 0x%lx\n", *env->pc);
    break;

  case INSTRUCTION_RULE:
    {
      struct environment subenv;
      struct arguments *def_args, *ref_args;
      
      /* init new environment */
      subenv = *env;
      
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

void codegen_1_instruction_db(struct instruction *instr, struct arguments *args,
			      const struct environment *env,
			      struct expressions *exprlist) {
  struct expression *expr;
  struct bytes bytes;
  struct argument *arg_it, *arg_end;
  uint8_t byte, *byte_it, *byte_end;
  uint64_t num, qword;
  int qwordi, qwordc, byte_i;
  struct expression qword_expr;
      
  /* initialization */
  bytes_init(&bytes);

  /* convert arguments into byte sequence */
  for (arg_it = args->argv, arg_end = arg_it + args->argc;
       arg_it < arg_end; ++arg_it) {
    /* check type of argument -- string or bindable argument */
    if (arg_it->kind == ARGUMENT_STR) {
      for (byte_it = (uint8_t *) arg_it->str; *byte_it; ++byte_it) {
	bytes_add(*byte_it, &bytes);
      }
    } else {
      /* bind argument */
      expr = environment_bindarg(arg_it, env);
      assert(expr);
	  
      /* compute qword expression and convert to byte */
      num = compute_expression(expr, env, PASS1);
      byte = (uint8_t) num;
      if ((int64_t) num != (int64_t) byte) {
	CGEN_ERROR(instr->srcinfo, "value %ld will be truncated to %d.", num,
		   byte);
      }

      /* add byte to byte array */
      bytes_add(byte, &bytes);

      /* free bound expression */
      expression_free(expr);
    }
  }

  /* group byte sequence into qword expressions */
  qwordc = bytes.bytec / 8 + (bytes.bytec % 8 ? 1 : 0); // divide, round up
  byte_it = bytes.bytev;
  byte_end = byte_it + bytes.bytec;
  for (qwordi = 0; qwordi < qwordc; ++qwordi) {
    /* initialize new qword value */
    qword = 0;

    for (byte_i = 0; byte_i < QWORD_SIZE && byte_it < byte_end;
	 ++byte_i, ++byte_it) {
      ((uint8_t *) &qword)[byte_i] = *byte_it;
    }

    /* create expression from qword number and insert into expressions vector */
    qword_expr.kind = EXPRESSION_INT;
    qword_expr.num = qword;
    expressions_add(&qword_expr, exprlist);
  }

  /* update program counter */
  *env->pc += qwordc * QWORD_SIZE;
}


/* pass 1: construct expression list & set addresses of blocks */
void codegen_pass1(struct program *prog, uint64_t *pc, struct expressions *exprlist,
		   const struct environment *env) {
  struct blocks *blocks;
  struct block *block_it, *block_end;

  /* generate expressions for all code blocks */
  blocks = &prog->blocks;
  for (block_it = blocks->blockv, block_end = block_it + blocks->blockc;
       block_it < block_end; ++block_it) {
    codegen_1_block(block_it, env, exprlist);
  }  
}


void codegen(struct program *prog, struct symtab *tab, uint64_t pc_origin,
	     uint64_t libc_base, const struct libc_syms *libc_syms,
	     uint64_t padding, uint64_t padding_val, int stages, FILE *outfile) {
  struct expressions exprlist; // intermediate list of expressions that will
                               // resolve to imm64's
  struct environment env;
  uint64_t pc;

  pc = pc_origin + padding;
  expressions_init(&exprlist);
  environment_init(&env, &pc, libc_base, libc_syms);

  /* produce padding */
  /*
  for (uint64_t i = 0; i < padding/QWORD_SIZE; ++i) {
    fwrite(&padding_val, sizeof(padding_val), 1, outfile);
  }
  */
  
  /* pass 1 */
  codegen_pass1_set_stagesyms(tab, stages, padding, pc_origin);
  codegen_pass1(prog, &pc, &exprlist, &env);

  /* post-pass-1 assertions */
  //  assert(pc == pc_origin + padding + exprlist.exprc*QWORD_SIZE);

  /* pass 2 */
  codegen_pass2_set_stagesyms(tab, stages, exprlist.exprc);
  codegen_pass2(&exprlist, &env, outfile);
  
}

void codegen_stage1_prologue(struct program *prog, struct symtab *tab,
			     uint64_t pc_origin, uint64_t libc_base,
			     const struct libc_syms *libc_syms, uint64_t padding,
			     uint64_t padding_val, int stages, FILE *outfile) {
  
}

vector_def(expressions, expression, expr);

/* computes expression (forces resolution to uint64_t) */
uint64_t compute_expression(const struct expression *expr,
			    const struct environment *env, int pass) {
  switch (expr->kind) {
  case EXPRESSION_INT:
  case EXPRESSION_PC:
    return (uint64_t) expr->num;
    
  case EXPRESSION_PLUS:
    return compute_expression(expr->lhs, env, pass)
      + compute_expression(expr->rhs, env, pass);
    
  case EXPRESSION_MINUS:
    return compute_expression(expr->lhs, env, pass)
      - compute_expression(expr->rhs, env, pass);

  case EXPRESSION_DIV:
    {
      uint64_t lhs = compute_expression(expr->lhs, env, pass),
	       rhs = compute_expression(expr->rhs, env, pass);
      if ((lhs % rhs)) {
	fprintf(stderr, "warning: left-hand side (=0x%lx) of expression is not\n" \
		"right-hand side (=0x%lx).\n", lhs, rhs);
      }
      return lhs / rhs;
    }
    
  case EXPRESSION_EXT:
  case EXPRESSION_ID:
    return compute_symbol(expr->sym, env, pass);

  case EXPRESSION_ADDR:
    return env->libc_base
      + compute_expression(expr->offset, env, pass);

    /*
  case EXPRESSION_PC:
    assert(pass == PASS1); // pc is only updated/valid in pass 1
    return *env->pc;
    */

  default:
    assert(0);
  }
}

/* returns 64-bit integer to which symbol resolves (symbol must be of type 
 * SYMBOL_BLOCK or SYMBOL_EQUATE) */
uint64_t compute_symbol(const struct symbol *sym, const struct environment *env,
			int pass) {
  const struct rule *equ;
  uint64_t offset;
  
  switch (sym->kind) {
  case SYMBOL_EQUATE:
    equ = sym->equ;
    assert(equ->kind == RULE_EQUATE);
    return compute_expression(&equ->equate, env, pass);
    
  case SYMBOL_BLOCK:
    assert (pass == PASS2); // block address will only be defined during PASS2
    return sym->blk->addr;
    
  case SYMBOL_EXTERN:
    if ((offset = libc_syms_getaddr(sym->name, env->libc_syms)) == (uint64_t) -1) {
      fprintf(stderr, "cgen: no such external symbol `%s' in libc.\n", sym->name);
      assert(0); // fix this later -- needs to propogate error up call stack
    }
    /* add offset to libc base address */
    return offset + env->libc_base;

  case SYMBOL_CONST:
    return sym->qconst;
    
  case SYMBOL_DEFINITION:
  case SYMBOL_REG:
  case SYMBOL_UNKNOWN:
  default:
    fprintf(stderr, "cgen: internal error: cannot resolve symbol `%s' of kind %d.\n",
	    sym->name, sym->kind);
    abort();
  }
}



/********************************************\
|******************* PASS 2 *****************|
\********************************************/

void codegen_pass2(struct expressions *exprlist, const struct environment *env,
		   FILE *outfile) {
  struct expression *expr_it, *expr_end;
  uint64_t qword;

  /* evaluate expressions & write pseudo machine code to file */
  for (expr_it = exprlist->exprv, expr_end = expr_it + exprlist->exprc;
       expr_it < expr_end; ++expr_it) {
    qword = compute_expression(expr_it, env, PASS2);
    fwrite(&qword, sizeof(qword), 1, outfile);
  }
}

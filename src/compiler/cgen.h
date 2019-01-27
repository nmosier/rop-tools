/* cgen.h
 * Nicholas Mosier 2019
 */

#ifndef __CGEN_H
#define __CGEN_H

#include <stdint.h>

#include "ast.h"
#include "symtab.h"
#include "util.h"

#define QWORD_SIZE 8

struct expressions {
  struct expression **exprv;
  int exprc;
  int maxc;
};
void expressions_init(struct expressions *exprs);
int expressions_add(struct expression *expr, struct expressions *exprs);


struct environment {
  uint64_t *pc;
  struct expression *imm64; // NULL if not bound
};


struct expression *environment_bindarg(struct argument *arg, struct environment *env);
void environment_init(struct environment *env, uint64_t *pc);
void environment_construct(struct environment *newenv, struct environment *curenv,
			   struct arguments *def_args, struct arguments *ref_args);

/* pass 1 */
void codegen_1_block(struct block *block, struct environment *env,
		     const struct libc_syms *libc_env,
		     struct expressions *exprlist);
void codegen_1_defn(struct rule *defn, struct environment *env,
		    const struct libc_syms *libc_env,
		    struct expressions *exprlist);
void codegen_pass1(struct program *prog, uint64_t *pc, struct expressions *exprlist,
		   const struct libc_syms *libc_env);
void codegen_1_instruction(struct instruction *instr, struct environment *env,
			   const struct libc_syms *libc_env,
			   struct expressions *exprlist);

void codegen(struct program *prog, struct symtab *tab,
	     const struct libc_syms *libc_env, uint64_t pc_origin,
	     uint64_t padding, uint64_t padding_val, FILE *outfile);

uint64_t compute_expression(const struct expression *expr, int pass,
			    const struct libc_syms *libc_env);
uint64_t compute_symbol(const struct symbol *sym, int pass,
			const struct libc_syms *libc_env);
void codegen_pass2(struct expressions *exprlist, const struct libc_syms *libc_env,
		   FILE *outfile);

#endif

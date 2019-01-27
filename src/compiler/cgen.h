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
  const struct libc_syms *libc_syms;
  uint64_t libc_base;
};


struct expression *environment_bindarg(struct argument *arg,
				       const struct environment *env);
void environment_init(struct environment *env, uint64_t *pc,
		      uint64_t libc_base, const struct libc_syms *libc_syms);
void environment_construct(struct environment *newenv,
			   const struct environment *curenv,
			   struct arguments *def_args, struct arguments *ref_args);

/* pass 1 */
void codegen_pass1(struct program *prog, uint64_t *pc, struct expressions *exprlist,
		   const struct environment *env);
void codegen_1_block(struct block *block, const struct environment *env,
		     struct expressions *exprlist);
void codegen_1_defn(struct rule *defn, struct environment *env,
		    struct expressions *exprlist);
void codegen_1_instruction(struct instruction *instr, const struct environment *env,
			   struct expressions *exprlist);

void codegen(struct program *prog, struct symtab *tab, uint64_t pc_origin,
	     uint64_t libc_base, const struct libc_syms *libc_syms,
	     uint64_t padding, uint64_t padding_val, FILE *outfile);

uint64_t compute_expression(const struct expression *expr,
			    const struct environment *env, int pass);
uint64_t compute_symbol(const struct symbol *sym, const struct environment *env,
			int pass);
void codegen_pass2(struct expressions *exprlist, const struct environment *env,
		   FILE *outfile);

#endif

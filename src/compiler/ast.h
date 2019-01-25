#ifndef __GRAMMAR_H
#define __GRAMMAR_H


#include <stdint.h>

/* source code info */
#define AST_SRCINFO_FILENAME_MAXLEN 32
struct srcinfo {
  int lineno;
  char filename[AST_SRCINFO_FILENAME_MAXLEN]; // fix later? 
};

/* AST nodes */

struct expression {
  enum expression_kind {EXPRESSION_SYM,
			EXPRESSION_ID,
			EXPRESSION_INT,
			EXPRESSION_PLUS,
		        EXPRESSION_MINUS}
    kind; /* in case of binop, kind = binop token */
  union {
    struct {
      struct expression *lhs;
      struct expression *rhs;
    };
    struct symbol *sym;
    int64_t num;
  };
};
const char *expression_kind2str(enum expression_kind kind);

struct argument {
  enum argument_kind {ARGUMENT_IMM64,
		      ARGUMENT_REG,
		      ARGUMENT_MEM,
		      ARGUMENT_EXPR
  } kind;
  union {
    struct symbol *reg;
    struct expression expr;
  };
};

struct arguments {
  struct argument *argv;
  int argc;
  int maxc;
};
void arguments_init(struct arguments *args);
int arguments_add(struct argument *arg, struct arguments *args);

struct instruction {
  enum instruction_kind { INSTRUCTION_RET, INSTRUCTION_RESQ,
			  INSTRUCTION_DQ, INSTRUCTION_RULE } kind;
  struct arguments args;
  struct symbol *sym;
  struct rule *ref; // rule that it is a reference to
  struct srcinfo srcinfo;
};

struct instructions {
  struct instruction *instrv;
  int instrc;
  int maxc;
};
void instructions_init(struct instructions *instrs);
int instructions_add(struct instruction *instr, struct instructions *instrs);

enum rule_kind { RULE_DEFINITION, RULE_EQUATE };

struct rule {
  enum rule_kind kind;
  struct symbol *sym;
  struct arguments args;
  union {
    struct instructions definition;
    struct expression equate;
  };
  struct srcinfo srcinfo;
};

struct rules {
  struct rule *rulev;
  int rulec;
  int maxc;
};
void rules_init(struct rules *rules);
int rules_add(struct rule *rule, struct rules *rules);

struct block {
  struct symbol *sym;
  struct instructions instrs;
  struct srcinfo srcinfo;
};

struct blocks {
  struct block *blockv;
  int blockc;
  int maxc;
};
void blocks_init(struct blocks *blocks);
int blocks_add(struct block *block, struct blocks *blocks);

struct program {
  struct rules rules;
  struct blocks blocks;
};
void program_init(struct program *prog);

/* comparison functions */
int rule_cmp(const struct rule *r1, const struct rule *r2);
int block_cmp(struct block *blk1, struct block *blk2);
int arguments_cmp(const struct arguments *a1, const struct arguments *a2);
int argument_cmp(const struct argument *a1, const struct argument *a2);

/* `has' functions */
int rules_has(struct rule *rule, struct rules *rules);

/* `match' functions */
int argument_match(const struct argument *ref, const struct argument *def);
int arguments_match(const struct arguments *ref, const struct arguments *def);
int instruction_match_rule(const struct instruction *instr, const struct rule *rule);

#endif

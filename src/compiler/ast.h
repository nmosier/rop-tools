#ifndef __GRAMMAR_H
#define __GRAMMAR_H


#include <stdint.h>

/* limits */

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
    char *sym;
    char *id;
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
    char *name; // optional
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

struct instruction_prefix {
  enum instruction_prefix_kind { PREFIX_RET, PREFIX_RESQ, PREFIX_DQ, PREFIX_ID } kind;
  char *val;
};

struct instruction {
  struct instruction_prefix prefix;
  struct arguments args;
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
  char *id;
  struct arguments args;
  union {
    struct instructions definition;
    struct expression equate;
  };
};

struct rules {
  struct rule *rulev;
  int rulec;
  int maxc;
};
void rules_init(struct rules *rules);
int rules_add(struct rule *rule, struct rules *rules);


//extern int yylloc;

#endif

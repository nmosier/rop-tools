#ifndef __GRAMMAR_H
#define __GRAMMAR_H

#include "token.h"

/* limits */

struct expression {
  enum token kind; /* in case of binop, kind = binop token */
  union {
    struct {
      struct expression *lhs;
      struct expression *rhs;
    } binop;
    char *sym;
    char *id;
    int64_t num;
  };
};

struct argument {
  enum argument_kind { ARGUMENT_IMM64, ARGUMENT_REG, ARGUMENT_MEM, ARGUMENT_EXPR };
  union {
    char *name; // optional
    struct expression expr;
  }
};

struct arguments {
  struct argument *argv;
  int argc;
  int maxc;
};
void arguments_init(struct arguments *args);
int arguments_add(struct argument *arg, struct arguments *args);

struct instruction_prefix {
  enum token kind;
  char *val;
};

struct instruction {
  struct instruction_prefix prefix;
  struct arguments args;
};

struct instructions {
  struct instruction *instrv;
  int instrc;
};

struct definition {
  char *id;
  struct arguments args;
  struct instructions instrs;
};

struct equate {
  char *id;
  struct expression expr;
};

struct rule {
  enum rule { DEFINITION, EQUATE };
  union {
    struct definition;
    struct equate;
  };
};

struct rules {
  struct rule *rulev;
  int rulec;
}  
  
#endif

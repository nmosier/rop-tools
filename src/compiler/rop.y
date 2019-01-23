%{
  #include <stdio.h>
  #include <stdint.h>
  #include "ast.h"
  #include "util.h"
  
  #define YYDEBUG 1
  #define YYLTYPE int
  #define YYLLOC_DEFAULT(Cur, Rhs, N)			\
    (Cur) = (N) ? YYRHSLOC(Rhs, 1) : YYRHSLOC(Rhs, 0);

  void yyerror(const char *);
  int yylex(void);
  extern int lineno;
  extern struct rules rop_rules;
%}

	/* declarations */
/* union declaration */
%union {
  const char *err_msg;
  char *name;
  char *reg;
  int64_t num;
  struct expression expression;
  struct argument argument;
  struct arguments arguments;
  struct instruction_prefix instruction_prefix;
  struct instruction instruction;
  struct instructions instructions;
  struct rule rule;
  struct rules rules;
}

/* terminals */
%token ERROR
%token DEF
%token ARGSEP
%token MEMLEFT
%token MEMRIGHT
%token RET
%token DQ
%token RESQ
%token INDENT
%token <reg> REG
%token <num> INT
%token IMM64
%token <name> SYMBOL
%token <name> IDENTIFIER
%token NEWLINE

/* non-terminals */
%type <expression> expression
%type <instruction_prefix> instruction_prefix
%type <instruction> instruction_line
%type <instructions> instruction_lines
%type <argument> argument
%type <arguments> argument_list
%type <arguments> optional_argument_list
%type <rule> rule_body
%type <rule> rule
%type <rule> equate_body
%type <rule> definition_body
%type <rules> optional_rule_list

/* precedenance declarations */
%left PLUS
%left MINUS

%%
	/* grammar rules */

 /* start symbol */
optional_rule_list:
  /* empty */ { rules_init(&$$); @$ = 1; }
| optional_rule_list rule { rules_add(&$2, &$$); rules_add(&$2, &rop_rules); @$ = @1; }


/* must be constant */
expression:
  SYMBOL  { $$.kind = EXPRESSION_SYM; $$.sym = $1; }
  | IDENTIFIER { $$.kind = EXPRESSION_ID; $$.id = $1; }
  | INT { $$.kind = EXPRESSION_INT; $$.num = $1; }
  | expression PLUS expression {
    $$.kind = EXPRESSION_PLUS;
    $$.lhs = memdup(&$1);
    $$.rhs = memdup(&$3);
  }
  | expression MINUS expression { $$.kind = EXPRESSION_MINUS; $$.lhs = &($1); $$.rhs = &($3); }

argument:
  IMM64 { $$.kind = ARGUMENT_IMM64; }
  | REG { $$.kind = ARGUMENT_REG; $$.name = $1; }
  | MEMLEFT REG MEMRIGHT { $$.kind = ARGUMENT_MEM; $$.name = $2; }
  | expression { $$.kind = ARGUMENT_EXPR; $$.expr = $1; }

argument_list:
  argument { arguments_init(&($$)); arguments_add(&$1, &$$); }
  | argument_list ARGSEP argument { arguments_add(&$3, &$$); }

optional_argument_list:
  /* empty */ { arguments_init(&$$); }
  | argument_list { $$ = $1; }

instruction_prefix:
  RET { $$.kind = PREFIX_RET; }
  | RESQ { $$.kind = PREFIX_RESQ; }
| DQ { $$.kind = PREFIX_DQ; }
  | IDENTIFIER { $$.kind = PREFIX_ID; $$.val = $1; }

instruction_line:
  INDENT instruction_prefix optional_argument_list NEWLINE { $$.prefix = $2; $$.args = $3; }

instruction_lines:
  instruction_line { instructions_init(&$$); instructions_add(&$1, &$$); }
  | instruction_lines instruction_line { instructions_add(&$2, &$$); }

definition_body:
  NEWLINE instruction_lines {
    $$.kind = RULE_DEFINITION;
    $$.definition = $2;
  }

equate_body:
  expression NEWLINE { $$.kind = RULE_EQUATE; $$.equate = $1; }

rule_body:
  definition_body { $$ = $1; }
  | equate_body { $$ = $1; }

rule:
  IDENTIFIER optional_argument_list DEF rule_body {
    $$ = $4;
    $$.id = $1;
    $$.args = $2;
  }
  
%%
	/* epilogue */
void yyerror(const char *s) {
  fprintf(stderr, "rop-parser: line %d %d: %s\n", lineno, yychar, s);
}

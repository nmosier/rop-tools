%{
  #include <stdio.h>
  #include "ast.h"

  #define YYDEBUG 1
  #define YYLTYPE int
  #define YYLLOC_DEFAULT(Cur, Rhs, N)		\
    (Cur) = (N) ? YYRHSLOC(Rhs, 1) : YYRHSLOC(Rhs, 0);

  void yyerror(const char *);
  int yylex(void);
  extern int lineno;
%}

	/* declarations */
/* union declaration */
%union {
  const char *err_msg;
  char *name;
  char *reg;
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
%token <expression> INT
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
  | optional_rule_list rule { $$ = $1; @$ = @1; }


/* must be constant */
expression:
  SYMBOL  { $$.kind = SYMBOL; $$.sym = $1; }
  | IDENTIFIER { $$.kind = IDENTIFIER; $$.id = $1; }
  | INT { $$ = $1; }
  | expression PLUS expression { $$.kind = PLUS; $$.lhs = &($1); $$.rhs = &($3); }
  | expression MINUS expression { $$.kind = MINUS; $$.lhs = &($1); $$.rhs = &($3); }

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
  RET { $$.kind = RET; }
  | RESQ { $$.kind = RESQ; }
  | DQ { $$.kind = DQ; }
  | IDENTIFIER { $$.kind = IDENTIFIER; $$.val = $1; }

instruction_line:
  INDENT instruction_prefix optional_argument_list NEWLINE { $$.prefix = $2; $$.args = $3; }

instruction_lines:
  instruction_line { instructions_init(&$$); instructions_add(&$1, &$$); }
  | instruction_lines instruction_line { instructions_add(&$2, &$$); }

definition_body:
  NEWLINE instruction_lines {
    $$.kind = DEFINITION;
    $$.definition = $2;
  }

equate_body:
  expression NEWLINE { $$.kind = EQUATE; $$.equate = $1; }

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

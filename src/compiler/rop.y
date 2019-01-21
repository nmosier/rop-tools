%{
  #include "ast.h"
%}

	/* declarations */
/* union declaration */
%union {
  const char *err_msg;
  char *name;
  char *reg;
  struct expression;
  struct argument;
  struct arguments;
  struct instruction_prefix;
  struct instruction;
  struct instructions;
  struct equate;
  struct rule;
  struct rules;
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
%type <rules> optional_rule_list
%type <definition> definition
%type <equate> equate
%type <rule> rule
%type <instruction_prefix> instruction_prefix
%type <instruction> instruction_line
%type <instructions> instruction_lines
%type <argument> argument
%type <arguments> argument_list
%type <arguments> optional_argument_list

/* precedenance declarations */
%left PLUS
%left MINUS

%%
	/* grammar rules */

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
  | MEMLEFT REG MEMRIGHT { $$.kind = ARGUMENT_MEM; $$.name = $2 }
  | expression { $$.kind = ARGUMENT_EXPR; $$.expr = $1; }

argument_list:
  argument { arguments_init(&($$)); arguments_add(&$1, &$$); }
  | argument_list ARGSEP argument { arguments_add(&$1, &$$); }

optional_argument_list:
  /* empty */
  | argument_list

instruction_prefix:
  RET
  | RESQ
  | DQ
  | IDENTIFIER

instruction_line:
  INDENT instruction_prefix optional_argument_list NEWLINE

instruction_lines:
  instruction_line
  | instruction_lines instruction_line

definition:
  IDENTIFIER optional_argument_list DEF instruction_lines

equate:
  IDENTIFIER DEF expression NEWLINE

rule:
  definition
  | equate

optional_rule_list:
  /* empty */
  | optional_rule_list rule
  
%%
	/* epilogue */

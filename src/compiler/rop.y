%{
  #include "ast.h"

  #define YYLTYPE int
  #define YYLLOC_DEFAULT(Cur, Rhs, N)		\
    (Cur) = (N) ? (YYRHSLOC(Rhs, 1)) : YYRHSLOC(Rhs, 0));
  
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
%type <expression> equate_body
%type <definition> definition_body
%type <instruction_prefix> instruction_prefix
%type <instruction> instruction_line
%type <instructions> instruction_lines
%type <argument> argument
%type <arguments> argument_list
%type <arguments> optional_argument_list
%type <rule> rule_body
%type <rule> rule
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
  | MEMLEFT REG MEMRIGHT { $$.kind = ARGUMENT_MEM; $$.name = $2 }
  | expression { $$.kind = ARGUMENT_EXPR; $$.expr = $1; }

argument_list:
  argument { arguments_init(&($$)); arguments_add(&$1, &$$); }
  | argument_list ARGSEP argument { arguments_add(&$3, &$$); }

optional_argument_list:
  /* empty */ { arguments_init(&$$); }
  | argument_list { arguments_init(&$$); arguments_add(&$1, &$$); }

instruction_prefix:
  RET { $$.kind = RET; }
  | RESQ { $$.kind = RESQ; }
  | DQ { $$.kind = DQ; }
  | IDENTIFIER { $$.kind = IDENTIFIER; $$.val = $1.name; }

instruction_line:
  INDENT instruction_prefix optional_argument_list NEWLINE { $$.prefix = $2; $$.args = $3; }

instruction_lines:
  instruction_line { instructions_init(&$$); instructions_add(&$1, &$$); }
  | instruction_lines instruction_line { instructions_add(&$2, &$$); }

definition_body:
  optional_argument_list DEF NEWLINE instruction_lines {
    $$.args = $1;
    $$.instrs = $4;
  }

equate_body:
  optional_argument_list DEF expression NEWLINE { $$ = $3; }

rule_body:
  definition_body { $$.kind = DEFINITION; $$.definition = $1; }
  | equate_body { $$.kind = EQUATE; $$.equate = $1; }

rule:
IDENTIFIER rule_body { $$ = $2; $$.id = $1; }

  
%%
	/* epilogue */

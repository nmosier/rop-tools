%{
  #include <stdio.h>
  #include <stdint.h>
  #include <string.h>
  #include "ast.h"
  #include "util.h"
  #include "symtab.h"
  
  #define YYDEBUG 1
  #define YYLTYPE int
  #define YYLLOC_DEFAULT(Cur, Rhs, N)			\
    (Cur) = (N) ? YYRHSLOC(Rhs, 1) : YYRHSLOC(Rhs, 0);

  void yyerror(const char *);
  int yylex(void);
  extern int lineno;
  extern const char *filename;
  extern struct program rop_program;
%}

	/* declarations */
/* union declaration */
%union {
  const char *err_msg;
  struct symbol *symbol;
  int64_t num;
  struct expression expression;
  struct argument argument;
  struct arguments arguments;
  struct instruction instruction;
  struct instructions instructions;
  struct rule rule;
  struct rules rules;
  struct block block;
  struct blocks blocks;
  struct program program;
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
%token <symbol> REG
%token <num> INT
%token IMM64
%token <symbol> SYMBOL
%token <symbol> IDENTIFIER
%token NEWLINE
%token LABEL
%token NEWSEG
%token ADDR


/* non-terminals */
%type <expression> expression

%type <instruction> instruction_prefix
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

%type <block> code_block
%type <blocks> optional_code_block_list

%type <program> program


/* precedenance declarations */
%left PLUS
%left MINUS

%%
	/* grammar rules */
    /* start symbol */
program:
  optional_rule_list NEWSEG optional_code_block_list {
    $$.rules = $1; $$.blocks = $3;
  }

/* must be constant */
expression:
  SYMBOL  { $$.kind = EXPRESSION_EXT; $$.sym = $1; }
  | IDENTIFIER { $$.kind = EXPRESSION_ID; $$.sym = $1; }
  | INT { $$.kind = EXPRESSION_INT; $$.num = $1; }
  | expression PLUS expression {
    $$.kind = EXPRESSION_PLUS;
    $$.lhs = memdup(&$1);
    $$.rhs = memdup(&$3);
  }
  | expression MINUS expression {
      $$.kind = EXPRESSION_MINUS; $$.lhs = &($1); $$.rhs = &($3);
    }
  | ADDR expression {
      $$.kind = EXPRESSION_ADDR;
      $$.offset = memdup(&$2);
    }

argument:
  IMM64 { $$.kind = ARGUMENT_IMM64; }
  | REG { $$.kind = ARGUMENT_REG; $$.reg = $1; }
  | MEMLEFT REG MEMRIGHT { $$.kind = ARGUMENT_MEM; $$.reg = $2; }
  | expression { $$.kind = ARGUMENT_EXPR; $$.expr = $1; }

argument_list:
  argument { arguments_init(&($$)); arguments_add(&$1, &$$); }
  | argument_list ARGSEP argument { arguments_add(&$3, &$$); }

optional_argument_list:
  /* empty */ { arguments_init(&$$); }
  | argument_list { $$ = $1; }

instruction_prefix:
  RET { $$.kind = INSTRUCTION_RET; }
  | RESQ { $$.kind = INSTRUCTION_RESQ; }
  | DQ { $$.kind = INSTRUCTION_DQ; }
  | IDENTIFIER {
      $$.kind = INSTRUCTION_RULE;
      $$.sym = $1;
      $$.sym->kind = SYMBOL_DEFINITION;
    }

instruction_line:
  INDENT instruction_prefix optional_argument_list NEWLINE {
    $$ = $2; $$.args = $3; $$.srcinfo.lineno = lineno;
    strncpy($$.srcinfo.filename, filename, AST_SRCINFO_FILENAME_MAXLEN);
  }

instruction_lines:
  instruction_line { instructions_init(&$$); instructions_add(&$1, &$$); }
  | instruction_lines instruction_line { $$ = $1; instructions_add(&$2, &$$); }

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
    $$.sym = $1;
    $1->kind = rulek2symk($4.kind);
    $$.args = $2;
    $$.srcinfo.lineno = lineno;
    strncpy($$.srcinfo.filename, filename, AST_SRCINFO_FILENAME_MAXLEN);
  }

optional_rule_list:
  /* empty */ { rules_init(&$$); @$ = 1; }
  | optional_rule_list rule {
    $$ = $1; rules_add(&$2, &$$); /* rules_add(&$2, &rop_rules); */ @$ = @1;
    rules_add(&$2, &rop_program.rules);
  }

code_block:
  IDENTIFIER LABEL NEWLINE instruction_lines {
    $1->kind = SYMBOL_BLOCK; $$.sym = $1; $$.instrs = $4;
    $$.srcinfo.lineno = lineno;
    strncpy($$.srcinfo.filename, filename, AST_SRCINFO_FILENAME_MAXLEN);
  }

optional_code_block_list:
  /* empty */ { blocks_init(&$$); }
  | optional_code_block_list code_block {
    $$ = $1; blocks_add(&$2, &$$);
    blocks_add(&$2, &rop_program.blocks);
  }

%%
	/* epilogue */
void yyerror(const char *s) {
  fprintf(stderr, "rop-parser: %s:%d %d: %s\n", filename, lineno, yychar, s);
}

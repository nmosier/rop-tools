%{
#include <stdio.h>
#include <assert.h>

#include "ast.h"
#include "rop.tab.h"
#include "symtab.h"
#include "util.h"
  
#define LEXER_DEBUG 0
#define LOG(msg) if (LEXER_DEBUG) fprintf(stderr, "%s:%d: %s\n", \
					  filename, lineno, msg)

  extern int lineno;
  extern const char *filename;
  extern struct symtab rop_symtab;
%}

	/* DEFINITIONS */
WHITESPACE 	       [ \t]

		       /* non-terminating comment after star `*' */
		       /* (already consumed) */
		       /*MULTICOMM	       [^/*\n]**/
MULTICOMM	       ([^*\n]("*"+[^/\n])?)*"*"*
NUMBER_DEC	       "-"?[[:digit:]]+
NUMBER_HEX	       "-"?0x[[:xdigit:]]+
CHAR_ESCAPE            [\\][[:print:][:space:]]
CHAR_PLAIN             [[:graph:][:blank:]]{-}[\\\"]
STRING_BODY            ({CHAR_ESCAPE}|{CHAR_PLAIN})*
STRING                 \"{STRING_BODY}\"

        /* conditions */
%x multiline
   /* %x singleline */
%option noyywrap

%%
	/* RULES */

	/* single-line comment */
^{WHITESPACE}*("//"|";")[[:print:]]*\n	{ ++lineno; }
("//"|";")[[:print:]]*\n	{ ++lineno; return NEWLINE; }
					
	/* multiline comment */
<multiline>{MULTICOMM}"*/"{WHITESPACE}*\n  {
                       ++lineno; BEGIN(INITIAL); LOG("comment end");
		     }
<multiline>{MULTICOMM}"*/"  { BEGIN(INITIAL); LOG("comment end"); }
<multiline>{MULTICOMM}\n       { ++lineno; LOG("comment in");}
{WHITESPACE}*"/*"              { BEGIN(multiline); LOG("comment begin"); }

			      /* whitespace rules */
			     /* indentation (after newline) */
^{WHITESPACE}+\n        {}
^{WHITESPACE}+		{ return INDENT; }
	/* consume rest of line */
{WHITESPACE}+	   {}
^[\n] { ++lineno; }
[\n]	{ ++lineno; return NEWLINE; }


	/* regs */
"r"([abcd]"x"|[sd]"i"|[0-9]+|[sb]"p") {
  struct symbol *regsym = symtab_put_bare(yytext, &rop_symtab);
  regsym->kind = SYMBOL_REG;
  yylval.symbol = regsym; return REG;
}

	/* single-character tokens */
"["			      { return MEMLEFT; }
"]"			      { return MEMRIGHT; }
","			      { return ARGSEP; }
"+"			      { return PLUS; }
"-"			      { return MINUS; }
":="			      { return DEF; }
"#"+\n                        { return NEWSEG; }
":"                           { return LABEL; }
"&"                           { return ADDR; }
"$"                           { return PC; }

         /* keywords */
ret			      { return RET; }
imm64			      { return IMM64; }
dq			      { return DQ; }
db                            { return DB; }
resq			      { return RESQ; }


	/* numbers */
{NUMBER_DEC}		      { yylval.num = strtoll(yytext, NULL, 10);
                                return INT; }
{NUMBER_HEX}                  { yylval.num = strtoll(yytext, NULL, 16);
                                 return INT; }


	/* identifiers */
[A-Z_][A-Z0-9_]*    {
  /* insert into symbol table */
  struct symbol *sym = symtab_put_bare(yytext, &rop_symtab);
  assert (sym);
  yylval.symbol = sym;
  // yylval.name = strdup(yytext);
  return IDENTIFIER;
}
        /* symbols */
"<"[[:graph:]]+">"	{
  char *name = strdup(yytext + 1);
  name[yyleng - 2] = '\0';
  struct symbol *sym = symtab_put_bare(name, &rop_symtab);
  free(name);
  assert (sym);
  sym->kind = SYMBOL_EXTERN; // we already know this b/c of the angle brackets
  yylval.symbol = sym;
  //yylval.name = strdup(yytext);
  return SYMBOL;
}

.			      { fprintf(stderr, "flex: illegal character %c \\%03o\n",
			        *yytext, *yytext); return ERROR; }
<<EOF>>			      { yyterminate(); }

%%
	/* CODE */


  

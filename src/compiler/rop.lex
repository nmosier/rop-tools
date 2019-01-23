%{
#include <stdio.h>

#include "ast.h"
#include "rop.tab.h"

#define LEXER_DEBUG 0
#define LOG(msg) if (LEXER_DEBUG) fprintf(stderr, "%d: %s\n", lineno, msg)

  int lineno;
  //extern YYSTYPE yylval;
%}

	/* DEFINITIONS */
WHITESPACE 	       [ \t]

		       /* non-terminating comment after star `*' */
		       /* (already consumed) */
		       /*MULTICOMM	       [^/*\n]**/
MULTICOMM	       ([^*\n]("*"+[^/\n])?)*"*"*
NUMBER_DEC	       "-"?[[:digit:]]+
NUMBER_HEX	       "-"?0x[[:xdigit:]]+
  //NUMBER		       "-"?({NUMBER_DEC}|{NUMBER_HEX})

        /* conditions */
%x multiline
   /* %x singleline */
%option noyywrap

%%
	/* RULES */

	/* single-line comment */
^("//"|";")[[:print:]]*\n	{ ++lineno; }
("//"|";")[[:print:]]*\n	{ ++lineno; return NEWLINE; }
					
	/* multiline comment */
<multiline>{MULTICOMM}"*/"\n  { ++lineno; BEGIN(INITIAL); LOG("comment end"); }
<multiline>{MULTICOMM}"*/"  { BEGIN(INITIAL); LOG("comment end"); }
<multiline>{MULTICOMM}\n       { ++lineno; LOG("comment in");}
"/*"	   	              { BEGIN(multiline); LOG("comment begin"); }

			      /* whitespace rules */
			     /* indentation (after newline) */
^{WHITESPACE}+		{ return INDENT; }
	/* consume rest of line */
{WHITESPACE}+	   {}
^[\n] { ++lineno; }
[\n]	{ ++lineno; return NEWLINE; }


	/* regs */
"r"([abcd]"x"|[sd]"i"|[0-9]+|"bp") { yylval.reg = strdup(yytext); return REG; }

	/* single-character tokens */
"["			      { return MEMLEFT; }
"]"			      { return MEMRIGHT; }
","			      { return ARGSEP; }
"+"			      { return PLUS; }
"-"			      { return MINUS; }
":="			      { return DEF; }
ret			      { return RET; }
imm64			      { return IMM64; }
dq			      { return DQ; }
resq			      { return RESQ; }
			      
	/* numbers */
{NUMBER_DEC}		      { yylval.num = strtoll(yytext, NULL, 10);
                                return INT; }
{NUMBER_HEX}                  { yylval.num = strtoll(yytext, NULL, 16);
                                 return INT; }


	/* identifiers */
[A-Z_][A-Z0-9_]*		      { yylval.name = strdup(yytext); return IDENTIFIER; }
        /* symbols */
"<"[[:graph:]]+">"		      { yylval.name = strdup(yytext); return SYMBOL; }

.			      { fprintf(stderr, "flex: illegal character %c \\%03o\n",
			        *yytext, *yytext); return ERROR; }
<<EOF>>			      { yyterminate(); }

%%
	/* CODE */


  

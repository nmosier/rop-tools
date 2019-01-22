%{
#include <stdio.h>
  //#include "tokens.h"
#include "ast.h"
#include "rop.tab.h"

  #define DEBUG 1
#define LOG(msg) if (DEBUG) fprintf(stderr, "%d: %s\n", lineno, msg)

  int lineno;
%}

	/* DEFINITIONS */
WHITESPACE 	       [ \t]

		       /* non-terminating comment after star `*' */
		       /* (already consumed) */
		       /*MULTICOMM	       [^/*\n]**/
MULTICOMM	       ([^*\n]("*"+[^/\n])?)*"*"*
NUMBER_DEC	       [[:digit:]]+
NUMBER_HEX	       0x[[:xdigit:]]+
NUMBER		       "-"?({NUMBER_DEC}|{NUMBER_HEX})

        /* conditions */
%x multiline
   /* %x singleline */
%option noyywrap

%%
	/* RULES */

	/* single-line comment */
("//"|";")[[:print:]]*	{}
					
	/* multiline comment */
<multiline>{MULTICOMM}"*/"     { BEGIN(INITIAL); LOG("comment begin"); }
<multiline>{MULTICOMM}\n       { ++lineno; LOG("comment in");}
"/*"	   	              { BEGIN(multiline); LOG("comment end"); }

			      /* whitespace rules */
			     /* indentation (after newline) */
^{WHITESPACE}+		{ return INDENT; }
	/* consume rest of line */
{WHITESPACE}+	   {}
^[\n] { ++lineno; }
[\n]+	{ ++lineno; return NEWLINE; }


	/* regs */
"r"([abcd]"x"|[sd]"i"|[0-9]+|"bp") { return REG; }

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
{NUMBER}		      { return INT; }


	/* identifiers */
[A-Z_][A-Z0-9_]*		      { return IDENTIFIER; }
        /* symbols */
"<"[[:graph:]]">"		      { return SYMBOL; }

.			      { fprintf(stderr, "flex: illegal character %c \\%03o\n",
			        *yytext, *yytext); return ERROR; }
<<EOF>>			      { yyterminate(); }

%%
	/* CODE */


  

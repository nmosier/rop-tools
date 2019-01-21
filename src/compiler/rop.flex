%{
#include <stdio.h>
#include "tokens.h"
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
			/* <singleline><<EOF>>	{ yyterminate(); } */
			/* <singleline>\n		{ BEGIN(INITIAL); ++lineno; } */

					
	/* multiline comment */
<multiline>{MULTICOMM}"*/"       { BEGIN(INITIAL); }
<multiline>{MULTICOMM}\n       { ++lineno; }
"/*"	   	              { BEGIN(multiline); }

			      /* whitespace rules */
			     /* indentation (after newline) */
^{WHITESPACE}+		{ return INDENT; }
	/* consume rest of line */
{WHITESPACE}+	   {}
\n	{ ++lineno; }


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


  
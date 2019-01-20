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



        /* conditions */
%x multiline
   /* %x singleline */
%option noyywrap

%%
	/* RULES */

	/* consume rest of line */
{WHITESPACE}*\n	{ ++lineno; }

	/* single-line comment */
("//"|";")[[:print:]]*	{}
			/* <singleline><<EOF>>	{ yyterminate(); } */
			/* <singleline>\n		{ BEGIN(INITIAL); ++lineno; } */

					
	/* multiline comment */
<multiline>{MULTICOMM}"*/"       { BEGIN(INITIAL); }
<multiline>{MULTICOMM}\n       { ++lineno; }
"/*"	   	              { BEGIN(multiline); }

        /* indentation */
{WHITESPACE}+		{ return INDENT; } 

	/* regs */
"r"([abcd]"x"|[sd]"i"|[0-9]+|"bp") { return REG; }

	/* single-character tokens */
"["			      { return MEMLEFT; }
"]"			      { return MEMRIGHT; }
","			      { return ARGSEP; }

":="			      { return DEF; }
ret			      { return RET; }
			      
	/* numbers */
"-"?"0x"?[0-9]+		      { return IMM; }

	/* identifiers */
[A-Z][A-Z0-9]*		      { return IDENTIFIER; }
        /* symbols */
"<"[[:graph:]]">"		      { return SYMBOL; }

.			      { fprintf(stderr, "flex: illegal character %c \\%03o\n",
			        *yytext, *yytext); return ERROR; }
<<EOF>>			      { yyterminate(); }

%%
	/* CODE */


  
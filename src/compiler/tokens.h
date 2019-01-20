#ifndef __TOKENS_H
#define __TOKENS_H

enum tokens {
	     ERROR = -1,
	     DEF = 1, // `:='
	     ARGSEP, // `,'
	     COMMENT, // `//' or `;'
	     MULTICOMMENT, // `/* ... */'
	     REG,
	     IDENTIFIER,
	     MEMLEFT, // `['
	     MEMRIGHT, // `]'
	     RET,
	     INDENT,
	     IMM, // immediate value (address, etc)
	     SYMBOL, // external symbol, e.g. <malloc>
};

extern int lineno;

#endif

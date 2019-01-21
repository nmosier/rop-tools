#ifndef __TOKENS_H
#define __TOKENS_H

enum tokens {
	     ERROR = -1,
	     DEF = 1, // `:='
	     ARGSEP, // `,'
	     REG,
	     IDENTIFIER,
	     MEMLEFT, // `['
	     MEMRIGHT, // `]'
	     RET,
	     DQ,
	     RESQ,
	     INDENT,
	     INT, // integer constant (address, etc)
	     IMM64, // `imm64'
	     SYMBOL, // external symbol, e.g. <malloc>
	     PLUS, // `+'
	     MINUS, // `-'
};

extern int lineno;

#endif

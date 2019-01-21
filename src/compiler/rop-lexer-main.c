#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "tokens.h"

int lineno = 1; // used by lexer

extern int yylex();
extern char *yytext;
extern int yyleng;
extern FILE *yyin;


int lex_file(FILE *infile, FILE *outfile);
const char *lex_tok2str(int token);
const char *lex_tok2desc(int token, const char *yytext);
int lex_dumptok(int lineno, int token, char *strval, FILE *outfile);

int main(int argc, char *argv[]) {
  FILE *infiles[argc];
  int infiles_cnt = 0;
  int exit_status = 0;
  FILE *outfile = stdout;

  /* get options */
  const char *optstring = "o:h";
  int optchar;
  while ((optchar = getopt(argc, argv, optstring)) >= 0) {
    switch (optchar) {
    case 'o':
      if ((outfile = fopen(optarg, "w")) == NULL) {
	perror("fopen");
	exit(1);
      }
      break;
    case 'h':
      fprintf(stderr, "usage: %s [-o outfile] [infile...]\n", argv[0]);
      exit(0);
    case '?':
    default:
      exit(1);
    }
  }

  if (argc == 1) {
    /* lex standard input */
    infiles_cnt = 1;
    infiles[0] = stdin;
  } else {
    /* lex all files passed as arguments */
    int i;
    for (i = 0; optind < argc; ++optind, ++i) {
      FILE *infile;

      if ((infile = fopen(argv[optind], "r")) == NULL) {
	perror("fopen");
	fprintf(stderr, "%s: fatal error; exiting.\n", argv[0]);
	exit_status = 1;
	goto cleanup;
      }
      infiles[i] = infile;
    }
    infiles_cnt = i;
  }

  /* lex files */
  for (int i = 0; i < infiles_cnt; ++i) {
    if (lex_file(infiles[i], outfile) < 0) {
      perror("lex_file");
      goto cleanup;
    }
  }

 cleanup:
  for (int i = 0; i < infiles_cnt; ++i) {
    if (fclose(infiles[i]) < 0) {
      perror("fclose");
      if (exit_status == 0) {
	exit_status = 2;
      }
    }
  }

  exit(exit_status);
}


int lex_file(FILE *infile, FILE *outfile) {
  /* reset line number */
  lineno = 1;

  /* set up flex with input file */
  yyin = infile;
  
  /* scan and print all tokens */
  int token;
  while ((token = yylex()) != 0) {
    if (lex_dumptok(lineno, token, yytext, outfile) < 0) {
      return -1;
    }
  }

  return 0;
}

int lex_dumptok(int lineno, int token, char *yytext, FILE *outfile) {
  const char *tokstr, *tokval;

  if ((tokstr = lex_tok2str(token)) == NULL) {
    return -1;
  }
  tokval = lex_tok2desc(token, yytext);
  fprintf(outfile, "%d\t%s\t%s\n", lineno, tokstr, tokval ? tokval : "");
  return 0;
}

const char *lex_tok2str(int token) {
  switch (token) {
  case DEF: return "DEF";
  case ARGSEP: return "ARGSEP";
  case REG: return "REG";
  case IDENTIFIER: return "IDENTIFIER";
  case MEMLEFT: return "MEMLEFT";
  case MEMRIGHT: return "MEMRIGHT";
  case RET: return "RET";
  case DQ: return "DQ";
  case RESQ: return "RESQ";
  case INDENT: return "INDENT";
  case INT: return "INT";
  case IMM64: return "IMM64";
  case SYMBOL: return "SYMBOL";
  case PLUS: return "PLUS";
  case MINUS: return "MINUS";
  case NEWLINE: return "NEWLINE";
  default: return NULL;
  }
}

const char *lex_tok2desc(int token, const char *yytext) {
  switch (token) {
  case DEF:
  case REG:
  case IDENTIFIER:
  case INT:
  case SYMBOL:
    return yytext;
  default:
    return NULL;
  }
}

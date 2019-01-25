#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "ast.h"
#include "rop.tab.h"
#include "symtab.h"
#include "semant.h"

extern FILE *yyin;

//struct rules rop_rules;
struct program rop_program;
struct symtab rop_symtab;

int main(int argc, char *argv[]) {
  FILE *infiles[argc];
  int infiles_cnt = 0;
  int exit_status = 0;
  FILE *outfile = stdout;
  extern int yydebug; yydebug = 0;

  /* get options */
  const char *optstring = "o:hd";
  int optchar;
  while ((optchar = getopt(argc, argv, optstring)) >= 0) {
    switch (optchar) {
    case 'o':
      if ((outfile = fopen(optarg, "w")) == NULL) {
	perror("fopen");
	exit(1);
      }
      break;
    case 'd':
      yydebug = 1;
      break;
    case 'h':
      fprintf(stderr, "usage: %s [-d] [-o outfile] [infile...]\n", argv[0]);
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

  /* initialize */
  program_init(&rop_program);
  symtab_init(&rop_symtab);

  /* parse files */
  for (int i = 0; i < infiles_cnt; ++i) {
    yyin = infiles[i];
    yyparse();
  }

  /* debugging: print out symtab */
  symtab_print(&rop_symtab, stderr);

  /* semantic analysis */
  if (semant_check(&rop_program, &rop_symtab) < 0) {
    fprintf(stderr, "%s: semantic analyzer detected errors.\n", argv[0]);
    goto cleanup;
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

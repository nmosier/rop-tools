#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "ast.h"
#include "rop.tab.h"
#include "symtab.h"
#include "semant.h"
#include "cgen.h"
#include "util.h"

extern FILE *yyin;
extern int yydebug;
int lineno;
const char *filename;

struct program rop_program;
struct symtab rop_symtab;

int debug = 0;

int main(int argc, char *argv[]) {
  FILE *infiles[argc];
  char *innames[argc];
  int infiles_cnt = 0;
  int exit_status = 0;
  FILE *outfile = stdout;
  uint64_t origin = 0;
  uint64_t padding = 0;
  uint64_t padding_val = 0;

  /* get options */
  const char *optstring = "o:hdb:p:";
  int optchar;
  char *endptr;
  char *name;
  while ((optchar = getopt(argc, argv, optstring)) >= 0) {
    switch (optchar) {
    case 'o':
      if ((outfile = fopen(optarg, "w")) == NULL) {
	perror("fopen");
	exit(1);
      }
      break;

    case 'b':
      origin = strtoul(optarg, &endptr, 16);
      if (endptr[0] != '\0') {
 	fprintf(stderr, "%s: -b: invalid origin: must be nonnegative"	\
		" 64-bit hexadecimal integer.\n", argv[0]);
	exit(1);
      }
      break;

    case 'p':
      padding = strtoul(optarg, &endptr, 16);
      name = "padding";
      if (endptr[0] == ',') {
	char *padding_val_str = endptr + 1;
	padding_val = strtoul(padding_val_str, &endptr, 16);
	name = "padding value";
      }
      if (endptr[0] != '\0') {
	fprintf(stderr, "%s: -b: invalid %s: must be nonnegative " \
		"64-bit hexadecimal integer.\n", argv[0], name);
	exit(1);
      }
      break;

    case 'd':
      debug = 1;
      break;

    case 'h':
      fprintf(stderr, "usage: %s [-d] [-b origin_addr] [-p padding[,padding_val]] [-o outfile] " \
	      "[infile...]\n", argv[0]);
      exit(0);

    case '?':
    default:
      exit(1);
    }
  }

  /* more argument checking */
  struct {
    uint64_t val;
    const char *name;
  } directives[] = { {origin, "origin"}, {padding, "padding"} };
  for (int i = 0; i < ARRLEN(directives); ++i) {
    if (directives[i].val == 0) {
      fprintf(stderr, "%s: warning: using %s of %lx.\n", argv[0], directives[i].name,
	      directives[i].val);
    }
    if (directives[i].val % 8) {
      fprintf(stderr, "%s: warning: %s is not aligned on qword (8-byte) " \
	      "boundary.\n", argv[0], directives[i].name);
    }
  }
  
  
  yydebug = debug;
  
  /* print debug info */
  if (debug) {
    fprintf(stderr, "%s: debug: using origin of %lx.\n", argv[0], origin);
  }

  if (argc == 1) {
    /* lex standard input */
    infiles_cnt = 1;
    infiles[0] = stdin;
    innames[0] = "<stdin>";
  } else {
    /* lex all files passed as arguments */
    int i;

    memcpy(innames, argv + optind, sizeof(*innames)*(argc - optind));
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
    lineno = 1;
    filename = innames[i];
    yyparse();
  }

  /* debugging: print out symtab */
  symtab_print(&rop_symtab, stderr);

  /* semantic analysis */
  if (semant(&rop_program, &rop_symtab) < 0) {
    fprintf(stderr, "%s: semantic analyzer detected erorrs.\n", argv[0]);
    goto cleanup;
  }

  /* code generation */
  codegen(&rop_program, &rop_symtab, origin, padding, padding_val, outfile);
  
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

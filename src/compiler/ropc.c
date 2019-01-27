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
#include "ropc.h"

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
  const char *anchor_sym = NULL;
  uint64_t anchor_addr, libc_base_addr = 0;
  const char *libc_sym_path = "libc.syms";
  FILE *libc_sym_file = NULL;
  struct libc_syms libc_syms;


  /* usage */
  const char *usage =
    "usage: %s [-d] [-b origin_addr] [-p padding[,padding_val]] "	\
    "        [-a anchor_symbol,anchor_addr] [-s syms_path] [-o outfile] " \
    "        [infile...]\n"							\
    "Compile ROP gadgets to shellcode.\n"				\
    "The options are:\n"						\
    "  -d                     Print debug information\n"		\
    "  -b <org>               Set origin address, i.e. the injection point\n" \
    "  -p <padsz>[,<padval>]  Set padding size (in bytes) before current frame's\n" \
    "                           return address, optionally followed by quad-word\n" \
    "                           padding value\n"			\
    "  -a <sym>,<addr>        Anchor libc symbol followed by address\n"	\
    "  -s <file>              Path to nm(1) dump of libc symbols\n"	\
    "  -o <file>              Place compiled shellcode in file\n";
    
  
  
  /* get options */
  const char *optstring = "o:hdb:p:a:s:";
  int optchar;
  char *endptr;
  char *name;
  while ((optchar = getopt(argc, argv, optstring)) >= 0) {
    switch (optchar) {
    case 'o': // output file
      if ((outfile = fopen(optarg, "w")) == NULL) {
	perror("fopen");
	exit(1);
      }
      break;

    case 'b': // origin
      origin = strtoul(optarg, &endptr, 16);
      if (endptr[0] != '\0') {
 	fprintf(stderr, "%s: -b: invalid origin: must be nonnegative"	\
		" 64-bit hexadecimal integer.\n", argv[0]);
	exit(1);
      }
      break;

    case 'p': // padding information
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

    case 'a': // anchor information 
      if ((anchor_sym = strsep(&optarg, ",")) == NULL) {
	fprintf(stderr, "%s: -a: anchor symbol required as argument.\n", argv[0]);
	exit(1);
      }
      if (optarg == NULL || optarg[0] == '\0') {
	fprintf(stderr, "%s: -a: anchor address required as argument.\n", argv[0]);
	exit(1);
      }
      anchor_addr = strtoul(optarg, &endptr, 16);
      if (endptr[0] != '\0') {
	fprintf(stderr, "%s: -a: invalid anchor address `%s'.\n", argv[0], optarg);
	exit(1);
      }
      break;

    case 's': // path to symbol table
      libc_sym_path = optarg;
      break;

    case 'd': // debug flag
      debug = 1;
      break;

    case 'h': // help
      fprintf(stderr, usage, argv[0]);
      exit(0);

    case '?': // unknown
    default:
      exit(1);
    }
  }

  /* open libc symtab file and initialize libc symbol table */
  if ((libc_sym_file = fopen(libc_sym_path, "r")) == NULL) {
    perror("fopen");
    exit_status = 3;
    goto cleanup;
  }
  if (libc_syms_init(&libc_syms, libc_sym_file) < 0) {
    perror("libc_syms_init");
    exit_status = 4;
    goto cleanup;
  }

  /* resolve base address (if given) */
  if (anchor_sym) {
    uint64_t libc_sym_off;

    if ((libc_sym_off = libc_syms_getaddr(anchor_sym, &libc_syms))
	== (uint64_t) -1) {
      fprintf(stderr, "%s: -a: anchor symbol `%s' could not be resolved.\n",
	      argv[0], anchor_sym);
      goto cleanup;
    }

    /* set libc base address */
    libc_base_addr = anchor_addr - libc_sym_off;
    if (debug) {
      fprintf(stderr, "%s: debug: using libc base address of 0x%lx.\n", argv[0],
	      libc_base_addr);
    }
  }
  
  /* parameter address/offset checking:
   *   - warning if address/offset not properly aligned
   *   - warning if address/offset is zero */
  struct {
    int flags;
    uint64_t val;
    const char *name;
    int align;
  } directives[] = { {ROPC_ADDR_CHK_ALIGN|ROPC_ADDR_CHK_POS, origin, "origin",
		      QWORD_SIZE},
		     {ROPC_ADDR_CHK_ALIGN|ROPC_ADDR_CHK_POS, padding, "padding",
		      QWORD_SIZE},
		     {ROPC_ADDR_CHK_POS, libc_base_addr, "libc base address",
		      -1},
  };
  for (int i = 0; i < ARRLEN(directives); ++i) {
    if ((directives[i].flags & ROPC_ADDR_CHK_POS) && directives[i].val == 0) {
      fprintf(stderr, "%s: warning: using %s of %lx.\n", argv[0], directives[i].name,
	      directives[i].val);
    }
    if ((directives[i].flags & ROPC_ADDR_CHK_ALIGN) && directives[i].val %
	directives[i].align) {
      fprintf(stderr, "%s: warning: %s is not aligned on %d-byte "	\
	      "boundary.\n", argv[0], directives[i].name, directives[i].align);
    }
  }


  
  /* print debug info */
  if (debug) {
    fprintf(stderr, "%s: debug: using origin of %lx.\n", argv[0], origin);
  }


  if (optind == argc) {
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
  codegen(&rop_program, &rop_symtab, &libc_syms, origin, padding, padding_val,
	  outfile);
  
 cleanup:
  for (int i = 0; i < infiles_cnt; ++i) {
    if (fclose(infiles[i]) < 0) {
      perror("fclose");
      if (exit_status == 0) {
	exit_status = 2;
      }
    }
  }
  if (libc_sym_file) {
    if (fclose(libc_sym_file) < 0) {
      if (exit_status == 0) {
	exit_status = 2;
      }
    }
  }

  exit(exit_status);

}

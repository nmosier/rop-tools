#include <stdlib.h>

#include "ast.h"

#define MAX(i1, i2) ((i1) < (i2) ? (i2) : (i1))

#define ARR_MINLEN 16

/* arguments functions */
void arguments_init(struct arguments *args) {
  memset(args, 0, sizeof(*args));
}

int arguments_add(struct argument *arg, struct arguments *args) {
  if (args->argc == args->maxc) {
    /* resize */
    struct argument *argv;
    if ((argv = realloc(args->argv,
			MAX(args->maxc*2, ARR_MINLEN) * sizeof(*args->argv)))
	== NULL) {
      return -1;
    }
    args->argv = argv;
    args->maxc *= 2;
  }
  memcpy(&args->argv[argc->argc++], arg, sizeof(*arg));
  return 0;
}

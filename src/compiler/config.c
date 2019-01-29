/* config.c
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"

int config_origin(const char *s, uint64_t *origin) {
  uint64_t origin_tmp;
  char *endptr;
  
  origin_tmp = strtoul(s, &endptr, 16);
  if (endptr[0] != '\0') {
    fprintf(stderr, "ropc: -b: invalid origin: must be nonnegative"	\
	    " 64-bit hexadecimal integer.\n");
    return -1;
  }
  *origin = origin_tmp;
  return 0;
}

int config_padding(const char *s, uint64_t *padding, uint64_t *padding_val) {
  uint64_t padding_tmp, padding_val_tmp;
  char *endptr;
  const char *name;
  
  padding_tmp = strtoul(s, &endptr, 16);
  name = "padding";
  if (endptr[0] == ',') {
    char *padding_val_str = endptr + 1;
    padding_val_tmp = strtoul(padding_val_str, &endptr, 16);
    name = "padding value";
  }
  if (endptr[0] != '\0') {
    fprintf(stderr, "ropc: -b: invalid %s: must be nonnegative "	\
	    "64-bit hexadecimal integer.\n", name);
    return -1;
  }

  *padding = padding_tmp;
  *padding_val = padding_val_tmp;
  return 0;
}

int config_anchor(char *s, const char **anchor_sym, uint64_t *anchor_addr) {
  char *endptr;
  char *anchor_sym_tmp, *anchor_sym_dup;
  uint64_t anchor_addr_tmp;
  
  if ((anchor_sym_tmp = strsep(&s, ",")) == NULL) {
    fprintf(stderr, "ropc: -a: anchor symbol required as argument.\n");
    return -1;
  }
  if (s == NULL || s[0] == '\0') {
    fprintf(stderr, "ropc: -a: anchor address required as argument.\n");
    return -1;
  }
  anchor_addr_tmp = strtoul(s, &endptr, 16);
  if (endptr[0] != '\0') {
    fprintf(stderr, "ropc: -a: invalid anchor address `%s'.\n", s);
    return -1;
  }

  if ((anchor_sym_dup = strdup(anchor_sym_tmp)) == NULL) {
    perror("ropc: strdup");
    return -1;
  }
  
  *anchor_sym = anchor_sym_dup;
  *anchor_addr = anchor_addr_tmp;
  return 0;
}


int config_libc_syms(const char *s, const char **path) {
  char *sdup;

  if ((sdup = strdup(s)) == NULL) {
    perror("ropc: strdup");
    return -1;
  }
  
  *path = sdup;
  return 0;
}

#define DIRECTIVE_MAXLEN 16
#define DIRECTIVE_LINELEN 128

int config_file(const char *path, struct config_params *params) {
  FILE *configf;
  char *delim = " \t\n";
  char line[DIRECTIVE_LINELEN];
  int retv;

  /* init */
  retv = -1;
  configf = NULL;

  /* open config file */
  if ((configf = fopen(path, "r")) == NULL) {
    fprintf(stderr, "ropc: fopen(%s): %s\n", path, strerror(errno));
    goto cleanup;
  }

  /* parse config file  */
  while (fgets(line, DIRECTIVE_LINELEN, configf)) {
    char *linep = line, *directive, *tok;
    
    if ((directive = strsep(&linep, delim)) == NULL) {
      fprintf(stderr, "ropc: config file parse error: unexpected whitespace.\n");
      goto cleanup;
    }

    if (strcmp(directive, ".origin") == 0) {
      if ((tok = strsep(&linep, delim)) == NULL) {
	fprintf(stderr, "ropc: config file parse error: expected origin address" \
		".\n");
	goto cleanup;
      }
      if (config_origin(tok, params->origin) < 0) {
	goto cleanup;
      }
    } else if (strcmp(directive, ".padding") == 0) {
      if ((tok = strsep(&linep, delim)) == NULL) {
	fprintf(stderr, "ropc: config file parse error: expected padding size.\n");
	goto cleanup;
      }
      if (config_padding(tok, params->padding, params->padding_val) < 0) {
	goto cleanup;
      }
    } else if (strcmp(directive, ".anchor") == 0) {
      if ((tok = strsep(&linep, delim)) == NULL) {
	fprintf(stderr, "ropc: config file parse error: expected anchor symbol "\
		"and address.\n");
	goto cleanup;
      }
      if (config_anchor(tok, params->anchor_sym, params->anchor_addr) < 0) {
	goto cleanup;
      }
    } else if (strcmp(directive, ".symbols") == 0) {
      if ((tok = strsep(&linep, delim)) == NULL) {
	fprintf(stderr, "ropc: config file parse error: expected path to libc "	\
		"symbol dump.\n");
	goto cleanup;
      }
      if (config_libc_syms(tok, params->libc_syms_path) < 0) {
	goto cleanup;
      }
    } else {
      /* not recognized */
      fprintf(stderr, "ropc: config file parse error: directive `%s' not " \
	      "recognized.\n", directive);
      goto cleanup;
    }
  }

  if (ferror(configf)) {
    perror("ropc: ferror");
    goto cleanup;
  }
  
  retv = 0;

 cleanup:
  if (configf) {
    if (fclose(configf) < 0) {
      perror("fclose");
      retv = -1;
    }
  }
  
  return retv;
}

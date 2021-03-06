/* trie.h
 * Nicholas Mosier 2019
 */

#ifndef __TRIE_H
#define __TRIE_H

#include "ropasm.h"

#define TRIE_ERROR NULL

typedef instr_t trie_val_t;
#define trie_val_eq instr_eq
#define trie_val_delete free
#define trie_val_print instr_print

#define TRIE_PRINT_MAXPREFIX 256

struct trie_node;
typedef struct trie_nodes {
  struct trie_node **arr;
  size_t cnt;
  size_t len;
} trie_nodes_t;

typedef struct trie_node {
  trie_val_t tn_val;
  struct trie_node *tn_parent;
  struct trie_nodes tn_children;
} trie_node_t;

/* user-specific */

typedef trie_node_t *trie_t;

trie_t trie_init();
void trie_delete(trie_t trie);
trie_node_t *trienode_new(trie_val_t *val, trie_node_t *par);
int trie_addval(trie_val_t *vals, size_t cnt, trie_t trie);
int trie_validate(trie_t trie);
int trie_print(trie_t trie, FILE *f, int mode);
int trie_width(trie_t trie);

enum trie_validate_retv { TRIE_VALIDATE_OK, TRIE_VALIDATE_NULLCHILD };

#endif

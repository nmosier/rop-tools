/* trie.c
 * Nicholas Mosier 2019
 */

#include <stdint.h>
#include <stdlib.h>
#include <libelf.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "vec.h"
#include "trie.h"
#include "util.h"

//static uint8_t trie_baseval = 0xc3;
trie_t trie_init() {
  trie_node_t *node;
  
  if ((node = malloc(sizeof(*node))) == NULL) {
    return TRIE_ERROR;
  }
  memset(node, 0, sizeof(*node));
  VECTOR_INIT(&node->tn_children);

  //assert (trie_validate(node) == TRIE_VALIDATE_OK);
  
  return node;
}

trie_node_t *trienode_new(trie_val_t *val, trie_node_t *par) {
  trie_node_t *node;

  if ((node = malloc(sizeof(*node)))) {
    memset(node, 0, sizeof(*node));
    VECTOR_INIT(&node->tn_children);
    memcpy(&node->tn_val, val, sizeof(*val));
    node->tn_parent = par;
  }

  //  assert (trie_validate(node) == TRIE_VALIDATE_OK);

  return node;
}

trie_t trie_addnodeat(trie_val_t *val, trie_node_t *par) {
  trie_node_t *node;

  //  assert (trie_validate(par) == TRIE_VALIDATE_OK);
  
  /* create new node */
  if ((node = trienode_new(val, par)) == NULL) {
    return TRIE_ERROR;
  }

  /* insert into trie */
  if (par) {
    if (VECTOR_INSERT(&node, &par->tn_children) < 0) {
      trie_delete(node);
      return TRIE_ERROR;
    }
  }

  return node;
}

// adds instruction to trie by following buffer.

// vals is simple array (not VECTOR!)
int trie_addval(trie_val_t *vals, size_t cnt, trie_t trie) {
  trie_node_t *node = trie;

  //  assert (trie_validate(trie) == TRIE_VALIDATE_OK);
  
  /* if cnt = 0, nothing to do */
  if (cnt == 0) {
    return 0;
  }

  /* some checks */
  assert (vals + 0);

  size_t child_i, child_cnt;
  trie_node_t **child_it;
  child_cnt = node->tn_children.cnt;
  for (child_i = 0, child_it = node->tn_children.arr; child_i < child_cnt;
       ++child_i, ++child_it) {
    trie_node_t *child = *child_it;
    assert (child);

    /* if leading value and value of child are equal, 
     * recursively add to child trie */
    if (trie_val_eq(&child->tn_val, vals + 0)) {
      int addval_result = trie_addval(vals + 1, cnt - 1, child);
      return addval_result;
    }
  }

  /* no value match with child -- create new child  */
  trie_node_t *newchild;
  if ((newchild = trie_addnodeat(vals + 0, node)) == TRIE_ERROR) {
    return -1;
  }
  /* recurse on child in case of any trailing values  */
  assert (newchild);
  int addval_result = trie_addval(vals + 1, cnt - 1, newchild);
  return addval_result;
}

// returns in for compliance with VECTOR_DELETE
int trie_delete_aux(trie_node_t **nodep);
void trie_delete(trie_t trie) {
  trie_delete_aux(&trie);
}

int trie_delete_aux(trie_node_t **nodep) {
  trie_node_t *node = *nodep;

  if (node != TRIE_ERROR) {
    VECTOR_DELETE(&node->tn_children, trie_delete_aux);
    free(node);
  }

  return 0;
}

int trie_print_aux(trie_node_t *node, FILE *f, const trie_val_t **prefix,
		   size_t prefix_cnt, int mode);
int trie_print(trie_t trie, FILE *f, int mode) {
  const trie_val_t *vals[TRIE_PRINT_MAXPREFIX];
  return trie_print_aux(trie, f, vals, 0, mode);
}


int trie_print_aux(trie_node_t *node, FILE *f, const trie_val_t **prefix,
		   size_t prefix_cnt, int mode) {
  /* base case: no children. */
  if (node->tn_children.cnt == 0 || prefix_cnt == TRIE_PRINT_MAXPREFIX) {
    /* print address */
    fprintf(f, "0x%lx:\n", node->tn_val.mcoff);
    
    /* print self */
    trie_val_print(&node->tn_val, f, mode);
    fprintf(f, "\n");

    /* print prefix 
     * note: don't print the first prefix value, since that's the `null' node */
    for (ssize_t i = prefix_cnt - 1; i >= 1; --i) {
      if (prefix[i]) {
	trie_val_print(prefix[i], f, mode);
	fprintf(f, "\n");
      }
    }
    fprintf(f, "\n");
  } else {
    /* print children nodes */
    size_t children_cnt = node->tn_children.cnt;
    prefix[prefix_cnt] = &node->tn_val;
    for (size_t i = 0; i < children_cnt; ++i) {
      trie_node_t *child = node->tn_children.arr[i];
      trie_print_aux(child, f, prefix, prefix_cnt + 1, mode);
    }
  }
  
  return 0;
}


int trie_validate(trie_t trie) {
  int child_status;
  size_t child_cnt = trie->tn_children.cnt;
  trie_node_t **children = trie->tn_children.arr;
  for (size_t child_i = 0; child_i < child_cnt; ++child_i) {
    if (children[child_i] == NULL) {
      return TRIE_VALIDATE_NULLCHILD;
    }
    if ((child_status = trie_validate(children[child_i])) != TRIE_VALIDATE_OK) {
      return child_status;
    }
  }

  return TRIE_VALIDATE_OK;
}

/* i.e. # of leaves */
int trie_width(trie_t trie) {
  if (trie->tn_children.cnt == 0) {
    return 1;
  }
  
  int acc = 0;
  for (size_t i = 0; i < trie->tn_children.cnt; ++i) {
    acc += trie_width(trie->tn_children.arr[i]);
  }
  return acc;
}

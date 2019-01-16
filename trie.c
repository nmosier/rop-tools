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

  return node;
}

trie_node_t *trienode_new(trie_val_t *val) {
  trie_node_t *node;

  if ((node = malloc(sizeof(*node)))) {
    memset(node, 0, sizeof(*node));
    VECTOR_INIT(&node->tn_children);
    //memcpy(&node->tn_val, val, sizeof(*val)); // trie owns data
    node->tn_val = val;
  }

  return node;
}

trie_t trie_addnodeat(trie_val_t *val, trie_node_t *par) {
  trie_node_t *node;

  /* create new node */
  if ((node = trienode_new(val)) == NULL) {
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
//int trie_addinstr_aux(uint8_t *instr, size_t len, Elf64_Off off,
//		      trie_node_t *curnode);
//int trie_addinstr(uint8_t *instr, size_t len, Elf64_Off off, trie_t trie) {
//  return trie_addinstr_aux(instr, len, off, trie);
//}

// vals is simple array (not VECTOR!)
int trie_addval(trie_val_t **vals, size_t cnt, trie_t trie) {
  trie_node_t *node = trie;
  
  /* if cnt = 0, nothing to do */
  if (cnt == 0) {
    return 0;
  }

  size_t child_i, child_cnt;
  trie_node_t **child_it;
  child_cnt = node->tn_children.cnt;
  for (child_i = 0, child_it = node->tn_children.arr; child_i < child_cnt;
       ++child_i, ++child_it) {
    trie_node_t *child = *child_it;

    /* if leading value and value of child are equal, 
     * recursively add to child trie */
    if (trie_val_eq(child->tn_val, vals[0])) {
      return trie_addval(vals + 1, cnt - 1, child);
    }
  }

  /* no value match with child -- create new child  */
  trie_node_t *newchild;
  if ((newchild = trie_addnodeat(vals[0], node)) == TRIE_ERROR) {
    return -1;
  }
  /* recurse on child in case of any trailing values  */
  return trie_addval(vals + 1, cnt - 1, newchild);
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
    trie_val_delete(node->tn_val);
    free(node);
  }

  return 0;
}

int trie_print_aux(trie_node_t *node, FILE *f, const trie_val_t **prefix,
		   size_t prefix_cnt);
int trie_print(trie_t trie, FILE *f) {
  const trie_val_t *vals[TRIE_PRINT_MAXPREFIX];
  return trie_print_aux(trie, f, vals, 0);
}


int trie_print_aux(trie_node_t *node, FILE *f, const trie_val_t **prefix,
		   size_t prefix_cnt) {
  /* print current node */
  
  /* print prefix of node */
  for (size_t i = 0; i < prefix_cnt; ++i) {
    trie_val_print(prefix[i], f, INSTR_PRINT_DISASM);
    fprintf(f, "\t");
  }
  /* pritn node value */
  trie_val_print(node->tn_val, f, INSTR_PRINT_DISASM);
  fprintf(f, "\n");

  /* print children nodes */
  size_t children_cnt = node->tn_children.cnt;
  if (prefix_cnt == TRIE_PRINT_MAXPREFIX) {
    return -1;
  }
  prefix[prefix_cnt] = node->tn_val;
  for (size_t i = 0; i < children_cnt; ++i) {
    trie_node_t *child = node->tn_children.arr[i];
    trie_print_aux(child, f, prefix, prefix_cnt + 1);
  }

  return 0;
}

/*
int trie_print_aux(trie_node_t *node, FILE *f, const uint8_t *prefix,
		    size_t prefix_len) {
  uint8_t *curinstr; // current instruction
  size_t curinstr_len;

  curinstr_len = prefix_len + node->tn_val.tv_len;
  if ((curinstr = malloc(curinstr_len)) == NULL) {
    return -1;
  }
  memcpy(curinstr, prefix, prefix_len);
  memcpy(curinstr + prefix_len, node->tn_val.tv_buf, node->tn_val.tv_len);
  
  /* print self */
/*  for (size_t i = 0; i < curinstr_len; ++i) {
    fprintf(f, "0x%2.2hx ", curinstr[i]);
  }
  fprintf(f, "\n");

  /* print children */
/*  for (size_t i = 0; i < node->tn_children.cnt; ++i) {
    if (trie_print_aux(node->tn_children.arr[i], f, curinstr, curinstr_len) < 0) {
      free(curinstr);
      return -1;
    }
  }

  free(curinstr);
  return 0;
}
*/

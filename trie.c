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

trie_t trie_addnodeat(uint8_t *instr, size_t len, Elf64_Off off,
		   trie_node_t *par) {
  trie_node_t *node;
  int retv;

  /* init */
  node = NULL;
  retv = -1;

  /* alloc & failsafe-init new node */
  if ((node = malloc(sizeof(*node))) == NULL) {
    goto cleanup;
  }
  memset(node, 0, sizeof(*node));
  VECTOR_INIT(&node->tn_children);
  if ((node->tn_val.tv_buf = malloc(len)) == NULL) {
    goto cleanup;
  }

  /* init new node */
  memcpy(node->tn_val.tv_buf, instr, len);
  node->tn_val.tv_len = len;
  node->tn_val.tv_off = off;
  node->tn_parent = par;

  /* add new node as child to _par_ (if non-null) */
  if (par) {
    if (VECTOR_INSERT(&node, &par->tn_children) < 0) {
      goto cleanup;
    }
  }


  /* success */
  retv = 0;
  
 cleanup:
  if (retv < 0) {
    if (node) {
      free(node->tn_val.tv_buf);
      VECTOR_DELETE(&node->tn_children, NULL); // note node will have no children
      free(node);
    }
  }

  if (retv < 0) {
    return NULL;
  }
  return par ? par : node;
}

// adds instruction to trie by following buffer.
int trie_addinstr_aux(uint8_t *instr, size_t len, Elf64_Off off,
		      trie_node_t *curnode);
int trie_addinstr(uint8_t *instr, size_t len, Elf64_Off off, trie_t trie) {
  return trie_addinstr_aux(instr, len, off, trie);
}

int trie_addinstr_aux(uint8_t *instr, size_t len, Elf64_Off off,
			       trie_node_t *curnode) {
  size_t curlen;
  size_t newlen;
  uint8_t *newinstr;

  curlen = curnode->tn_val.tv_len;
  newlen = len - curlen;
  newinstr = instr + curlen;

  /* check if instruction already matches current node */
  assert (len >= curlen && memcmp(instr, curnode->tn_val.tv_buf, curlen) == 0);
  if (len == curlen) {
    return 0;
  }
  
  /* recursive case */
  size_t nchildren = curnode->tn_children.cnt;
  size_t i;
  
  /* strategy: match prefix of instruction bytes with child's */
  for (i = 0; i < nchildren; ++i) {
    trie_node_t *child;
    size_t childlen;
    
    child = curnode->tn_children.arr[i];
    childlen = child->tn_val.tv_len;
    if (memcmp(newinstr, child->tn_val.tv_buf, MIN(childlen, newlen)) == 0) {
      /* found match */
      break;
    }
  }
  
  if (i == nchildren) {
    /* child match not found; add new child */
    if (trie_addnodeat(newinstr, newlen, off, curnode) == NULL) {
      return -1;
    }
    return 0;
  }

  /* child match found -- recursive call */
     
  return trie_addinstr_aux(newinstr, newlen, off, curnode->tn_children.arr[i]);
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
    free(node->tn_val.tv_buf);
    free(node);
  }

  return -0;
}

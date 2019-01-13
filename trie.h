/* trie.h
 * Nicholas Mosier 2019
 */

#ifndef __TRIE_H
#define __TRIE_H

#define TRIE_ERROR NULL

typedef struct trie_val {
  uint8_t *tv_buf;
  size_t tv_len;
  Elf64_Off tv_off;
} trie_val_t;

struct trie_node;
typedef struct trie_nodes {
  struct trie_node **arr;
  size_t cnt;
  size_t len;
} trie_nodes_t;

typedef struct trie_node {
  struct trie_val tn_val;
  struct trie_node *tn_parent;
  struct trie_nodes tn_children;
} trie_node_t;

/* user-specific */

typedef trie_node_t *trie_t;

trie_t trie_init();
void trie_delete(trie_t trie);
trie_t trie_addnodeat(uint8_t *instr, size_t len, Elf64_Off off,
		      trie_node_t *par);
int trie_addinstr(uint8_t *instr, size_t len, Elf64_Off off, trie_t trie);

#endif

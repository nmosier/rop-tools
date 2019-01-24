/* symtab.h
 * Nicholas Mosier 2019
 */

#ifndef __SYMTAB_H
#define __SYMTAB_H

#include "ast.h"

#define SYMTAB_ENTC 256

/* symbol table */
enum symbol_kind {
  SYMBOL_DEFINITION,
  SYMBOL_EQUATE,
  SYMBOL_EXTERN,
  SYMBOL_UNKNOWN,
};
enum symbol_kind rulek2symk(enum rule_kind rulek);
const char *symk2str(enum symbol_kind symk);

struct symbol {
  char *name;
  enum symbol_kind kind;
  union {
    struct rule *rule_ptr;
  };
};

struct syment {
  struct symbol sym;
  struct syment *next;
};

struct symtab {
  struct syment **entv;
  int entc;
};

int symtab_init(struct symtab *tab);
int symbol_hash(const struct symbol *sym);
struct symbol *symtab_put(struct symbol *sym, struct symtab *tab);
struct symbol *symtab_put_bare(const char *name, struct symtab *tab);
int symbol_cmp(const struct symbol *sym1, const struct symbol *sym2);
struct syment **symtab_entref(struct symbol *sym, struct symtab *tab);
enum symbol_kind rulek2symk(enum rule_kind rulek);
struct symbol *symtab_get(char *name, struct symtab *tab);
int symtab_print(const struct symtab *tab, FILE *f);
int symbol_print(const struct symbol *sym, FILE *f);

#endif

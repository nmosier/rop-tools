#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ast.h"
#include "rop.tab.h"
#include "symtab.h"
#include "util.h"

int symtab_init(struct symtab *tab) {
  struct syment **entv;
  
  /* allocate hash table */
  if ((entv = calloc(SYMTAB_ENTC, sizeof(*entv))) == NULL) {
    return -1;
  }

  /* set members of symbol table */
  tab->entv = entv;
  tab->entc = SYMTAB_ENTC;

  return 0;
}

int symbol_hash(const struct symbol *sym) {
  int acc;
  const char *it;

  acc = 0;
  for (it = sym->name; *it; ++it) {
    acc = acc * 17 + *it;
  }

  return acc;
}

int symbol_cmp(const struct symbol *sym1, const struct symbol *sym2) {
  return strcmp(sym1->name, sym2->name);
}

/* returns 1 if entry inserted, 0 if entry already exists, -1 if error */
/* symtab_ent: returns pointer to place in table that points to symbol */
struct syment **symtab_entref(struct symbol *sym, struct symtab *tab) {
  struct syment *ent;
  struct syment **ref;
  int index;

  index = abs(symbol_hash(sym) % tab->entc);
  ref = &tab->entv[index];
  ent = tab->entv[index];

  /* skip over other entries */
  for (ref = &tab->entv[index], ent = *ref;
       ent && symbol_cmp(&ent->sym, sym);
       ref = &ent->next, ent = *ref)
    {}

  return ref;
}

int symtab_put(struct symbol *sym, struct symtab *tab) {
  struct syment **ref;
  struct syment *ent;
  
  ref = symtab_entref(sym, tab);
  if (*ref) {
    return 0; // already exists in table
  }

  /* construct & insert new entry at _ref_ */
  if ((ent = malloc(sizeof(*ent))) == NULL) {
    return -1; // error
  }
  memcpy(&ent->sym, sym, sizeof(*sym));
  ent->next = NULL;
  *ref = ent;

  return 1; // successfully inserted
}

int symtab_put_bare(const char *name, struct symtab *tab) {
  struct symbol sym;

  if ((sym.name = strdup(name)) == NULL) {
    return -1;
  }
  sym.kind = SYMBOL_UNKNOWN;
  
  return symtab_put(&sym, tab);
}

struct symbol *symtab_get(char *name, struct symtab *tab) {
  struct symbol sym;
  sym.name = name;
  return &(*symtab_entref(&sym, tab))->sym;
}

enum symbol_kind rulek2symk(enum rule_kind rulek) {
  switch (rulek) {
  case RULE_DEFINITION: return SYMBOL_DEFINITION;
  case RULE_EQUATE: return SYMBOL_EQUATE;
  default: return (enum symbol_kind) -1;
  }
}

const char *symk2str(enum symbol_kind symk) {
  switch (symk) {
  case SYMBOL_DEFINITION: return "DEF";
  case SYMBOL_EQUATE: return "EQE";
  case SYMBOL_EXTERN: return "EXT";
  case SYMBOL_UNKNOWN: return "UNK";
  default: return NULL;
  }
}


int symbol_print(const struct symbol *sym, FILE *f) {
  return fprintf(f, "%s\t%s", sym->name, symk2str(sym->kind));
}

int symtab_print(const struct symtab *tab, FILE *f) {
  struct syment **entit, **entend, *listit;
  int retv;

  retv = 0;
  for (entit = tab->entv, entend = tab->entv + tab->entc;
       entit < entend; ++entit) {
    if (*entit) {
      for (listit = *entit; listit; listit = listit->next) {
	if (symbol_print(&listit->sym, f) < 0) {
	  retv = -1;
	}
	fprintf(f, "\n");
      }
    }
  }

  return retv;
}

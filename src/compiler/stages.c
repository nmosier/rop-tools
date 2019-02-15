/* stages.c -- functions specific to single- and multi-stage exploits
 * Nicholas Mosier 2019
 */

/* semant_install_stage: installs stage-specific symbols in the symbol table
 * Note: the values of these symbols are undefined until midway through code
 * generation  and should NOT be used by the programmer.
 * 
 * Symbols installed:
 *   PADDING -- padding value
 *   PAYLOAD_LENGTH -- length of payload (only if two-stage, i.e. stages == 2)
 *   PAYLOAD_ADDR -- base address to map payload to (only if two-stage)
 *   PAYLOAD_FD -- file descriptor of payload (only if two-stage)
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "stages.h"
#include "util.h"

int semant_install_stagesyms(struct symtab *tab, int stages) {
  struct symbol *sym;
  
  if ((sym = symtab_put_bare("PADDING", tab)) == NULL) {
    return -1;
  }
  sym->kind = SYMBOL_CONST;
  
  if (stages == 2) {
    if ((sym = symtab_put_bare("PAYLOAD_LENGTH", tab)) == NULL) {
      return -1;
    }
    sym->kind = SYMBOL_CONST;
    
    if ((sym = symtab_put_bare("PAYLOAD_ADDR", tab)) == NULL) {
      return -1;
    }
    sym->kind = SYMBOL_CONST;

    if ((sym = symtab_put_bare("PAYLOAD_FD", tab)) == NULL) {
      return -1;
    }
    sym->kind = SYMBOL_CONST;
  }

  return 0;
}

void codegen_pass1_set_stagesyms(struct symtab *tab, int stages, uint64_t padding,
				uint64_t origin) {
  struct symbol *sym;

  if (stages == 2) {
    sym = symtab_get("PADDING", tab);
    assert(sym && sym->kind == SYMBOL_CONST);
    sym->qconst = padding;
    
    sym = symtab_get("PAYLOAD_ADDR", tab);
    assert(sym && sym->kind == SYMBOL_CONST);
    sym->qconst = origin;

    sym = symtab_get("PAYLOAD_FD", tab);
    assert(sym && sym->kind == SYMBOL_CONST);
    sym->qconst = PAYLOAD_FD;
  }
}

void codegen_pass2_set_stagesyms(struct symtab *tab, int stages, uint64_t exprcnt) {
  struct symbol *sym;

  if (stages == 2) {
    sym = symtab_get("PAYLOAD_LENGTH", tab);
    assert(sym && sym->kind == SYMBOL_CONST);
    sym->qconst = roundpage(exprcnt) * QWORD_SIZE;
    fprintf(stderr, "payload length = 0x%zx\n", roundpage(exprcnt) * QWORD_SIZE);
  }
}

/* stages.h
 * Nicholas Mosier 2019
 */

#ifndef __STAGES_H
#define __STAGES_H

#include <stdint.h>
#include "symtab.h"

#define PAYLOAD_FD 0 // stdin


int semant_install_stagesyms(struct symtab *tab, int stages);
void codegen_pass1_set_stagesyms(struct symtab *tab, int stages, uint64_t padding,
				 uint64_t origin);
void codegen_pass2_set_stagesyms(struct symtab *tab, int stages, uint64_t exprcnt);


#endif

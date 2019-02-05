/* ropelf.h
 * Nicholas Mosier 2019
 */

#ifndef __ROPELF_H
#define __ROPELF_H

#include <stdint.h>
#include <stdio.h>
#include <libelf.h>



Elf *ropelf_begin(int fd);
void ropelf_end(Elf *elf);

#endif

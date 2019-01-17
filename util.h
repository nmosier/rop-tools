/* util.h
 * Nicholas Mosier 2019
 */

#ifndef __UTIL_H
#define __UTIL_H

#include <stdio.h>
#include <libelf.h>

#define MAX(i1, i2) ((i1) < (i2) ? (i2) : (i1))
#define MIN(i1, i2) ((i1) < (i2) ? (i1) : (i2))

void pelferror(const char *s);
Elf64_Off phoffset(uint16_t phnum, uint16_t phentsize);

void *memdup(void *ptr, size_t size);

#endif

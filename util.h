/* util.h
 * Nicholas Mosier 2019
 */

#ifndef __UTIL_H
#define __UTIL_H

void pelferror(const char *s);
Elf64_Off phoffset(uint16_t phnum, uint16_t phentsize);

#endif

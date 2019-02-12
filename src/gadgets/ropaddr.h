/* ropaddr.h
 * Nicholas Mosier 2019
 */

#ifndef __ROPADDR_H
#define __ROPADDR_H

struct rop_addrs;

struct rop_addrs *rop_addrs_new(const char *path);
void rop_addrs_delete(struct rop_addrs *addrs);
int rop_addrs_has(uint64_t val, struct rop_addrs *addrs);
 
#endif

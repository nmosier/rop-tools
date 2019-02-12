/* ropaddr.cpp
 * Nicholas Mosier 2019
 */

#include <unordered_set>
#include <fstream>
#include <iostream>
//#include "ropaddr.h"

extern "C" struct rop_addrs: std::unordered_set<uint64_t> {};

extern "C" struct rop_addrs *rop_addrs_new(const char *path) {
  std::ifstream addrs_f;
  
  addrs_f.open(path, std::ifstream::in);
  if (!addrs_f) {
    return NULL;
  }

  struct rop_addrs *addrs = new rop_addrs();
  
  uint64_t val;
  while ((addrs_f >> val)) {
    addrs->insert(val);
  }
  addrs_f.close();
  
  return addrs;
}

extern "C" void rop_addrs_delete(struct rop_addrs *addrs) {
  delete addrs;
}

extern "C" int rop_addrs_has(uint64_t val, struct rop_addrs *addrs) {
  return addrs->find(val) != addrs->end();
}

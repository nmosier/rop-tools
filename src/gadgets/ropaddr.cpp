/* ropaddr.cpp
 * Nicholas Mosier 2019
 */

#include <unordered_set>
#include <fstream>
//#include "ropaddr.h"

extern "C" struct rop_addrs: std::unordered_set<uint64_t> {
  rop_addrs(const char *path);
};

/* initialize set from filepath */
rop_addrs::rop_addrs(const char *path) {
  std::ifstream addrs_f;
  addrs_f.open(path);

  uint64_t val;
  while ((addrs_f >> val)) {
    insert(val);
  }
}

extern "C" struct rop_addrs *rop_addrs_new(const char *path) {
  try {
    return new rop_addrs(path);
  } catch (const std::exception &err) {
    return NULL;
  }
}

extern "C" void rop_addrs_delete(struct rop_addrs *addrs) {
  delete addrs;
}

extern "C" int rop_addrs_has(uint64_t val, struct rop_addrs *addrs) {
  return addrs->find(val) == addrs->end();
}

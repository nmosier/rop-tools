/* util.c
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <string.h>

void *memdup_f(void *ptr, size_t size) {
  void *newptr = malloc(size);
  if (newptr) {
    memcpy(newptr, ptr, size);
  }
  return newptr;
}

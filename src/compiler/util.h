/* util.h
 * Nicholas Mosier 2019
 */

#ifndef __UTIL_H
#define __UTIL_H

#define ARR_MINLEN 16

#define memdup(ptr) ((typeof(ptr)) memdup_f(ptr, sizeof(*ptr)))
void *memdup_f(void *ptr, size_t size);

#define MAX(i1, i2) ((i1) < (i2) ? (i2) : (i1))

#endif

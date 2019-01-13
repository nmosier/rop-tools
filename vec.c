#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "util.h"
#include "vec.h"

/* vector_init()
 * DESC: initialize vector _vec_ of size sizeof(*vec)=_vsize_. */
void vector_init(void *vec, size_t vsize) {
   memset(vec, 0, vsize);
}

/* vector_resize()
 * DESC: resize vector to new length.
 * ARGS:
 *  - newlen: new length (max # of entries)  of vector.
 *  - oldarr: pointer to beginning of vector array member.
 *  - oldcnt: pointer to vector's count member.
 *  - oldlen: pointer to vector's length member.
 *  - esize: size of one element in array.
 * RETV: 0 on success, -1 on error. Updates all relevant vector fields.
 * NOTE: see VECTOR_RESIZE() for macro version.
 */
int vector_resize(size_t newlen, void **oldarr, size_t *oldcnt, size_t *oldlen, size_t esize) {
   void *newarr;

   if ((newarr = reallocarray(*oldarr, newlen, esize)) == NULL) {
      return -1;
   }

   memset(newarr, 0, newlen * esize);

   *oldarr = newarr;
   *oldlen = newlen;
   *oldcnt = MIN(*oldcnt, newlen);

   return 0;
}

/* vector_rem(): return number of remaining unused entries in array */
size_t vector_rem(size_t cnt, size_t len) {
   return len - cnt;
}

/* vector_insert()
 * DESC: inserts element into (end of) vector.
 * ARGS:
 *  - elt: pointer to element to insert, or NULL for uninitialized array slot.
 *  - arr: pointer to vector array (beginning).
 *  - cnt: number of (used) elements in array. 
 *  - len: number of (allocated) elements in array.
 *  - esize: size of one array element.
 * RETV: return index of inserted element on success; -1 on error.
 */
ssize_t vector_insert(void *elt, void **arr, size_t *cnt, size_t *len, size_t esize) {
   void *newelt;

   if (vector_rem(*cnt, *len) == 0) {
      size_t newlen = MAX(VECTOR_MINLEN, *len * 2);
      if (vector_resize(newlen, arr, cnt, len, esize) < 0) {
         return -1;
      }
   }

   newelt = (char *) (*arr) + (esize * (*cnt));

   if (elt) {
      memcpy(newelt, elt, esize);
   }

   /* return index of inserted element & update count */
   return (*cnt)++;
}

// remove at index
/* vector_remove()
 * DESC: remove element at index _i_ of vector array.
 * ARGS:
 *  - i: index of element to remove.
 *  - arr: pointer to beginning of vector's array.
 *  - cnt: pointer to number of elements in array.
 *  - esize: size of one element.
 *  - del: pointer to function to delete element (NULL if no deletion/cleanup necessary).
 *         Must return -1 on error, 0 on success.
 * RETV: return 0 on success, -1 on error.
 */
int vector_remove(size_t i, void **arr, size_t *cnt, size_t esize, int (*del)(void *)) {
   char *elt;
   char *last_elt;
   int retv = 0;

   /* computer ptr to elt */
   elt = (char *) *arr + i * esize;
   
   /* delete element */
   if (del && del(elt) < 0) {
      retv = -1;
   }
   
   /* update number of entries */
   --(*cnt);

   /* fill slot with last entry */
   last_elt = (char *) (*arr) + (esize * (*cnt));
   if (i != *cnt) {
      memcpy(elt, last_elt, esize);
   }

   return retv;
}

/* vector_delete()
 * DESC: delete vector members.
 * ARGS:
 *  - vec: pointer to vector.
 *  - arr: pointer to vector's array.
 *  - cnt: pointer to vector's element count.
 *  - vsize: size of vector type.
 *  - esize: size of one element.
 * RETV: 0 on success, -1 on error.
 */
int vector_delete(void *vec, void **arr, size_t *cnt, size_t vsize, size_t esize,
                   int (*del)(void *)) {
   size_t i;
   char *elt_it;
   int retv, errsav;

   retv = 0;
   
   if (del) {
      for (i = 0, elt_it = (char *) *arr; i < *cnt; ++i, elt_it += esize) {
         if (del(elt_it) < 0) {
            retv = -1;
            errsav = errno;
         }
     }
   }

   fprintf(stderr, "freeing array at %p\n", (const void *) *arr);
   free(*arr);
   memset(vec, 0, vsize);

   if (retv < 0) {
      errno = errsav;
   }
   return retv;
}

/* vec.h
 * Nicholas Mosier 2019
 */

#ifndef __VEC_H
#define __VEC_H

#include <string.h>
#include <stdlib.h>

#include "util.h"

#define VEC_MINLEN 16

#define vector_decl(name, type, abbr)		\
  vector_struct_decl(name, type, abbr);		\
  vector_init_decl(name, type, abbr);		\
  vector_add_decl(name, type, abbr)

#define vector_def(name, type, abbr)		\
  vector_init_def(name, type, abbr)		\
  vector_add_def(name, type, abbr)

#define vector_struct_decl(name, type, abbr)	\
  struct name {					\
    struct type *abbr##v;			\
    int abbr##c;				\
    int maxc;					\
  }

#define vector_init_decl(name, type, abbr)	\
  void name##_init(struct name *abbr##s)
#define vector_init_def(name, type, abbr)		\
  vector_init_decl(name, type, abbr) {			\
    memset(abbr##s, 0, sizeof(*abbr##s));		\
  }

#define vector_add_decl(name, type, abbr)	\
  int name##_add(struct type *abbr, struct name *abbr##s)

#define vector_add_def(name, type, abbr)		\
  vector_add_decl(name, type, abbr) {			\
    if (abbr##s->abbr##c == abbr##s->maxc) {		\
      /* resize*/					\
      struct type *abbr##v;				\
      int newc = MAX(abbr##s->maxc*2, VEC_MINLEN);				\
      if ((abbr##v = realloc(abbr##s->abbr##v, newc * sizeof(*abbr##s->abbr##v))) \
	  == NULL) {							\
	return -1;							\
      }									\
      abbr##s->abbr##v = abbr##v;					\
      abbr##s->maxc = newc;						\
    }									\
    memcpy(&abbr##s->abbr##v[abbr##s->abbr##c++], abbr, sizeof(*abbr)); \
    return 0;								\
  }


#endif

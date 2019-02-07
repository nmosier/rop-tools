/* config.h
 * Nicholas Mosier 2019
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <stdint.h>

#define BASE10 10

struct config_params {
  uint64_t *origin;
  uint64_t *padding;
  uint64_t *padding_val;
  const char **anchor_sym;
  uint64_t *anchor_addr;
  const char **libc_syms_path;
};

int config_origin(const char *s, uint64_t *origin);
int config_padding(const char *s, uint64_t *padding, uint64_t *padding_val);
int config_anchor(char *s, const char **anchor_sym, uint64_t *anchor_addr);
int config_libc_syms(const char *s, const char **path);
int config_stages(const char *str, int *stages);
int config_file(const char *path, struct config_params *params);

#endif

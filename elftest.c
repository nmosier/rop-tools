#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include <libelf.h>
#include <gelf.h>

#include "ropelf.h"
#include "util.h"

#define VERBOSE 1

#define EXIT_FAILED 3 // exit status

#define PHDRS_MAXSIZE 10

int main(int argc, char *argv[]) {
  int fd;
  int exitno;
  const char *path;

  Elf *elf;

  /* initialization */
  fd = -1;
  exitno = EXIT_FAILED;
  
  if (argc != 2) {
    fprintf(stderr, "usage: %s elf_file\n", argv[0]);
    exit(1);
  }
  path = argv[1];


  /* open file to parse */
  if ((fd = open(path, O_RDONLY, 0)) < 0) {
    perror("open");
    exit(3);
  }

  if ((elf = ropelf_begin(fd)) == NULL) {
    goto cleanup;
  }

  rop_banks_t banks;
  if (banks_create(fd, elf, &banks) < 0) {
    perror("banks_create");
    goto cleanup;
  }

  if (VERBOSE) {
    fprintf(stderr, "successfully created %zu banks.\n", banks.len);
  }

  /* dump exec hex */
  bank_hexdump(&banks.arr[0], stdout);
  
  /* success */
  fprintf(stderr, "success!\n");
  exitno = 0;
  
  /* cleanup */
 cleanup:
  ropelf_end(elf);
  if (fd >= 0) {
    if (close(fd) < 0) {
      perror("close");
      exitno = EXIT_FAILED;
    }
  }

  exit(exitno);
}




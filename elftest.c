#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include <libelf.h>

#include "ropelf.h"
#include "util.h"

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

  Elf64_Phdr phdrs[PHDRS_MAXSIZE];
  if (ropelf_getexecphdrs(elf, phdrs, PHDRS_MAXSIZE);

  /* success */
  printf("success!\n");
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




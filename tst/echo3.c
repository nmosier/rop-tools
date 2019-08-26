#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 300

void find_libc(void) {
  printf("sleep @ %p\n", (void *) sleep);
}

void echo(int fd) {
  ssize_t nbytes;
  char buf[8];

  if ((nbytes = read(fd, buf, BUFSIZE)) < 0) {
    perror("read");
    return;
  }
  printf("read %zu bytes\n", nbytes);
  fwrite(buf, 1, nbytes, stdout);
}

int main(int argc, char *argv[]) {
  int fd;
  
  if (argc != 2) {
    printf("usage: %s path\n", argv[0]);
    return 1;
  }
  
  if ((fd = open(argv[1], O_RDWR)) < 0) {
    perror("open");
    return 0;
  }
  echo(fd);
}

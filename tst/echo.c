#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define BUFSIZE 300

void echo() {
  ssize_t nbytes;
  char buf[8];

  if ((nbytes = read(0, buf, BUFSIZE)) < 0) {
    perror("read");
    return;
  }
  printf("read %zu bytes\n", nbytes);
  fwrite(buf, 1, nbytes, stdout);
}

int main(int argc, char *argv[]) {
  echo();
}

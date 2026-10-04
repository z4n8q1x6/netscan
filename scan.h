#ifndef SCAN_H
#define SCAN_H

#include <stdint.h>
#include <unistd.h>

typedef struct {
  int *fds;
  int max_fd;
  size_t count;
} SocketManager;

int scan(const char *host, uint16_t i1, uint16_t i2);

#endif // SCAN_H

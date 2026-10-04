#include "scan.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <bits/types/struct_timeval.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static int create_connect_socket(SocketManager *manager,
                                 struct sockaddr_in addr, int port_number) {
  int count = manager->count;
  manager->fds[count] = socket(AF_INET, SOCK_STREAM, 0);
  if (manager->fds[count] == -1) {
    perror("create_connect_socket: socket");
    return 0;
  }
  if (fcntl(manager->fds[count], F_SETFL, O_NONBLOCK) == -1) {
    perror("create_connect_socket: fcntl");
    manager->fds[count] = -1;
    return 0;
  }
  addr.sin_port = htons(port_number);
  if (connect(manager->fds[count], (const struct sockaddr *)&addr,
              sizeof(addr)) == -1) {
    if (errno != EINPROGRESS) {
      perror("create_connect_socket: connect");
      manager->fds[count] = -1;
      return 0;
    }
  }
  if (manager->fds[count] > manager->max_fd) {
    manager->max_fd = manager->fds[count];
  }
  manager->count++;
  return 1;
}

int scan(const char *host, uint16_t i1, uint16_t i2) {
  int nb_ports = i2 - i1 + 1;
  struct sockaddr_in addr = {.sin_family = AF_INET};
  if (inet_pton(AF_INET, host, &addr.sin_addr) == -1) {
    perror("inet_pton");
    return 0;
  }
  int highest_fd = -1;
  long MAX_FDS =
      sysconf(_SC_OPEN_MAX) - 3; // -3 cuz of stdin, stdout and stderr
  if (MAX_FDS == -1) {
    perror("scan: sysconf");
    return 0;
  }
  if (MAX_FDS > FD_SETSIZE - 3)
    MAX_FDS = FD_SETSIZE - 3;

  SocketManager manager = {.count = 0, .max_fd = -1};
  manager.fds = malloc(nb_ports * sizeof(*manager.fds));
  if (manager.fds == NULL) {
    perror("scan: malloc");
    return 0;
  }
  for (int i = 0; i < nb_ports; i++) {
    manager.fds[i] = -1;
  }

  unsigned int scanned = 0;
  // create sockets and connect sockets
  for (int i = i1; i <= i2 && i < MAX_FDS + i1; i++) {
    create_connect_socket(&manager, addr, i);
    scanned++;
  }

  fd_set wfds;
  int opt;
  socklen_t optlen = sizeof(opt);
  int n = nb_ports;
  // handle connections
  while (n > 0) {
    // reset select in loop
    FD_ZERO(&wfds);
    for (size_t i = 0; i < manager.count; i++) {
      if (manager.fds[i] != -1) {
        FD_SET(manager.fds[i], &wfds);
      }
    }
    struct timeval timeout = {.tv_sec = 5, .tv_usec = 0};
    int retval = select(manager.max_fd + 1, NULL, &wfds, NULL, &timeout);
    if (retval == -1) {
      perror("scan: select");
    } else if (retval) {
      for (size_t i = 0; i < manager.count; i++) {
        if (FD_ISSET(manager.fds[i], &wfds) != 0) {
          if (getsockopt(manager.fds[i], SOL_SOCKET, SO_ERROR, &opt, &optlen) ==
              -1) {
            perror("scan: getsockopt");
          } else {
            if (opt == 0) {
              printf("Connected to port %zu.\n", i1 + i);
            }
            n--;
            close(manager.fds[i]);
            manager.fds[i] = -1;
            if (scanned < nb_ports) {
              create_connect_socket(&manager, addr, i1 + scanned);
              scanned++;
            }
          }
        }
      }
    } else {
      printf("Connection Timeout.\n");
      printf("Scanned %d/%d ports in interval [%d, %d].\n", scanned, nb_ports,
             i1, i2);
      n = -1;
    }
  }
  if (n == 0)
    printf("Scanned %d/%d ports in interval [%d, %d].\n", scanned, nb_ports, i1,
           i2);
  return 1;
}

#include "ping.h"
#include "scan.h"
#include "util.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <bits/getopt_core.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>

int main(int argc, char **argv) {
  int opt;
  char *host = NULL;
  char *ports = NULL;
  char *network = NULL;

  while ((opt = getopt(argc, argv, "H:p:n:")) != -1) {
    switch (opt) {
    case 'H':
      host = optarg;
      break;
    case 'p':
      ports = optarg;
      break;
    case 'n':
      network = optarg;
      break;
    default:
      help();
      exit(EXIT_FAILURE);
    }
  }
  if (optind != argc) {
    help();
    exit(EXIT_FAILURE);
  }
  int is_hp = ports != NULL && host != NULL;
  int is_net = network != NULL;
  if (is_hp && !is_net) {
    // scan
    uint32_t i1, i2;
    char *delim = strchr(ports, '-');
    if (delim == NULL) {
      help();
      exit(EXIT_FAILURE);
    }
    i1 = atoi(ports);
    i2 = atoi(delim + 1);
    // i hate c "strings", i hate c "strings", i hate c "strings"
    if (!scan(host, i1, i2)) {
      printf("Failed to scan");
    }
  } else if (!is_hp && is_net) {
    // ping
    SV sv_ip = init_SV(network);
    SV sv_cidr = cut_by_delim(&sv_ip, '/');

    int cidr = atoi(sv_cidr.str);
    if (cidr > 32 || cidr < 0) {
      return 1;
    }
    if (cidr == 32) {
      // single host
      sv_ip.str[strcspn(sv_ip.str, "/")] = '\0';
      printf("ip = %s\n\n", sv_ip.str);
      ping(sv_ip.str);
    } else if (cidr == 31) {
      // point to point
      sv_ip.str[strcspn(sv_ip.str, "/")] = '\0';
      uint32_t x[4];
      x[0] = atoi(sv_ip.str);
      SV sv_tmp = sv_ip;
      for (int i = 1; i < 4; i++) {
        sv_tmp = cut_by_delim(&sv_tmp, '.');
        x[i] = atoi(sv_tmp.str);
      }
      uint32_t ip = x[0] << 24 | x[1] << 16 | x[2] << 8 | x[3];
      char second_ip[256] = {0};
      ip_bintostr(second_ip, sizeof(second_ip), ip + 1);
      printf("ips: %s\t%s\n\n", sv_ip.str, second_ip);
      ping(sv_ip.str);
      ping(second_ip);
    } else {
      uint64_t nb_addrs = 1ULL << (32 - cidr);
      printf("nb_addrs=2^%d=%lu\n", (32 - cidr), nb_addrs);
      uint32_t x[4];
      x[0] = atoi(sv_ip.str);
      SV tmp = sv_ip;
      for (int i = 1; i < 4; i++) {
        tmp = cut_by_delim(&tmp, '.');
        x[i] = atoi(tmp.str);
      }
      uint32_t ip = x[0] << 24 | x[1] << 16 | x[2] << 8 | x[3];
      uint32_t mask = 0xFFFFFFFF - nb_addrs + 1;
      uint32_t network = ip & mask;
      uint32_t broadcast = network + nb_addrs - 1;
      char buffer[256] = {0};
      ip_bintostr(buffer, sizeof(buffer), mask);
      printf("mask = %s\n", buffer);
      ip_bintostr(buffer, sizeof(buffer), network);
      printf("network = %s\n", buffer);
      ip_bintostr(buffer, sizeof(buffer), broadcast);
      printf("broadcast = %s\n", buffer);
      printf("ips:\n");
      for (uint32_t i = network + 1; i < broadcast; i++) {
        char tmp[256] = {0};
        ip_bintostr(tmp, sizeof(tmp), i);
        printf("%s\t", tmp);
      }
      printf("\n\n");
      for (uint32_t i = network + 1; i < broadcast; i++) {
        char tmp[256] = {0};
        ip_bintostr(tmp, sizeof(tmp), i);
        ping(tmp);
      }
    }
  } else {
    help();
    exit(EXIT_FAILURE);
  }
  return 1;
}

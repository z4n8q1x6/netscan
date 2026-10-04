#include "ping.h"
#include "util.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static uint16_t checksum(void *addr, size_t count) {
  uint32_t sum = 0;
  uint16_t *p = (uint16_t *)addr;
  while (count > 1) {
    sum += *p;
    p++;
    count -= 2;
  }
  // handle odd number of bytes
  if (count > 0) {
    sum += *((uint8_t *)p);
  }

  // handle overflow (carry)
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  uint16_t checksum = ~sum;
  return checksum;
}

int ping(const char *ip) {
  int icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (icmp_sock == -1) {
    perror("socket");
    return 0;
  }
  struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
  if (setsockopt(icmp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
    perror("setsockopt");
    return 0;
  }
  struct sockaddr_in dest_addr;
  dest_addr.sin_family = AF_INET;
  int parsing = inet_pton(AF_INET, ip, &dest_addr.sin_addr);
  if (parsing != 1) {
    if (parsing == -1) {
      perror("inet_pton");
    }
    return 0;
  }
  char data_str[] = "sup world";
  size_t data_len = sizeof(data_str);
  size_t packet_size = sizeof(struct icmphdr) + data_len;
  char *packet = malloc(packet_size);
  if (packet == NULL) {
    perror("malloc");
    return 0;
  }
  struct icmphdr *icmp = (struct icmphdr *)packet;
  icmp->type = ICMP_ECHO;
  icmp->code = 0;
  icmp->un.echo.id = (uint16_t)getpid();
  icmp->un.echo.sequence = 1;
  icmp->checksum = 0;
  char *data = packet + sizeof(struct icmphdr);
  memcpy(data, data_str, data_len);
  icmp->checksum = checksum(packet, packet_size);

  ssize_t sent = sendto(icmp_sock, (void *)packet, packet_size, 0,
                        (struct sockaddr *)&dest_addr, sizeof(dest_addr));

  if (sent == -1) {
    perror("sendto");
    return 0;
  } else {
    printf("Sent %ld bytes to %s\n", sent, ip);
  }

  char buffer[1024] = {0};
  ssize_t received =
      recvfrom(icmp_sock, (void *)&buffer, sizeof(buffer), 0, NULL, NULL);
  if (received == -1) {
    perror("recvfrom");
    return 0;
  }

  struct iphdr *ipheader = (struct iphdr *)buffer;
  size_t ipheader_size = ipheader->ihl * 4;
  struct icmphdr *reply = (struct icmphdr *)(buffer + ipheader_size);
  if (reply->type == ICMP_ECHOREPLY &&
      reply->un.echo.id == (uint16_t)getpid()) {
    printf("Received %ld bytes from %s\n", received - ipheader_size, ip);
    return 1;
  }
  printf("Failed to receive reply\n");
  return 0;
}

#ifndef PING_H
#define PING_H

#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

int ping(const char *ip);

#endif // PING_H

#include "util.h"
#include "stdio.h"
#include <string.h>

void help() {
  fprintf(stderr, "Usage: netscan -H <host> -p <portX-portY>\n");
  fprintf(stderr, "       netscan -n <network/CIDR>\n");
}

void ip_bintostr(char *buffer, size_t len, uint32_t sum) {
  uint32_t tmp[4];
  tmp[0] = sum >> 24;
  tmp[1] = (uint8_t)(sum >> 16);
  tmp[2] = (uint8_t)(sum >> 8);
  tmp[3] = (uint8_t)sum;
  snprintf(buffer, len, "%u.%u.%u.%u", tmp[0], tmp[1], tmp[2], tmp[3]);
}

SV init_SV(char *str) { return (SV){.str = str, .len = strlen(str)}; }
SV cut_by_delim(SV *sv, char delim) {
  int i = 0;
  while (sv->str[i] != delim) {
    i++;
  }
  if (i == sv->len)
    return *sv;
  // Hi-World
  // 01234567
  SV ret = {.str = sv->str + i + 1, .len = sv->len - i - 1};
  sv->len = i;
  return ret;
}

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <unistd.h>

typedef struct {
  char *str;
  size_t len;
} SV;

#define SV_FMT "%.*s"
#define SV_ARG(sv) (int)(sv).len, (sv).str

SV init_SV(char *str);
SV cut_by_delim(SV *sv, char delim);
void help();
void ip_bintostr(char *buffer, size_t len, uint32_t sum);

#endif // UTIL_H

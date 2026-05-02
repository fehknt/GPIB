#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "gpib_utils.h"

S64 ascnum(const C8 *string, U32 base, C8 **end) {
  if (string == NULL) return 0;
  
  const C8 *ptr = string;
  while (*ptr == ' ') {
    ptr++;
  }

  U32 i, j;
  U64 total = 0L;
  S64 sgn = 1;

  for (i = 0; ptr[i] != '\0'; i++) {
    if ((i == 0) && (ptr[i] == '-')) {
      sgn = -1;
      continue;
    }

    for (j = 0; j < base; j++) {
      if (toupper((U8)ptr[i]) == "0123456789ABCDEF"[j]) {
        total = (total * (U64)base) + (U64)j;
        break;
      }
    }

    if (j == base) {
      if (end != NULL) {
        *end = (C8*)&ptr[i];
      }

      return sgn * (S64)total;
    }
  }

  if (end != NULL) {
    *end = (C8*)&ptr[i];
  }

  return sgn * (S64)total;
}


C8 *stristr(const C8 *str1, const C8 *str2, bool underscores_match_hyphens) {
  if (str1 == NULL || str2 == NULL) return NULL;
  
  C8 *result = NULL;
  C8 *t1 = _strdup(str1);
  C8 *t2 = _strdup(str2);

  if (t1 == NULL || t2 == NULL) {
      if (t1) free(t1);
      if (t2) free(t2);
      return NULL;
  }

  _strupr(t1);
  _strupr(t2);

  if (underscores_match_hyphens) {
    for (C8 *t = t1; *t; t++)
      if (*t == '-')
        *t = '_';
    for (C8 *t = t2; *t; t++)
      if (*t == '-')
        *t = '_';
  }

  C8 *res = strstr(t1, t2);

  if (res != NULL) {
    result = (C8*)&str1[res - t1];
  }

  free(t1);
  free(t2);

  return result;
}

void kill_trailing_whitespace(C8 *dest) {
  if (dest == NULL) return;
  S32 l = (S32)strlen(dest);

  while (--l >= 0) {
    if (!isspace((U8)dest[l])) {
      dest[l + 1] = 0;
      break;
    }
  }
  if (l < 0) dest[0] = 0;
}

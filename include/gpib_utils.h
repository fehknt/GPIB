#ifndef GPIB_UTILS_H
#define GPIB_UTILS_H

#include "typedefs.h"

// Numeric conversion
S64 ascnum(const C8 *string, U32 base, C8 **end = NULL);

// String utilities
C8 *stristr(const C8 *str1, const C8 *str2, bool underscores_match_hyphens = FALSE);
void kill_trailing_whitespace(C8 *dest);

#endif

#ifndef DOS2ANSI_SAUCEQUERY_H
#define DOS2ANSI_SAUCEQUERY_H

#include "decl.h"

C_CLASS_DECL(Sauce);

const char *SauceQuery_check(const char *query)
    ATTR_NONNULL((1));
int SauceQuery_print(const Sauce *sauce, const char *query, int crlf)
    ATTR_NONNULL((1)) ATTR_NONNULL((2));

#endif

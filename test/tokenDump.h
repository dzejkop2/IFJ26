#ifndef TOKEN_DUMP_H
#define TOKEN_DUMP_H

#include "../scanner.h"

void dumpTokens(void);

/**
 * @brief Returns a readable name of a token type (used by parser debug prints)
 */
const char *specName(specTokenType t);

#endif

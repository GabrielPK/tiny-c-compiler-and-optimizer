/*
 * File:	literal.h
 *
 * Description:	This file contains the public function declarations for
 *		handling integer and string literals as symbols.
 */

# ifndef LITERAL_H
# define LITERAL_H
# include <string>
# include "Symbol.h"

Symbol *makeLiteral(const std::string &name);
Symbol *makeLiteral(int value);

const Symbols getLiterals();

int valueOf(Symbol *sym);
bool isNumber(Symbol *sym);

# endif /* LITERAL_H */

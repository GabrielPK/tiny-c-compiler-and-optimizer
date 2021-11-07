/*
 * File:	Scope.h
 *
 * Description:	This file contains the class definitions for Scopes in Tiny
 *		C.  Each scope contains a vector of symbols and a link to
 *		its enclosing scope.
 *
 *		We could have used a map instead of a vector, but we want
 *		to maintain declaration order and we expect the number of
 *		symbols to be small.
 */

# ifndef SCOPE_H
# define SCOPE_H
# include <string>
# include <vector>
# include "Symbol.h"

class Scope {
    typedef std::string string;

    Scope *_enclosing;
    Symbols _symbols;

public:
    Scope(Scope *enclosing = nullptr);

    Scope *enclosing() const;
    const Symbols &symbols() const;

    void insert(Symbol *symbol);
    Symbol *find(const string &name) const;
    Symbol *lookup(const string &name) const;
};

# endif /* SCOPE_H */
